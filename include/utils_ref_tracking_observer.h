#pragma once
#include "utils_observers.h"
#include "xrx_prologue.h"
#include <cassert>

namespace xrx::detail
{
    struct RefObserverState
    {
        bool _completed = false;
        bool _with_error = false;
        bool _unsubscribed = false;

        bool is_finalized() const
        {
            const bool terminated = (_completed || _with_error);
            return (terminated || _unsubscribed);
        }
    };

    template<typename Observer_, bool InvokeComplete = true>
    struct RefTrackingObserver_
    {
        Observer_* _observer = nullptr;
        RefObserverState* _state = nullptr;

        template<typename Value>
        XRX_FORCEINLINE() OnNextAction on_next(Value&& v)
        {
            XRX_CHECK_RVALUE(v);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);
            const auto action = on_next_with_action(*_observer, XRX_FWD(v));
            _state->_unsubscribed = action._stop;
            return action;
        }

        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);

            if constexpr (InvokeComplete)
            {
                on_completed_optional(XRX_MOV(*_observer));
            }
            _state->_completed = true;
        }

        template<typename Error>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(Error&&) e)
        {
            XRX_CHECK_RVALUE(e);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);

            on_error_optional(XRX_MOV(*_observer), XRX_MOV(e));
            _state->_with_error = true;
        }
    };
}

#include "xrx_epilogue.h"
