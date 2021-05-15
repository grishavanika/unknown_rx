#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "observable_interface.h"
#include <type_traits>
#include <utility>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , bool IsSourceAsync
        , bool IsProducedAsync>
    struct FlatMapObservable;

    struct SyncState
    {
        bool _unsubscribed = false;
        bool _end_with_error = false;
        bool _completed = false;
    };

    template<typename Observer, typename Value>
    struct InnerSyncObserver
    {
        Observer* _destination = nullptr;
        SyncState* _state = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(Value&&) value)
        {
            static_assert(not std::is_lvalue_reference_v<Value>);
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
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
            }
            _state->_end_with_error = true;
        }
    };

    template<typename ProducedValue, typename Observer, typename Observable>
    static bool invoke_inner(Observer& destination_, XRX_RVALUE(Observable&&) inner)
    {
        static_assert(not std::is_lvalue_reference_v<Observable>);
        SyncState state;
        auto unsubscribe = XRX_MOV(inner).subscribe(
            InnerSyncObserver<Observer, ProducedValue>(&destination_, &state));
        static_assert(not decltype(unsubscribe)::has_effect::value
            , "Sync Observable should not have unsubscribe.");
        const bool stop = (state._unsubscribed || state._end_with_error);
        assert((state._completed || stop)
            && "Sync Observable should be ended after .subscribe() return.");
        return (not stop);
    };

    template<typename Observer, typename Producer, typename SourceValue, typename ProducedValue>
    struct OuterSyncObserver
    {
        Producer* _produce = nullptr;
        Observer* _destination = nullptr;
        SyncState* _state = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            const bool continue_ = invoke_inner<ProducedValue>(
                *_destination, (*_produce)(XRX_MOV(source_value)));
            _state->_unsubscribed = (not continue_);
            return ::xrx::unsubscribe(_state->_unsubscribed);
        }
        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            on_completed_optional(*_destination);
            _state->_completed = true;
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
            }
            _state->_end_with_error = true;
        }
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , false/*synchronous source Observable*/
        , false/*synchronous Observable that was produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        SourceObservable _source;
        Produce _produce;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(std::is_same_v<source_error, produce_error>
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using value_type = produce_type;
        using error_type = source_error;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(Observer&& destination_) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterSyncObserver<Observer_, Produce, source_type, produce_type>;
            SyncState state;
            auto unsubscribe = XRX_MOV(_source).subscribe(
                Root_(&_produce, &destination_, &state));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }
    };

    template<typename SourceObservable, typename Produce>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable_::value_type>()));
        using Impl = FlatMapObservable<
              SourceObservable_
            , ProducedObservable
            , Produce
            , IsAsyncObservable<SourceObservable_>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberFlatMap
        {
            F _produce;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::FlatMap
                    , SourceObservable, F>
            {
                static_assert(not std::is_lvalue_reference_v<SourceObservable>);
                return make_operator(operator_tag::FlatMap()
                    , XRX_MOV(source), XRX_MOV(_produce));
            }
        };
    } // namespace detail

    template<typename F>
    auto flat_map(XRX_RVALUE(F&&) produce)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        using F_ = std::remove_reference_t<F>;
        return detail::RememberFlatMap<F_>(XRX_MOV(produce));
    }
} // namespace xrx
