#include "concepts_observer.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "subject.h"
#include "observable_interface.h"
#include "observables_factory.h"
#include "operators/operator_filter.h"
#include "operators/operator_transform.h"
#include "operators/operator_publish.h"
#include "operators/operator_interval.h"
#include "operators/operator_create.h"
#include "operators/operator_subscribe_on.h"
#include "operators/operator_observe_on.h"

#include <string>
#include <functional>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cinttypes>

using namespace xrx;
using namespace detail;

struct InitialSourceObservable_
{
    using value_type = int;
    using error_type = none_tag;

    struct Unsubscriber
    {
        using has_effect = std::false_type;
        bool detach() { return false; }
    };

    template<typename Observer>
    Unsubscriber subscribe(Observer&& observer) &&
    {
        auto strict = observer::make_complete(std::forward<Observer>(observer));
        strict.on_next(1);
        strict.on_next(2);
        strict.on_next(3);
        strict.on_next(4);
        strict.on_completed();
        return Unsubscriber();
    }

    InitialSourceObservable_ fork() { return *this; }
};

// Not serious implementation. Just to build needed interface.
struct EventLoop
{
    using clock = std::chrono::steady_clock;
    using clock_duration = clock::duration;
    using clock_point = clock::time_point;

    using ActionCallback = std::function<void ()>;
    struct TickAction
    {
        clock_point _last_tick;
        clock_duration _period;
        ActionCallback _action;
    };
    using TickActions = HandleVector<TickAction>;
    using ActionHandle = typename TickActions::Handle;
    TickActions _tick_actions;

    using TaskCallback = std::function<void ()>;
    using Tasks = HandleVector<TaskCallback>;
    using TaskHandle = typename Tasks::Handle;
    Tasks _tasks;

    template<typename F>
    ActionHandle tick_every(clock_point start_from, clock_duration period, F f)
    {
        const ActionHandle handle = _tick_actions.push_back(TickAction(
            start_from
            , clock_duration(period)
            , ActionCallback(std::move(f))));
        return handle;
    }

    bool tick_cancel(ActionHandle handle)
    {
        return _tick_actions.erase(handle);
    }

    template<typename F>
    TaskHandle task_post(F task)
    {
        const TaskHandle handle = _tasks.push_back(TaskCallback(std::move(task)));
        return handle;
    }

    bool task_cancel(TaskHandle handle)
    {
        return _tasks.erase(handle);
    }

    std::size_t poll_one()
    {
        clock_duration smallest = clock_duration::max();
        ActionHandle to_execute;
        _tick_actions.for_each([&](TickAction& action, ActionHandle handle)
        {
            const clock_duration point = (action._last_tick + action._period).time_since_epoch();
            if (point < smallest)
            {
                smallest = point;
                to_execute = handle;
            }
        });
        if (not to_execute)
        {
            for (std::size_t i = 0; i < _tasks._values.size(); ++i)
            {
                const TaskHandle handle(_tasks._values[i]._version, std::uint32_t(i));
                if (handle)
                {
                    TaskCallback* callback = _tasks.get(handle);
                    assert(callback);
                    auto f = std::move(*callback);
                    _tasks.erase(handle);
                    std::move(f)();
                    return 1;
                }
            }
            return 0;
        }

        TickAction* action = _tick_actions.get(to_execute);
        assert(action);
        const clock_point desired_point = (action->_last_tick + action->_period);
        std::this_thread::sleep_until(desired_point); // no-op if time in the past.
        const clock_point now_point = clock::now();
        // We are single-threaded now, `action` can't be invalidated.
        assert(_tick_actions.get(to_execute));
        action->_action();
        // Action can invalidate item.
        if (TickAction* action_modified = _tick_actions.get(to_execute))
        {
            action_modified->_last_tick = clock::now();
        }
        return 1;
    }

    std::size_t work_count() const
    {
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

        template<typename F>
        ActionHandle tick_every(clock_point start_from, clock_duration period, F f)
        {
            assert(_event_loop);
            return _event_loop->tick_every(
                std::move(start_from)
                , std::move(period)
                , std::move(f));
        }

        bool tick_cancel(ActionHandle handle)
        {
            assert(_event_loop);
            return _event_loop->tick_cancel(handle);
        }

        template<typename F>
        TaskHandle task_post(F task)
        {
            assert(_event_loop);
            return _event_loop->task_post(std::move(task));
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

int main()
{
#if (0)
    // https://stackoverflow.com/questions/45051166/rxcpp-timeout-on-blocking-function
    // https://github.com/ReactiveX/RxCpp/issues/151
    InitialSourceObservable_ initial;
    using O = Observable_<InitialSourceObservable_>;
    O observable(std::move(initial));

    {
        auto unsubscriber = observable.fork().subscribe([](int value)
        {
            std::printf("Observer: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    {
        auto unsubscriber = observable.fork()
            .filter([](int)   { return true; })
            .filter([](int v) { return ((v % 2) == 0); })
            .filter([](int)   { return true; })
            .subscribe([](int value)
        {
            std::printf("Filtered even numbers: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    {
        Subject_<int, int> subject;
        using ExpectedUnsubscriber = typename decltype(subject)::Unsubscriber;

        subject.subscribe([](int) {});

        auto unsubscriber = subject.as_observable()
            .filter([](int)   { return true; })
            .filter([](int v) { return ((v % 2) == 0); })
            .filter([](int)   { return true; })
            .subscribe(observer::make([](int value)
        {
            std::printf("[Subject1] Filtered even numbers: %i.\n", value);
        }
            , []()
        {
            std::printf("[Subject1] Completed.\n");
        }));
        static_assert(std::is_same_v<decltype(unsubscriber), ExpectedUnsubscriber>);
        static_assert(ExpectedUnsubscriber::has_effect());

        subject.as_observable()
            .filter([](int v) { return (v == 3); })
            .transform([](int v) { return std::to_string(v); })
            .subscribe([](std::string v)
        {
            std::printf("[Subject2] Exactly 3: %s.\n", v.c_str());
        });

        subject.on_next(1);
        subject.on_next(2);

        unsubscriber.detach();

        subject.on_next(3);
        subject.on_next(4);
        subject.on_completed();
    }

    {
        Subject_<int, int> subject;
        auto unsubscriber = subject.as_observable().publish().ref_count().subscribe(
            [](int v)
        {
            printf("Shared: %i.\n", v);
        });
        subject.on_next(1);
        unsubscriber.detach();
        subject.on_next(2);
    }

#if (0)
    {
        EventLoop event_loop;
        auto unsubscriber = observable::interval(std::chrono::seconds(1), event_loop.scheduler())
            .filter([](std::uint64_t ticks)
            {
                return (ticks >= 0);
            })
            .subscribe([](std::uint64_t ticks)
            {
                printf("%" PRIu64 " ticks elapsed\n", ticks);
            });

        std::size_t executed_total = 0;
        while (event_loop.work_count() > 0)
        {
            const std::size_t executed_now = event_loop.poll_one();
            if (executed_now == 0)
            {
                std::this_thread::yield();
            }
            executed_total += executed_now;
            if (executed_total == 10)
            {
                unsubscriber.detach();
            }
        }
    }
#endif

    {
        EventLoop event_loop;
        auto intervals = observable::interval(std::chrono::seconds(1), event_loop.scheduler())
            .publish()
            .ref_count();
        using Unregister = decltype(intervals.fork().subscribe([](std::uint64_t ticks) {}));
        std::vector<Unregister> to_unregister;
        to_unregister.push_back(intervals.fork().subscribe([](std::uint64_t ticks)
        {
            printf("[0] %" PRIu64 " ticks elapsed\n", ticks);
        }));

        std::size_t executed_total = 0;
        while (event_loop.work_count() > 0)
        {
            const std::size_t executed_now = event_loop.poll_one();
            if (executed_now == 0)
            {
                std::this_thread::yield();
                continue;
            }
            executed_total += executed_now;
            if ((executed_total % 5) == 0)
            {
                const std::size_t index = to_unregister.size();
                to_unregister.push_back(intervals.fork().subscribe([=](std::uint64_t ticks)
                {
                    printf("[%i] %" PRIu64 " ticks elapsed\n", int(index), ticks);
                }));
            }

            if (executed_total == 100)
            {
                for (Unregister& unregister : to_unregister)
                {
                    unregister.detach();
                }
                to_unregister.clear();
            }
        }
    }
    {
        auto unsubscriber = observable::create<int, int>([](AnyObserver<int, int> observer)
        {
            observer.on_next(1);
            observer.on_next(2);
            observer.on_completed();
        })
            .filter([](int) { return true; })
            .subscribe([](int v) { printf("[create] %i\n", v); });
        unsubscriber.detach();
    }

    {
        EventLoop event_loop;
        auto unsubscriber = observable::create<std::uint64_t>([&event_loop](auto observer)
        {
            return observable::interval(std::chrono::seconds(1), event_loop.scheduler())
                .subscribe(std::move(observer));
        })
            .filter([](std::uint64_t) { return true; })
            .subscribe([](std::uint64_t v) { printf("[create with unsubscribe] %" PRIu64 "\n", v); });

        assert(event_loop.work_count() != 0);
        event_loop.poll_one();
        event_loop.poll_one();
        event_loop.poll_one();
        unsubscriber.detach();
        assert(event_loop.work_count() == 0);
    }
#endif

    {
        InitialSourceObservable_ initial;
        using O = Observable_<InitialSourceObservable_>;
        O observable(std::move(initial));

        std::cout << std::this_thread::get_id() << "[subscribe_on] start (main)" << "\n";

        EventLoop event_loop;
        auto unsubscribe = observable.fork()
            .subscribe_on(event_loop.scheduler())
            .observe_on(event_loop.scheduler())
            .subscribe([](int v)
        {
            std::cout << std::this_thread::get_id() << "[subscribe_on] (worker)" << " " << v << "\n";
        });
        // #XXX: not thread-safe to unsubscribe.
        (void)unsubscribe;

        std::thread worker([&]()
        {
            while (event_loop.work_count() > 0)
            {
                (void)event_loop.poll_one();
            }
        });
        worker.join();
    }
}
