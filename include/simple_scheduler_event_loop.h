#pragma once
#include "utils_containers.h"
#include "utils_fast_FWD.h"
#include "debug/assert_mutex.h"

#include <functional>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <concepts>
#include <cassert>

namespace xrx::debug
{
    // Some debug code to play around `Scheduler` concept.
    // Not a final interface/API.
    // 
    struct EventLoop
    {
        using clock = std::chrono::steady_clock;
        using clock_duration = clock::duration;
        using clock_point = clock::time_point;

        struct IActionCallback
        {
            clock_point _start_from;
            clock_point _last_tick;
            clock_duration _period;

            virtual ~IActionCallback() = default;
            virtual bool invoke() = 0;
        };

        template<typename F, typename State>
        struct ActionCallback_ : IActionCallback
        {
            F _f;
            State _state;

            explicit ActionCallback_(XRX_RVALUE(F&&) f, XRX_RVALUE(State&&) state)
                : _f(XRX_MOV(f))
                , _state(XRX_MOV(state))
            {
            }

            virtual bool invoke() override
            {
                return _f(_state);
            }
        };
        // Need to be able to copy when executing - to release a lock, hence shared_ptr<>.
        // Can be avoided if request to remove/cancel is delayed to next frame ?
        // This way we can use std::function<> only, probably.
        using TickAction = std::shared_ptr<IActionCallback>;

        template<typename F, typename State>
        static std::unique_ptr<IActionCallback> make_action(
            clock_point start_from, clock_duration period
            , XRX_RVALUE(F&&) f, XRX_RVALUE(State&&) state)
        {
            XRX_CHECK_RVALUE(f);
            XRX_CHECK_RVALUE(state);
            using Action_ = ActionCallback_<std::remove_reference_t<F>, std::remove_reference_t<State>>;
            auto action = std::make_unique<Action_>(XRX_MOV(f), XRX_MOV(state));
            action->_start_from = start_from;
            action->_last_tick = {};
            action->_period = period;
            return action;
        }

        using TickActions = ::xrx::detail::HandleVector<TickAction>;
        using ActionHandle = typename TickActions::Handle;
        TickActions _tick_actions;
        mutable AssertMutex<> _assert_mutex_tick;

        using TaskCallback = std::function<void ()>;
        using Tasks = ::xrx::detail::HandleVector<TaskCallback>;
        using TaskHandle = typename Tasks::Handle;
        Tasks _tasks;
        mutable AssertMutex<> _assert_mutex_tasks;

        template<typename F, typename State>
            requires std::convertible_to<std::invoke_result_t<F, State&>, bool>
        ActionHandle tick_every(clock_point start_from, clock_duration period, F&& f, State&& state)
        {
            auto guard = std::lock_guard(_assert_mutex_tick);
            const ActionHandle handle = _tick_actions.push_back(make_action(
                start_from
                , clock_duration(period)
                , XRX_FWD(f)
                , XRX_FWD(state)));
            return handle;
        }

        bool tick_cancel(ActionHandle handle)
        {
            auto guard = std::lock_guard(_assert_mutex_tick);
            return _tick_actions.erase(handle);
        }

        template<typename F>
        TaskHandle task_post(F&& task)
        {
            const TaskHandle handle = _tasks.push_back(TaskCallback(XRX_FWD(task)));
            return handle;
        }

        bool task_cancel(TaskHandle handle)
        {
            return _tasks.erase(handle);
        }

        std::size_t poll_one_task()
        {
            TaskCallback task;
            {
                auto guard = std::lock_guard(_assert_mutex_tasks);
                if (_tasks.size() == 0)
                {
                    return 0;
                }
                for (std::size_t i = 0; i < _tasks._values.size(); ++i)
                {
                    const TaskHandle handle(_tasks._values[i]._version, std::uint32_t(i));
                    if (handle)
                    {
                        TaskCallback* callback = _tasks.get(handle);
                        assert(callback);
                        task = XRX_MOV(*callback);
                        _tasks.erase(handle);
                        break; // find & execute first task.
                    }
                }
            }
            if (task)
            {
                XRX_MOV(task)();
                return 1;
            }
            return 0;
        }

        std::size_t poll_one()
        {
            ActionHandle to_execute;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                clock_duration smallest = clock_duration::max();
                for (auto&& [action, handle] : _tick_actions.iterate_with_handle())
                {
                    const clock_duration point = (action->_last_tick + action->_period).time_since_epoch();
                    if (point < smallest)
                    {
                        smallest = point;
                        to_execute = handle;
                    }
                }
            }
            if (not to_execute)
            {
                return poll_one_task();
            }

            clock_point desired_point;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                TickAction* action = _tick_actions.get(to_execute);
                if (not action)
                {
                    // Someone just removed it.
                    return 0;
                }
                desired_point = ((*action)->_last_tick + (*action)->_period);
            }
        
            // #TODO: resume this one when new item arrives
            // (with probably closer-to-execute time point).
            std::this_thread::sleep_until(desired_point); // no-op if time in the past.

            TickAction invoke_action;
            bool do_remove = false;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                if (TickAction* action = _tick_actions.get(to_execute))
                {
                    invoke_action = *action; // copy shared.
                }
            }
            if (invoke_action)
            {
                // Notice: we don't have lock.
                do_remove = invoke_action->invoke();
            }
            if (do_remove)
            {
                const bool removed = tick_cancel(to_execute);
                assert(removed); (void)removed;
                return 1;
            }
            auto guard = std::lock_guard(_assert_mutex_tick);
            // Action can removed from the inside of callback.
            if (TickAction* action_modified = _tick_actions.get(to_execute))
            {
                (*action_modified)->_last_tick = clock::now();
            }
            return 1;
        }

        std::size_t work_count() const
        {
            // std::scoped_lock<> requires .try_lock() ?
            auto ticks_lock = std::lock_guard(_assert_mutex_tick);
            auto tasks_lock = std::lock_guard(_assert_mutex_tasks);
            return (_tick_actions.size() + _tasks.size());
        }

        struct Scheduler
        {
            using clock = EventLoop::clock;
            using clock_duration = EventLoop::clock_duration;
            using clock_point = EventLoop::clock_point;
            using ActionHandle = EventLoop::ActionHandle;
            using TaskHandle = EventLoop::TaskHandle;

            EventLoop* _event_loop = nullptr;

            template<typename F, typename State>
            ActionHandle tick_every(clock_point start_from, clock_duration period, F&& f, State&& state)
            {
                assert(_event_loop);
                return _event_loop->tick_every(
                    XRX_MOV(start_from)
                    , XRX_MOV(period)
                    , XRX_FWD(f)
                    , XRX_FWD(state));
            }

            bool tick_cancel(ActionHandle handle)
            {
                assert(_event_loop);
                return _event_loop->tick_cancel(handle);
            }

            template<typename F>
            TaskHandle task_post(F&& task)
            {
                assert(_event_loop);
                return _event_loop->task_post(XRX_FWD(task));
            }

            bool task_cancel(TaskHandle handle)
            {
                assert(_event_loop);
                return _event_loop->task_cancel(handle);
            }

            // Scheduler for which every task is ordered.
            // #TODO: looks like too big restriction.
            // on_next() can go in whatever order, but on_complete and on_error
            // should be last one.
            Scheduler stream_scheduler() { return *this; }
        };

        Scheduler scheduler()
        {
            return Scheduler{this};
        }
    };
} // namespace xrx::debug


