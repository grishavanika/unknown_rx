#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_ref_tracking_observer.h"
#include "xrx_prologue.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <variant>
#include <mutex>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Transform, typename Tuple>
    struct TupleAsVariant;

    template<typename Transform, typename... Args>
    struct TupleAsVariant<Transform, std::tuple<Args...>>
    {
        using variant_type = std::variant<
            typename Transform::template invoke_<Args>...>;
    };

    template<typename Tuple, typename F>
    static bool runtime_tuple_get(Tuple& tuple, std::size_t index, F&& f)
    {
        constexpr std::size_t Size_ = std::tuple_size_v<Tuple>;
        if (index >= Size_)
        {
            return false;
        }
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            const bool yes_ = (((Is == index)
                ? ((bool)XRX_FWD(f)(std::get<Is>(tuple)))
                : false) || ...);
            return yes_;
        }(std::make_index_sequence<Size_>());
    }

    template<std::size_t I, typename Variant, typename Value>
    static bool variant_emplace_at(Variant& variant, std::size_t index, Value&& value)
    {
        using T = std::variant_alternative_t<I, Variant>;
        if constexpr (std::is_constructible_v<T, Value&&>)
        {
            assert(index == I); (void)index;
            variant.emplace<I>(XRX_FWD(value));
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename Variant, typename Value>
    static bool runtime_variant_emplace(Variant& variant, std::size_t index, Value&& value)
    {
        constexpr std::size_t Size_ = std::variant_size_v<Variant>;
        if (index >= Size_)
        {
            return false;
        }
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            const bool yes_ = (((Is == index)
                ? variant_emplace_at<Is>(variant, index, XRX_FWD(value))
                : false) || ...);
            return yes_;
        }(std::make_index_sequence<Size_>());
    }

    template<typename Tuple, bool IsAsync>
    struct ConcatObservable;

    template<typename Tuple>
    struct ConcatObservable<Tuple, false/*Sync*/>
    {
        static_assert(std::tuple_size_v<Tuple> >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_tuple)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        template<typename Observer>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                RefObserverState state;
                using OnePass = RefTrackingObserver_<Observer, false/*no on_completed*/>;
                auto detach = XRX_FWD(observable).subscribe(OnePass(&observer, &state));
                static_assert(not decltype(detach)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (state._unsubscribed || state._with_error);
                assert((state._completed || stop)
                    && "Sync Observable should be ended after .subscribe() return.");
                return (not stop);
            };

            const bool all_processed = std::apply([&](auto&&... observables)
            {
                return (invoke_(observer, XRX_FWD(observables)) && ...);
            }
                , XRX_MOV(_tuple));
            if (all_processed)
            {
                on_completed_optional(observer);
            }
            return NoopDetach();
        }
    };

    template<typename Tuple>
    struct ConcatObservable<Tuple, true/*Async*/>
    {
        static constexpr std::size_t Size_ = std::tuple_size_v<Tuple>;
        static_assert(Size_ >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::true_type;

        Tuple _observables;

        struct ObservableToDetach
        {
            template<typename O>
            using invoke_ = typename O::DetachHandle;
        };

        using DetachVariant = typename TupleAsVariant<ObservableToDetach, Tuple>::variant_type;

        struct Unsubscription
        {
            std::recursive_mutex _mutex;
            DetachVariant _unsubscribers;
        };

        struct Detach
        {
            using has_effect = std::true_type;
            std::shared_ptr<Unsubscription> _unsubscription;
            bool operator()()
            {
                auto shared = std::exchange(_unsubscription, {});
                if (not shared)
                {
                    return false;
                }
                auto guard = std::lock_guard(shared->_mutex);
                auto handle = [](auto&& DetachHandle)
                {
                    return DetachHandle();
                };
                return std::visit(XRX_MOV(handle), shared->_unsubscribers);
            }
        };

        using DetachHandle = Detach;

        template<typename Observer>
        struct Shared_;

        template<typename Observer>
        struct OnePassObserver
        {
            std::shared_ptr<Shared_<Observer>> _shared;

            auto on_next(XRX_RVALUE(value_type&&) value);
            void on_completed();
            void on_error(XRX_RVALUE(error_type&&) e);
        };

        template<typename Observer>
        struct Shared_ : public std::enable_shared_from_this<Shared_<Observer>>
        {
            Observer _destination;
            Tuple _observables;
            std::size_t _cursor;
            Unsubscription _unsubscription;

            XRX_FORCEINLINE() std::recursive_mutex& mutex() { return _unsubscription._mutex; }

            explicit Shared_(XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Tuple&&) observables)
                : _destination(XRX_MOV(observer))
                , _observables(XRX_MOV(observables))
                , _cursor(0)
                , _unsubscription()
            {
            }

            void start_impl(std::size_t index)
            {
                auto guard = std::lock_guard(mutex());
                assert(_cursor == index);
                const bool ok = runtime_tuple_get(_observables, index
                    , [&](auto& observable)
                {
                    using Impl = OnePassObserver<Observer>;
                    return runtime_variant_emplace(
                        _unsubscription._unsubscribers
                        , index
                        , observable.fork_move() // We move/own observable.
                            .subscribe(Impl(this->shared_from_this())));
                });
                assert(ok); (void)ok;
            }

            auto on_next(XRX_RVALUE(value_type&&) value)
            {
                auto guard = std::lock_guard(mutex());
                return on_next_with_action(_destination, XRX_MOV(value));
            }

            void on_completed()
            {
                auto guard = std::lock_guard(mutex());
                assert(_cursor < Size_);
                ++_cursor;
                if (_cursor == Size_)
                {
                    return on_completed_optional(_destination);
                }
                start_impl(_cursor);
            }

            void on_error(XRX_RVALUE(error_type&&) e)
            {
                auto guard = std::lock_guard(mutex());
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e));
            }
        };

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_observables)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_observables); }

        template<typename Observer>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            auto shared = std::make_shared<Shared_<Observer>>(XRX_MOV(observer), XRX_MOV(_observables));
            shared->start_impl(0);
            std::shared_ptr<Unsubscription> unsubscription(shared, &shared->_unsubscription);
            return DetachHandle(XRX_MOV(unsubscription));
        }
    };

    template<typename Tuple>
    template<typename Observer>
    auto ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_next(XRX_RVALUE(value_type&&) value)
    {
        return _shared->on_next(XRX_MOV(value));
    }
    template<typename Tuple>
    template<typename Observer>
    void ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_completed()
    {
        return _shared->on_completed();
    }
    template<typename Tuple>
    template<typename Observer>
    void ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_error(XRX_RVALUE(error_type&&) e)
    {
        return _shared->on_error(XRX_MOV(e));
    }

    template<typename Observable1, typename Observable2, bool>
    struct HaveSameStreamTypes
    {
        static constexpr bool value =
               std::is_same_v<typename Observable1::value_type
                            , typename Observable2::value_type>
            && std::is_same_v<typename Observable1::error_type
                            , typename Observable2::error_type>;
    };

    template<typename Observable1, typename Observable2>
    struct HaveSameStreamTypes<Observable1, Observable2
        , false> // not Observables.
            : std::false_type
    {
    };

    // AreConcatCompatible<> is a combination of different workarounds
    // for MSVC and GCC. We just need to check if value_type (+error_type) are same
    // for both types, if they are Observables.
    template<typename Observable1, typename Observable2>
    struct AreConcatCompatible
    {
        static constexpr bool are_observables =
               ConceptObservable<Observable1>
            && ConceptObservable<Observable2>;
        static constexpr bool value =
            HaveSameStreamTypes<
                Observable1
              , Observable2
              , are_observables>::value;
    };

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Concat
        , XRX_RVALUE(Observable1&&) observable1
        , XRX_RVALUE(Observable2&&) observable2
        , XRX_RVALUE(ObservablesRest&&)... observables)
            requires  (AreConcatCompatible<Observable1, Observable2>::value
                   && (AreConcatCompatible<Observable1, ObservablesRest>::value && ...))
    {
        XRX_CHECK_TYPE_NOT_REF(Observable1);
        XRX_CHECK_TYPE_NOT_REF(Observable2);
        static_assert(((not std::is_reference_v<ObservablesRest>) && ...)
            , "Expected to have non-reference types (not T& ot T&&; to be moved from). ");

        constexpr bool IsAnyAsync = false
            || ( IsAsyncObservable<Observable1>())
            || ( IsAsyncObservable<Observable2>())
            || ((IsAsyncObservable<ObservablesRest>()) || ...);

        using Tuple = std::tuple<Observable1, Observable2, ObservablesRest...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(
            XRX_MOV(observable1)
            , XRX_MOV(observable2)
            , XRX_MOV(observables)...)));
    }
} // namespace xrx::detail

#include "xrx_epilogue.h"
