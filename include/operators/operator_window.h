#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "concepts_observable.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_containers.h"
#include "subject.h"
#include "xrx_prologue.h"
#include <type_traits>
#include <algorithm>
#include <vector>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, bool IsAsync>
    struct WindowProducerObservable; // Not yet implemented.

    template<typename Value>
    struct WindowSyncObservable
    {
        using value_type = Value;
        using error_type = void_;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;

        using Storage = SmallVector<value_type, 1>;
        Storage _values;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            const bool all = _values.for_each([&](const value_type& v) XRX_FORCEINLINE_LAMBDA()
            {
                const auto action = on_next_with_action(observer, value_type(v)); // copy.
                if (action._stop)
                {
                    return false;
                }
                return true;
            });
            if (all)
            {
                (void)on_completed_optional(XRX_MOV(observer));
            }
            return NoopDetach();
        }

        WindowSyncObservable fork() && { return WindowSyncObservable(XRX_MOV(*this)); }
        WindowSyncObservable fork() &  { return WindowSyncObservable(*this); }
    };

    template<typename SourceObservable>
    struct WindowProducerObservable<SourceObservable, false/*synchronous*/>
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using source_value = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using ObservableValue_  = WindowSyncObservable<source_value>;
        using value_type   = Observable_<ObservableValue_>;
        using error_type   = source_error;
        using is_async     = std::false_type;
        using DetachHandle = NoopDetach;

        using Storage = ObservableValue_::Storage;

        struct State_
        {
            bool _unsubscribed = false;
            bool _end_with_error = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct ObserverImpl_
        {
            State_* _state = nullptr;
            Observer* _observer = nullptr;
            std::size_t _count = 0;
            Storage _values;

            auto on_next(XRX_RVALUE(source_value&&) value)
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                if (_values._size < _count)
                {
                    if (_values._size == 0)
                    {
                        _values = Storage(_count);
                    }
                    _values.push_back(XRX_MOV(value));
                }
                assert(_values._size <= _count);
                if (_values._size == _count)
                {
                    // #XXX: since we store/cache emitted values to Vector,
                    // we do not preserver Time information. Should this
                    // be the issue for Sync source observable ?
                    // If we are not going to cache, there
                    // should be something like Subject<> passed as a window.
                    // This would also mean window would be single pass/one time pass.
                    // (I.e., caller needs to subscribe immediately.)
                    // How RxCpp behaves ?
                    // Also, any Observable/Operator (window_toggle currently) that uses
                    // Subject<> as implementation detail has same issue:
                    // passed to the client Observable is single-pass; needs to be consumed
                    // immediately right during source/parent values emission.
                    const auto action = on_next_with_action(*_observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    if (action._stop)
                    {
                        _state->_unsubscribed = true;
                        return ::xrx::unsubscribe(true);
                    }
                }
                return ::xrx::unsubscribe(false);
            }

            void on_completed()
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);

                bool finalize_ = true;
                if (_values._size > 0)
                {
                    const auto action = on_next_with_action(*_observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    finalize_ = (not action._stop);
                }
                if (finalize_)
                {
                    (void)on_completed_optional(XRX_MOV(*_observer));
                }
                _state->_completed = true;
            }

            void on_error(XRX_RVALUE(error_type&&) e)
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);

                on_error_optional(XRX_MOV(*_observer), XRX_MOV(e));
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                // It's unknown how many observables we should emit, just end the sequence.
                (void)on_completed_optional(XRX_MOV(observer));
                return DetachHandle();
            }
            State_ state;
            auto detach = XRX_FWD(_source).subscribe(
                ObserverImpl_<Observer>(&state, &observer, _count, Storage()));
            static_assert(not decltype(detach)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return DetachHandle();
        }

        WindowProducerObservable fork() && { return WindowProducerObservable(XRX_MOV(_source), _count); };
        WindowProducerObservable fork() &  { return WindowProducerObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    struct WindowProducerObservable<SourceObservable, true/*Async*/>
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using source_value = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using Subject_ = Subject_<source_value, source_error>;

        using value_type   = typename Subject_::Observable;
        using error_type   = source_error;
        using is_async     = std::true_type;
        using DetachHandle = typename SourceObservable::DetachHandle;

        template<typename Observer>
        struct StateImpl_
        {
            Observer _observer;
            Subject_ _producer;
            std::size_t _count = 0;
            std::size_t _cursor = 0;

            explicit StateImpl_(XRX_RVALUE(Observer&&) observer, std::size_t count)
                : _observer(XRX_MOV(observer))
                , _producer()
                , _count(count)
                , _cursor(0)
            {}

            bool try_start()
            {
                _producer = Subject_();
                const auto action = on_next_with_action(_observer, _producer.as_observable());
                return (not action._stop);
            }

            auto on_next(XRX_RVALUE(source_value&&) value)
            {
                assert(_cursor < _count);
                _producer.on_next(XRX_MOV(value));
                ++_cursor;
                if (_cursor == _count)
                {
                    _cursor = 0;
                    _producer.on_completed();
                    if (not try_start())
                    {
                        return ::xrx::unsubscribe(true);
                    }
                }
                return ::xrx::unsubscribe(false);
            }

            void on_completed()
            {
                _producer.on_completed();
                on_completed_optional(XRX_MOV(_observer));
            }

            void on_error(XRX_RVALUE(error_type&&) e)
            {
                _producer.on_error(error_type(e)); // copy.
                on_error_optional(XRX_MOV(_observer), XRX_MOV(e));
            }
        };

        template<typename Observer>
        struct ObserverImpl_
        {
            // shared_ptr<> is needed there only because of
            // initial call to .try_start() that triggers target's
            // Observer on_next(). After that we can't move
            // target, hence need to save it to a box and move a box instead.
            std::shared_ptr<StateImpl_<Observer>> _shared;

            auto on_next(XRX_RVALUE(source_value&&) value)
            {
                return _shared->on_next(XRX_MOV(value));
            }

            void on_completed()
            {
                return _shared->on_completed();
            }

            void on_error(XRX_RVALUE(error_type&&) e)
            {
                return _shared->on_error(XRX_MOV(e));
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                // It's unknown how many observables we should emit, just end the sequence.
                (void)on_completed_optional(XRX_MOV(observer));
                return DetachHandle();
            }

            auto shared = std::make_shared<StateImpl_<Observer>>(XRX_MOV(observer), _count);
            if (not shared->try_start())
            {
                return DetachHandle();
            }
            return XRX_FWD(_source).subscribe(XRX_MOV(ObserverImpl_<Observer>(XRX_MOV(shared))));
        }

        WindowProducerObservable fork() && { return WindowProducerObservable(XRX_MOV(_source), _count); };
        WindowProducerObservable fork() &  { return WindowProducerObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Window
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        using Impl = WindowProducerObservable<SourceObservable
            , IsAsyncObservable<SourceObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto window(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Window()
                , XRX_MOV(source), std::size_t(_count));
        };
    }
} // namespace xrx

#include "xrx_epilogue.h"
