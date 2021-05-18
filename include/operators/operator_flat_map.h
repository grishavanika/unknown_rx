#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <vector>
#include <optional>
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

    template<typename E1, typename E2>
    struct MergedErrors
    {
        using are_compatible = std::is_same<E1, E1>;
        using E = E1;
    };
    template<>
    struct MergedErrors<void, none_tag>
    {
        using are_compatible = std::true_type;
        using E = void;
    };
    template<>
    struct MergedErrors<none_tag, void>
    {
        using are_compatible = std::true_type;
        using E = void;
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map
        , bool IsSourceAsync
        , bool IsProducedAsync>
    struct FlatMapObservable;

    struct State_Sync_Sync
    {
        bool _unsubscribed = false;
        bool _end_with_error = false;
        bool _completed = false;
    };

    template<typename Observer, typename Value, typename Map, typename SourceValue>
    struct InnerObserver_Sync_Sync
    {
        Observer* _destination = nullptr;
        State_Sync_Sync* _state = nullptr;
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
    static bool invoke_inner_sync_sync(Observer& destination_
        , XRX_RVALUE(Observable&&) inner
        , Map& map
        , XRX_RVALUE(SourceValue&&) source_value)
    {
        static_assert(not std::is_lvalue_reference_v<Observable>);
        static_assert(not std::is_lvalue_reference_v<SourceValue>);
        using InnerSync_ = InnerObserver_Sync_Sync<Observer, ProducedValue, Map, SourceValue>;
        State_Sync_Sync state;
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
    struct OuterObserver_Sync_Sync
    {
        Producer* _produce = nullptr;
        Map* _map = nullptr;
        Observer* _destination = nullptr;
        State_Sync_Sync* _state = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            SourceValue copy = source_value;
            const bool continue_ = invoke_inner_sync_sync<ProducedValue>(
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
        , false/*Sync source Observable*/
        , false/*Sync Observables produced*/>
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

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Sync<Observer_, Produce, Map, source_type, produce_type>;
            State_Sync_Sync state;
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

    // The state of single subscription.
    template<typename SourceValue, typename Observable>
    struct ChildObservableState_Sync_Async
    {
        using Unsubscriber = typename Observable::Unsubscriber;

        SourceValue _source_value;
        std::optional<Observable> _observable;
        std::optional<Unsubscriber> _unsubscriber;
    };

    // Tricky state for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    // 
    // `_unsubscribe` is valid if there is shared data valid.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState
    {
        using Child_ = ChildObservableState_Sync_Async<SourceValue, Observable>;
        std::vector<Child_> _children;
        std::atomic_bool* _unsubscribe = nullptr;
    };

    template<typename Observable, typename SourceValue>
    struct Unsubscriber_Sync_Async
    {
        using has_effect = std::true_type;

        using AllObservables_ = AllObservablesState<Observable, SourceValue>;
        std::shared_ptr<AllObservables_> _shared;

        bool detach()
        {
            if (not _shared)
            {
                return false;
            }
            // `_unsubscribe` is a raw pointer into `_shared`.
            // Since _shared is valid, we know this pointer is valid too.
            assert(_shared->_unsubscribe);
            (void)_shared->_unsubscribe->exchange(true);
            bool at_least_one = false;
            for (auto& child : _shared->_children)
            {
                assert(child._unsubscriber);
                at_least_one |= child._unsubscriber->detach();
            }
            return at_least_one;
        }
    };

    template<typename Observer
        , typename Map
        , typename Observable
        , typename SourceValue
        , typename InnerValue>
    struct SharedState_Sync_Async
    {
        using AllObservables = AllObservablesState<Observable, SourceValue>;
        AllObservables _observables;
        Observer _observer;
        Map _map;
        std::size_t _subscribed_count;
        std::atomic<std::size_t> _completed_count;
        std::atomic<bool> _unsubscribe = false;
#if (0)
        debug::AssertMutex<> _serialize;
#else
        std::mutex _serialize;
#endif

        explicit SharedState_Sync_Async(XRX_RVALUE(AllObservables&&) observables
            , XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Map&&) map)
                : _observables(XRX_MOV(observables))
                , _observer(XRX_MOV(observer))
                , _map(XRX_MOV(map))
                , _subscribed_count(0)
                , _completed_count(0)
                , _unsubscribe(false)
                , _serialize()
        {
        }

        void unsubscribe_all()
        {
            // Multiple, in-parallel calls to `_children`
            // should be ok. We never invalidate it.
            for (auto& child : _observables._children)
            {
                assert(child._unsubscriber);
                child._unsubscriber->detach();
                // Note, we never reset _unsubscriber optional.
                // May be accessed by external Unsubscriber user holds.
            }
        }

        unsubscribe on_next(std::size_t child_index, XRX_RVALUE(InnerValue&&) inner_value)
        {
            assert(child_index < _observables._children.size());
            if (_unsubscribe)
            {
                // Not needed: called in all cases when `_unsubscribe` is set to true.
                // unsubscribe_all();
                return unsubscribe(true);
            }

            auto source_value = _observables._children[child_index]._source_value;
            bool stop = false;
            {
                // The lock there is ONLY to serialize (possibly)
                // parallel calls to on_next()/on_error().
                auto guard = std::lock_guard(_serialize);
                const auto action = on_next_with_action(_observer
                    , _map(XRX_MOV(source_value), XRX_MOV(inner_value)));
                stop = action._stop;
            }

            if (not stop)
            {
                return unsubscribe(false);
            }
            _unsubscribe = true;
            unsubscribe_all();
            return unsubscribe(true);
        }
        template<typename... VoidOrError>
        void on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            (void)child_index;
            assert(child_index < _observables._children.size());
            assert(not _unsubscribe);
            // The lock there is ONLY to serialize (possibly)
            // parallel calls to on_next()/on_error().
            if constexpr ((sizeof...(e)) == 0)
            {
                auto guard = std::lock_guard(_serialize);
                on_error_optional(XRX_MOV(_observer));
            }
            else
            {
                auto guard = std::lock_guard(_serialize);
                on_error_optional(XRX_MOV(_observer), XRX_MOV(e...));
            }
            _unsubscribe = true;
            unsubscribe_all();
        }
        void on_completed(std::size_t child_index)
        {
            assert(child_index < _observables._children.size());
            assert(not _unsubscribe);
            const std::size_t finished = (_completed_count.fetch_add(+1) + 1);
            assert(finished <= _subscribed_count);
            if (finished == _subscribed_count)
            {
                auto guard = std::lock_guard(_serialize);
                on_completed_optional(XRX_MOV(_observer));
            }
            auto& kill = _observables._children[child_index]._unsubscriber;
            assert(kill);
            kill->detach();
        }
    };

    template<typename SharedState_, typename Value, typename Error>
    struct InnerObserver_Sync_Async
    {
        std::size_t _index;
        std::shared_ptr<SharedState_> _shared;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(Value&&) value)
        {
            return _shared->on_next(_index, XRX_MOV(value));
        }
        XRX_FORCEINLINE() void on_completed()
        {
            return _shared->on_completed(_index);
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            if constexpr ((sizeof...(e)) == 0)
            {
                return _shared->on_error(_index);
            }
            else
            {
                return _shared->on_error(_index, XRX_MOV(e...));
            }
        }
    };

    template<typename Observer, typename Producer, typename Observable, typename SourceValue>
    struct OuterObserver_Sync_Async
    {
        Observer* _destination = nullptr;
        AllObservablesState<Observable, SourceValue>* _observables;
        Producer* _produce = nullptr;
        State_Sync_Sync* _state = nullptr;

        XRX_FORCEINLINE() void on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            SourceValue copy = source_value;
            _observables->_children.emplace_back(XRX_MOV(copy)
                , std::optional<Observable>((*_produce)(XRX_MOV(source_value))));
        }
        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            // Do not complete stream yet. Need to subscribe &
            // emit all children items.
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
        , false /*Sync source Observable*/
        , true  /*Async Observables produced*/>
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

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::true_type;

        using Unsubscriber = Unsubscriber_Sync_Async<ProducedObservable, source_type>;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Async<Observer_, Produce, ProducedObservable, source_type>;
            using AllObsevables = AllObservablesState<ProducedObservable, source_type>;
            using SharedState_ = SharedState_Sync_Async<Observer_, Map, ProducedObservable, source_type, produce_type>;
            using InnerObserver_ = InnerObserver_Sync_Async<SharedState_, produce_type, error_type>;

            State_Sync_Sync state;
            AllObsevables observables;
            auto unsubscribe = XRX_MOV(_source).subscribe(
                Root_(&destination_, &observables, &_produce, &state));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            if (observables._children.size() == 0)
            {
                // Done. Nothing to subscribe to.
                return Unsubscriber();
            }

            auto shared = std::make_shared<SharedState_>(XRX_MOV(observables)
                , XRX_MOV(destination_), XRX_MOV(_map));
            shared->_observables._unsubscribe = &shared->_unsubscribe;
            shared->_subscribed_count = shared->_observables._children.size();
            std::shared_ptr<AllObsevables> unsubscriber(shared, &shared->_observables);

            for (std::size_t i = 0; i < shared->_subscribed_count; ++i)
            {
                auto& child = shared->_observables._children[i];
                assert(child._observable);
                assert(not child._unsubscriber);
                child._unsubscriber.emplace(child._observable->fork_move()
                    .subscribe(InnerObserver_(i, shared)));
                child._observable.reset();
            }

            return Unsubscriber(XRX_MOV(unsubscriber));
        }
    };

    template<typename Observer, typename Producer, typename Map, typename SourceValue, typename ProducedValue>
    struct OuterObserver_Async_Sync
    {
        Producer _produce;
        Map _map;
        Observer _destination;
        State_Sync_Sync _state;

        explicit OuterObserver_Async_Sync(XRX_RVALUE(Producer&&) produce, XRX_RVALUE(Map&&) map, XRX_RVALUE(Observer&&) observer)
            : _produce(XRX_MOV(produce))
            , _map(XRX_MOV(map))
            , _destination(XRX_MOV(observer))
            , _state()
        {
        }

        auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            SourceValue copy = source_value;
            const bool continue_ = invoke_inner_sync_sync<ProducedValue>(
                _destination
                , (_produce)(XRX_MOV(source_value))
                , _map
                , XRX_MOV(copy));
            _state._unsubscribed = (not continue_);
            return ::xrx::unsubscribe(_state._unsubscribed);
        }
        void on_completed()
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            on_completed_optional(_destination);
            _state._completed = true;
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e...));
            }
            _state._end_with_error = true;
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
        , true  /*Async source Observable*/
        , false /*Sync Observables produced*/>
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

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::true_type;

        using Unsubscriber = typename SourceObservable::Unsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using OuterObserver_ = OuterObserver_Async_Sync<Observer, Produce, Map, source_type, produce_type>;

            OuterObserver_ outer(XRX_MOV(_produce), XRX_MOV(_map), XRX_MOV(destination_));
            return XRX_MOV(_source).subscribe(XRX_MOV(outer));
        }
    };

    template<typename SourceObservable, typename Produce, typename Map>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce, XRX_RVALUE(Map&&) map)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
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

    template<typename SourceObservable, typename Produce>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
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
