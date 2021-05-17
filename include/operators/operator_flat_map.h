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
    struct FlatMapIdentity
    {
        template<typename SourceValue, typename InnerValue>
        InnerValue operator()(XRX_RVALUE(SourceValue&&) /*main_value*/, XRX_RVALUE(InnerValue&&) inner_value) const
        {
            static_assert(not std::is_lvalue_reference_v<SourceValue>);
            static_assert(not std::is_lvalue_reference_v<InnerValue>);
            return XRX_MOV(inner_value);
        }
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map
        , bool IsSourceAsync
        , bool IsProducedAsync>
    struct FlatMapObservable;

    struct SyncState
    {
        bool _unsubscribed = false;
        bool _end_with_error = false;
        bool _completed = false;
    };

    template<typename Observer, typename Value, typename Map, typename SourceValue>
    struct InnerSyncObserver
    {
        Observer* _destination = nullptr;
        SyncState* _state = nullptr;
        Map* _map = nullptr;
        SourceValue* _source_value = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(Value&&) value)
        {
            static_assert(not std::is_lvalue_reference_v<Value>);
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            const auto action = on_next_with_action(*_destination
                , (*_map)(SourceValue(*_source_value)/*copy*/, XRX_MOV(value)));
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

    template<typename ProducedValue, typename Observer, typename Observable, typename Map, typename SourceValue>
    static bool invoke_inner(Observer& destination_
        , XRX_RVALUE(Observable&&) inner
        , Map& map
        , XRX_RVALUE(SourceValue&&) source_value)
    {
        static_assert(not std::is_lvalue_reference_v<Observable>);
        static_assert(not std::is_lvalue_reference_v<SourceValue>);
        using InnerSync_ = InnerSyncObserver<Observer, ProducedValue, Map, SourceValue>;
        SyncState state;
        auto unsubscribe = XRX_MOV(inner).subscribe(
            InnerSync_(&destination_, &state, &map, &source_value));
        static_assert(not decltype(unsubscribe)::has_effect::value
            , "Sync Observable should not have unsubscribe.");
        const bool stop = (state._unsubscribed || state._end_with_error);
        assert((state._completed || stop)
            && "Sync Observable should be ended after .subscribe() return.");
        return (not stop);
    };

    template<typename Observer, typename Producer, typename Map, typename SourceValue, typename ProducedValue>
    struct OuterSyncObserver
    {
        Producer* _produce = nullptr;
        Map* _map = nullptr;
        Observer* _destination = nullptr;
        SyncState* _state = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            SourceValue copy = source_value;
            const bool continue_ = invoke_inner<ProducedValue>(
                *_destination
                , (*_produce)(XRX_MOV(source_value))
                , *_map
                , XRX_MOV(copy));
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
        , typename Produce
        , typename Map>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , Map
        , false/*synchronous source Observable*/
        , false/*synchronous Observable that was produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        [[no_unique_address]] SourceObservable _source;
        [[no_unique_address]] Produce _produce;
        [[no_unique_address]] Map _map;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(std::is_same_v<source_error, produce_error>
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = source_error;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(Observer&& destination_) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterSyncObserver<Observer_, Produce, Map, source_type, produce_type>;
            SyncState state;
            auto unsubscribe = XRX_MOV(_source).subscribe(
                Root_(&_produce, &_map, &destination_, &state));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }
    };

    template<typename SourceObservable, typename Produce, typename Map>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce, XRX_RVALUE(Map&&) map)
            //requires ConceptObservable<SourceObservable>
            //      && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Map_ = std::remove_reference_t<Map>;
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable_::value_type>()));
        using Impl = FlatMapObservable<
              SourceObservable_
            , ProducedObservable
            , Produce
            , Map_
            , IsAsyncObservable<SourceObservable_>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), XRX_MOV(map)));
    }

    template<typename SourceObservable, typename Produce, typename Map>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce)
            //requires ConceptObservable<SourceObservable>
            //      && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Map_ = FlatMapIdentity;
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable_::value_type>()));
        using Impl = FlatMapObservable<
              SourceObservable_
            , ProducedObservable
            , Produce
            , Map_
            , IsAsyncObservable<SourceObservable_>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), FlatMapIdentity()));
    }

} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F, typename Map>
        struct RememberFlatMap
        {
            [[no_unique_address]] F _produce;
            [[no_unique_address]] Map _map;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::FlatMap
                    , SourceObservable, F, Map>
            {
                static_assert(not std::is_lvalue_reference_v<SourceObservable>);
                return make_operator(operator_tag::FlatMap()
                    , XRX_MOV(source), XRX_MOV(_produce), XRX_MOV(_map));
            }
        };
    } // namespace detail

    template<typename F>
    auto flat_map(XRX_RVALUE(F&&) produce)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        using F_ = std::remove_reference_t<F>;
        using IdentityMap = detail::FlatMapIdentity;
        return detail::RememberFlatMap<F_, IdentityMap>(XRX_MOV(produce), IdentityMap());
    }

    template<typename F, typename Map>
    auto flat_map(XRX_RVALUE(F&&) produce, XRX_RVALUE(Map&&) map)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        static_assert(not std::is_lvalue_reference_v<Map>);
        using F_ = std::remove_reference_t<F>;
        using Map_ = std::remove_reference_t<Map>;
        return detail::RememberFlatMap<F_, Map_>(XRX_MOV(produce), XRX_MOV(map));
    }
} // namespace xrx
