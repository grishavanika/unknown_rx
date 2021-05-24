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
        InnerValue operator()(XRX_RVALUE(SourceValue&&) main_value, XRX_RVALUE(InnerValue&&) inner_value) const
        {
            XRX_CHECK_RVALUE(main_value);
            XRX_CHECK_RVALUE(inner_value);
            return XRX_MOV(inner_value);
        }
    };

    template<typename E1, typename E2>
    struct MergedErrors
    {
        using are_compatible = std::is_same<E1, E2>;
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
            XRX_CHECK_RVALUE(value);
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
            }
            _state->_end_with_error = true;
        }
    };

    template<typename ProducedValue, typename Observer, typename Observable, typename Map, typename SourceValue>
    XRX_FORCEINLINE() static bool invoke_inner_sync_sync(Observer& destination_
        , XRX_RVALUE(Observable&&) inner
        , Map& map
        , XRX_RVALUE(SourceValue&&) source_value)
    {
        XRX_CHECK_RVALUE(inner);
        XRX_CHECK_RVALUE(source_value);
        using InnerSync_ = InnerObserver_Sync_Sync<Observer, ProducedValue, Map, SourceValue>;
        State_Sync_Sync state;
        auto detach = XRX_MOV(inner).subscribe(
            InnerSync_(&destination_, &state, &map, &source_value));
        static_assert(not decltype(detach)::has_effect::value
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
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
        using DetachHandle = NoopDetach;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Sync<Observer_, Produce, Map, source_type, produce_type>;
            State_Sync_Sync state;
            auto detach = XRX_MOV(_source).subscribe(
                Root_(&_produce, &_map, &destination_, &state));
            static_assert(not decltype(detach)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return DetachHandle();
        }
    };

    // The state of single subscription.
    template<typename SourceValue, typename Observable>
    struct ChildObservableState_Sync_Async
    {
        using DetachHandle = typename Observable::DetachHandle;

        SourceValue _source_value;
        std::optional<Observable> _observable;
        std::optional<DetachHandle> _detach;
    };

    // State for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState_Sync_Async
    {
        using Child_ = ChildObservableState_Sync_Async<SourceValue, Observable>;
        std::vector<Child_> _children;
        bool _unsubscribe = false;
#if (0)
        using Mutex = debug::AssertMutex<>;
#else
        using Mutex = std::mutex;
#endif
        Mutex* _mutex = nullptr;
    };

    template<typename Observable, typename SourceValue>
    struct Detach_Sync_Async
    {
        using has_effect = std::true_type;

        using AllObservables_ = AllObservablesState_Sync_Async<Observable, SourceValue>;
        std::shared_ptr<AllObservables_> _shared;

        bool operator()()
        {
            // Note: not thread-safe.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                return false;
            }
            assert(shared->_mutex);
            auto guard = std::lock_guard(*shared->_mutex);
            shared->_unsubscribe = true;
            bool at_least_one = false;
            for (auto& child : shared->_children)
            {
                // #XXX: invalidate unsubscribers references.
                assert(child._detach);
                const bool detached = (*child._detach)();
                at_least_one |= detached;
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
        using AllObservables = AllObservablesState_Sync_Async<Observable, SourceValue>;
        using Mutex = typename AllObservables::Mutex;
        AllObservables _observables;
        Observer _observer;
        Map _map;
        std::size_t _subscribed_count;
        std::size_t _completed_count;
        Mutex _mutex;

        explicit SharedState_Sync_Async(XRX_RVALUE(AllObservables&&) observables
            , XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Map&&) map)
                : _observables(XRX_MOV(observables))
                , _observer(XRX_MOV(observer))
                , _map(XRX_MOV(map))
                , _subscribed_count(0)
                , _completed_count(0)
                , _mutex()
        {
            _observables._mutex = &_mutex;
        }

        void unsubscribe_all(std::size_t ignore_index)
        {
            for (std::size_t i = 0, count = _observables._children.size(); i < count; ++i)
            {
                if (i == ignore_index)
                {
                    // #TODO: explain.
                    continue;
                }
                auto& child = _observables._children[i];
                assert(child._detach);
                (*child._detach)();
                // Note, we never reset _detach optional.
                // May be accessed by external Unsubscriber user holds.
            }
        }

        unsubscribe on_next(std::size_t child_index, XRX_RVALUE(InnerValue&&) inner_value)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size());
            if (_observables._unsubscribe)
            {
                // #XXX: suspicious.
                // Not needed: called in all cases when `_unsubscribe` is set to true.
                // unsubscribe_all();
                return unsubscribe(true);
            }

            auto source_value = _observables._children[child_index]._source_value;
            const auto action = on_next_with_action(_observer
                , _map(XRX_MOV(source_value), XRX_MOV(inner_value)));
            if (action._stop)
            {
                _observables._unsubscribe = true;
                unsubscribe_all(child_index);
                return unsubscribe(true);
            }
            return unsubscribe(false);
        }
        template<typename... VoidOrError>
        void on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size());
            assert(not _observables._unsubscribe);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_observer));
            }
            else
            {
                auto guard = std::lock_guard(_observables._serialize);
                on_error_optional(XRX_MOV(_observer), XRX_MOV(e)...);
            }
            _observables._unsubscribe = true;
            unsubscribe_all(child_index);
        }
        void on_completed(std::size_t child_index)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size()); (void)child_index;
            assert(not _observables._unsubscribe);
            ++_completed_count;
            assert(_completed_count <= _subscribed_count);
            if (_completed_count == _subscribed_count)
            {
                on_completed_optional(XRX_MOV(_observer));
            }
#if (0) // #XXX: suspicious.
            auto& kill = _observables._children[child_index]._detach;
            assert(kill);
            kill->DetachHandle();
#endif
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
                return _shared->on_error(_index, XRX_MOV(e)...);
            }
        }
    };

    template<typename Observer, typename Producer, typename Observable, typename SourceValue>
    struct OuterObserver_Sync_Async
    {
        Observer* _destination = nullptr;
        AllObservablesState_Sync_Async<Observable, SourceValue>* _observables;
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
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

        using DetachHandle = Detach_Sync_Async<ProducedObservable, source_type>;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Async<Observer_, Produce, ProducedObservable, source_type>;
            using AllObsevables = AllObservablesState_Sync_Async<ProducedObservable, source_type>;
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
                return DetachHandle();
            }

            auto shared = std::make_shared<SharedState_>(XRX_MOV(observables)
                , XRX_MOV(destination_), XRX_MOV(_map));
            shared->_subscribed_count = shared->_observables._children.size();
            std::shared_ptr<AllObsevables> unsubscriber(shared, &shared->_observables);

            for (std::size_t i = 0; i < shared->_subscribed_count; ++i)
            {
                auto& child = shared->_observables._children[i];
                assert(child._observable);
                assert(not child._detach);
                child._detach.emplace(child._observable->fork_move()
                    .subscribe(InnerObserver_(i, shared)));
                child._observable.reset();
            }

            return DetachHandle(XRX_MOV(unsubscriber));
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
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
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

        using DetachHandle = typename SourceObservable::DetachHandle;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using OuterObserver_ = OuterObserver_Async_Sync<Observer, Produce, Map, source_type, produce_type>;
            return XRX_MOV(_source).subscribe(OuterObserver_(
                XRX_MOV(_produce), XRX_MOV(_map), XRX_MOV(destination_)));
        }
    };

    // The state of single subscription.
    template<typename Observable, typename SourceValue>
    struct ChildObservableState_Async_Async
    {
        using DetachHandle = typename Observable::DetachHandle;

        SourceValue _source_value;
        DetachHandle _detach;
    };

    // Tricky state for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    // 
    // `_unsubscribe` is valid if there is shared data valid.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState_Async_Async
    {
        using Child_ = ChildObservableState_Async_Async<Observable, SourceValue>;
        
#if (0)
        std::mutex _mutex;
#else
        debug::AssertMutex<> _mutex;
#endif
        std::vector<Child_> _children;
        bool _unsubscribe = false;

        bool detach_all_unsafe(std::size_t ignore_index)
        {
            // Should be locked.
            // auto guard = std::lock_guard(_mutex);
            if (_unsubscribe)
            {
                return false;
            }
            _unsubscribe = true;
            for (std::size_t i = 0, count = _children.size(); i < count; ++i)
            {
                if (i == ignore_index)
                {
                    // We may be asked to unsubscribe during
                    // observable's on_next/on_completed/on_error calls.
                    // DetachHandle() may destroy observer's state, hence we
                    // skip such cases.
                    // It should be fine, since we do unsubscribe in other ways
                    // (i.e., return unsubscribe(true)).
                    continue;
                }
                Child_& child = _children[i];
                // #XXX: invalidate all unsubscribers references.
                (void)child._detach();
            }
            return true;
        }
    };

    template<typename OuterDetach, typename ObservablesState>
    struct Detach_Async_Async
    {
        OuterDetach _outer;
        std::shared_ptr<ObservablesState> _observables;

        using has_effect = std::true_type;
        
        bool operator()()
        {
            // Note: not thread-safe.
            auto observables = std::exchange(_observables, {});
            if (not observables)
            {
                return false;
            }
            const bool root_detached = std::exchange(_outer, {})();
            auto guard = std::lock_guard(observables->_mutex);
            const bool at_least_one_child = observables->detach_all_unsafe(
                std::size_t(-1)/*nothing to ignore*/);
            return (root_detached or at_least_one_child);
        }
    };

    template<typename Map
        , typename Observer
        , typename Produce
        , typename SourceValue
        , typename ChildObservable>
    struct SharedState_Async_Async
    {
        using AllObservables = AllObservablesState_Async_Async<ChildObservable, SourceValue>;
        using child_value = typename ChildObservable::value_type;

        Map _map;
        Observer _destination;
        Produce _produce;
        AllObservables _observables;
        int _subscriptions_count;

        explicit SharedState_Async_Async(XRX_RVALUE(Map&&) map, XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Produce&&) produce)
            : _map(XRX_MOV(map))
            , _destination(XRX_MOV(observer))
            , _produce(XRX_MOV(produce))
            , _observables()
            , _subscriptions_count(1) // start with subscription to source
        {
        }

        bool on_next_child(XRX_RVALUE(SourceValue&&) source_value
            , std::shared_ptr<SharedState_Async_Async> self)
        {
            using InnerObserver = InnerObserver_Sync_Async<SharedState_Async_Async
                , child_value
                , typename ChildObservable::error_type>;

            // Need to guard subscription to be sure racy complete (on_completed_source())
            // will not think there are no children anymore; also external
            // unsubscription may miss child we just construct now.
            auto guard = std::lock_guard(_observables._mutex);
            if (_observables._unsubscribe)
            {
                return true;
            }
            ++_subscriptions_count;
            const std::size_t index = _observables._children.size();
            // Now, we need to insert _before_ subscription first
            // so if there will be some inner values emitted during subscription
            // we will known about Observable.
            auto copy = source_value;
            auto& state = _observables._children.emplace_back(XRX_MOV(source_value));
            state._detach = _produce(XRX_MOV(copy)).subscribe(InnerObserver(index, XRX_MOV(self)));
            return false;
        }

        void on_completed_source(std::size_t child_index)
        {
            auto lock = std::lock_guard(_observables._mutex);
            --_subscriptions_count;
            assert(_subscriptions_count >= 0);
            if (_subscriptions_count >= 1)
            {
                // Not everyone completed yet.
                return;
            }
            _observables.detach_all_unsafe(child_index);
            on_completed_optional(XRX_MOV(_destination));
        }

        template<typename... VoidOrError>
        void on_error_source(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            auto lock = std::lock_guard(_observables._mutex);
            _observables.detach_all_unsafe(child_index);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
            }
        }

        template<typename... VoidOrError>
        auto on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            (void)child_index;
            on_error_source(child_index, XRX_MOV(e)...);
        }

        auto on_completed(std::size_t child_index)
        {
            (void)child_index;
            on_completed_source(child_index);
        }

        auto on_next(std::size_t child_index, XRX_RVALUE(child_value&&) value)
        {
            auto guard = std::unique_lock(_observables._mutex);
            assert(child_index < _observables._children.size());
            auto source_value = _observables._children[child_index]._source_value;
            const auto action = on_next_with_action(
                _destination, _map(XRX_MOV(source_value), XRX_MOV(value)));
            if (action._stop)
            {
                _observables.detach_all_unsafe(child_index/*ignore*/);
                return unsubscribe(true);
            }
            return unsubscribe(false);
        }
    };

    template<typename Shared, typename SourceValue, typename Produce>
    struct OuterObserver_Async_Async
    {
        std::shared_ptr<Shared> _shared;
        Produce _produce;
        State_Sync_Sync _state;

        unsubscribe on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _state._unsubscribed = _shared->on_next_child(
                XRX_MOV(source_value)
                , _shared);
            return unsubscribe(_state._unsubscribed);
        }
        void on_completed()
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _shared->on_completed_source(std::size_t(-1));
            _state._completed = true;
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _shared->on_error_source(std::size_t(-1), XRX_MOV(e)...);
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
        , true /*Async source Observable*/
        , true /*Async Observables produced*/>
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

        using OuterDetach = typename SourceObservable::DetachHandle;
        using InnerDetach = typename ProducedObservable::DetachHandle;
        using AllObservables = AllObservablesState_Async_Async<ProducedObservable, source_type>;
        using DetachHandle = Detach_Async_Async<OuterDetach, AllObservables>;

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

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using Shared_ = SharedState_Async_Async<Map, Observer, Produce, source_type, ProducedObservable>;
            using AllObservablesRef = std::shared_ptr<typename Shared_::AllObservables>;
            using OuterObserver_ = OuterObserver_Async_Async<Shared_, source_type, Produce>;

            auto shared = std::make_shared<Shared_>(XRX_MOV(_map), XRX_MOV(destination_), XRX_MOV(_produce));
            auto observables_ref = AllObservablesRef(shared, &shared->_observables);
            auto unsubscriber = XRX_MOV(_source).subscribe(OuterObserver_(XRX_MOV(shared), XRX_MOV(_produce)));
            return DetachHandle(XRX_MOV(unsubscriber), XRX_MOV(observables_ref));
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
    template<typename F>
    auto flat_map(XRX_RVALUE(F&&) produce)
    {
        XRX_CHECK_RVALUE(produce);
        return [_produce = XRX_MOV(produce)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            using IdentityMap = detail::FlatMapIdentity;
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::FlatMap()
                , XRX_MOV(source), XRX_MOV(_produce), IdentityMap());
        };
    }

    template<typename F, typename Map>
    auto flat_map(XRX_RVALUE(F&&) produce, XRX_RVALUE(Map&&) map)
    {
        XRX_CHECK_RVALUE(produce);
        XRX_CHECK_RVALUE(map);
        return [_produce = XRX_MOV(produce), _map = XRX_MOV(map)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::FlatMap()
                , XRX_MOV(source), XRX_MOV(_produce), XRX_MOV(_map));
        };
    }
} // namespace xrx
