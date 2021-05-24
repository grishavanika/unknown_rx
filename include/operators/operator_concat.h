#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_fast_FWD.h"
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
        }((std::make_index_sequence<Size_>()));
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
        }((std::make_index_sequence<Size_>()));
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
        using detach = NoopDetach;

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_tuple)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        struct State
        {
            bool _unsubscribed = false;
            bool _end_with_error = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct OneObserver
        {
            Observer* _destination = nullptr;
            State* _state = nullptr;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(value_type&&) value)
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                const auto action = on_next_with_action(*_destination, XRX_MOV(value));
                _state->_unsubscribed = action._stop;
                return action;
            }
            XRX_FORCEINLINE() void on_completed()
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                _state->_completed = true; // on_completed(): nothing to do, move to next observable.
            }
            template<typename... VoidOrError>
            XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(*_destination));
                }
                else
                {
                    on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
                }
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            using Observer_ = std::remove_reference_t<Observer>;
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                State state;
                auto unsubscribe = XRX_FWD(observable).subscribe(
                    OneObserver<Observer_>(&observer, &state));
                static_assert(not decltype(unsubscribe)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (state._unsubscribed || state._end_with_error);
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
            using invoke_ = typename O::detach;
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
                auto handle = [](auto&& detach)
                {
                    return detach();
                };
                return std::visit(XRX_MOV(handle), shared->_unsubscribers);
            }
        };

        using detach = Detach;

        template<typename Observer>
        struct Shared_;

        template<typename Observer>
        struct OneObserver
        {
            std::shared_ptr<Shared_<Observer>> _shared;

            auto on_next(XRX_RVALUE(value_type&&) value);
            void on_completed();
            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e);
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
                    using Impl = OneObserver<Observer>;
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

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                auto guard = std::lock_guard(mutex());
                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(_destination));
                }
                else
                {
                    on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
                }
            }
        };

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_observables)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_observables); }

        template<typename Observer>
        detach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            using Observer_ = std::remove_reference_t<Observer>;
            auto shared = std::make_shared<Shared_<Observer_>>(XRX_MOV(observer), XRX_MOV(_observables));
            shared->start_impl(0);
            std::shared_ptr<Unsubscription> unsubscription(shared, &shared->_unsubscription);
            return detach(XRX_MOV(unsubscription));
        }
    };

    template<typename Tuple>
    template<typename Observer>
    auto ConcatObservable<Tuple, true>::OneObserver<Observer>::on_next(XRX_RVALUE(value_type&&) value)
    {
        return _shared->on_next(XRX_MOV(value));
    }
    template<typename Tuple>
    template<typename Observer>
    void ConcatObservable<Tuple, true>::OneObserver<Observer>::on_completed()
    {
        return _shared->on_completed();
    }
    template<typename Tuple>
    template<typename Observer>
    template<typename... VoidOrError>
    void ConcatObservable<Tuple, true>::OneObserver<Observer>::on_error(XRX_RVALUE(VoidOrError&&)... e)
    {
        return _shared->on_error(XRX_MOV(e)...);
    }

    template<typename Observable1, typename Observable2, bool>
    struct HaveSameStreamTypes
    {
        // #XXX: there is another similar code somewhere; move to helper.
        using error1 = typename Observable1::error_type;
        using error2 = typename Observable2::error_type;

        static constexpr bool is_void_like_error1 =
               std::is_same_v<error1, void>
            or std::is_same_v<error1, none_tag>;
        static constexpr bool is_void_like_error2 =
               std::is_same_v<error2, void>
            or std::is_same_v<error2, none_tag>;
        static constexpr bool are_compatibe_errors =
               (is_void_like_error1 and is_void_like_error2)
            or std::is_same_v<error1, error2>;

        static constexpr bool value =
               std::is_same_v<typename Observable1::value_type
                            , typename Observable2::value_type>
            && are_compatibe_errors;
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
        constexpr bool IsAnyAsync = false
            or ( IsAsyncObservable<Observable1>())
            or ( IsAsyncObservable<Observable2>())
            or ((IsAsyncObservable<ObservablesRest>()) or ...);

        using Tuple = std::tuple<
            std::remove_reference_t<Observable1>
            , std::remove_reference_t<Observable2>
            , std::remove_reference_t<ObservablesRest>...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(
            XRX_MOV(observable1)
            , XRX_MOV(observable2)
            , XRX_MOV(observables)...)));
    }
} // namespace xrx::detail
