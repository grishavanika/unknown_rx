#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "concepts_observable.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_containers.h"
#include "utils_defines.h"
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
        using error_type = none_tag;
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
        using error_type   = typename SourceObservable::error_type;
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

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);

                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(*_observer));
                }
                else
                {
                    on_error_optional(XRX_MOV(*_observer), XRX_MOV(e)...);
                }
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
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Window
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Impl = WindowProducerObservable<SourceObservable_
            , IsAsyncObservable<SourceObservable_>::value>;
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
