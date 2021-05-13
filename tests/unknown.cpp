#include "concepts_observer.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "subject.h"
#include "debug/assert_mutex.h"
#include "observable_interface.h"
#include "observables_factory.h"
#include "operators/operator_filter.h"
#include "operators/operator_transform.h"
#include "operators/operator_publish.h"
#include "operators/operator_interval.h"
#include "operators/operator_create.h"
#include "operators/operator_subscribe_on.h"
#include "operators/operator_observe_on.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"

#include <string>
#include <functional>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cinttypes>

using namespace xrx;
using namespace detail;

#if (1)
struct Allocs
{
    std::size_t _count = 0;
    std::size_t _size = 0;

    static Allocs& get()
    {
        static Allocs o;
        return o;
    }
};

void* operator new(std::size_t count)
{
    auto& x = Allocs::get();
    ++x._count;
    ++x._size += count;
    return malloc(count);
}

void* operator new[](std::size_t count)
{
    auto& x = Allocs::get();
    ++x._count;
    ++x._size += count;
    return malloc(count);
}

void operator delete(void* ptr) noexcept
{
    free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    free(ptr);
}
#endif

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
        // #XXX: handle unsubscribe.
        (void)::xrx::detail::on_next(observer, 1);
        (void)::xrx::detail::on_next(observer, 2);
        (void)::xrx::detail::on_next(observer, 3);
        (void)::xrx::detail::on_next(observer, 4);
        (void)::xrx::detail::on_completed_optional(std::move(observer));
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

        explicit ActionCallback_(F f, State state)
            : _f(std::move(f))
            , _state(std::move(state))
        {
        }

        virtual bool invoke() override
        {
            return _f(_state);
        }
    };
    // Need to be able to copy when executing to release a lock, hence shared_ptr.
    using TickAction = std::shared_ptr<IActionCallback>;

    template<typename F, typename State>
    static std::shared_ptr<IActionCallback> make_action(
        clock_point start_from, clock_duration period
        , F f, State state)
    {
        auto shared = std::make_shared<ActionCallback_<F, State>>(std::move(f), std::move(state));
        shared->_start_from = start_from;
        shared->_last_tick = {};
        shared->_period = period;
        return shared;
    }

    using TickActions = HandleVector<TickAction>;
    using ActionHandle = typename TickActions::Handle;
    TickActions _tick_actions;
    mutable debug::AssertMutex<> _assert_mutex_tick;

    using TaskCallback = std::function<void ()>;
    using Tasks = HandleVector<TaskCallback>;
    using TaskHandle = typename Tasks::Handle;
    Tasks _tasks;
    mutable debug::AssertMutex<> _assert_mutex_tasks;

    template<typename F, typename State>
        requires std::convertible_to<std::invoke_result_t<F, State&>, bool>
    ActionHandle tick_every(clock_point start_from, clock_duration period, F f, State state)
    {
        auto _ = std::lock_guard(_assert_mutex_tick);
        const ActionHandle handle = _tick_actions.push_back(make_action(
            start_from
            , clock_duration(period)
            , std::move(f)
            , std::move(state)));
        return handle;
    }

    bool tick_cancel(ActionHandle handle)
    {
        auto _ = std::lock_guard(_assert_mutex_tick);
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

    std::size_t poll_one_task()
    {
        TaskCallback task;
        {
            auto _ = std::lock_guard(_assert_mutex_tasks);
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
                    task = std::move(*callback);
                    _tasks.erase(handle);
                    break; // find & execute first task.
                }
            }
        }
        if (task)
        {
            std::move(task)();
            return 1;
        }
        return 0;
    }

    std::size_t poll_one()
    {
        ActionHandle to_execute;
        {
            auto _ = std::lock_guard(_assert_mutex_tick);
            clock_duration smallest = clock_duration::max();
            _tick_actions.for_each([&](TickAction& action, ActionHandle handle)
            {
                const clock_duration point = (action->_last_tick + action->_period).time_since_epoch();
                if (point < smallest)
                {
                    smallest = point;
                    to_execute = handle;
                }
            });
        }
        if (not to_execute)
        {
            return poll_one_task();
        }

        clock_point desired_point;
        {
            auto _ = std::lock_guard(_assert_mutex_tick);
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
            auto _ = std::lock_guard(_assert_mutex_tick);
            if (TickAction* action = _tick_actions.get(to_execute))
            {
                invoke_action = *action; // copy.
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
            assert(removed);
            return 1;
        }
        auto _ = std::lock_guard(_assert_mutex_tick);
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
        ActionHandle tick_every(clock_point start_from, clock_duration period, F f, State state)
        {
            assert(_event_loop);
            return _event_loop->tick_every(
                std::move(start_from)
                , std::move(period)
                , std::move(f)
                , std::move(state));
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

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    const int status = RUN_ALL_TESTS();
#if (1)
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

    {
        EventLoop event_loop;
        auto unsubscriber = observable::interval(std::chrono::milliseconds(100), event_loop.scheduler())
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

    {
        EventLoop event_loop;
        auto intervals = observable::interval(std::chrono::milliseconds(100), event_loop.scheduler())
            .publish()
            .ref_count();
        using Unregister = decltype(intervals.fork().subscribe([](std::uint64_t) {}));
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

            if (executed_total == 40)
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
            // #TODO: need to handle return action.
            (void)observer.on_next(1);
            (void)observer.on_next(2);
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

    {
        InitialSourceObservable_ temp;
        using O = Observable_<InitialSourceObservable_>;
        O cold(std::move(temp));

        std::cout << std::this_thread::get_id() << "[subscribe_on] start (main)" << "\n";

        EventLoop event_loop;
        auto unsubscribe = cold.fork()
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

#if (0)
    Allocs::get() = Allocs{};
#endif
    observable::range(1)
        .take(10)
        .subscribe([](auto v)
    {
        printf("[take] %i\n", v);
    });
#if (0)
    std::cout << "Allocations count: " << Allocs::get()._count << "\n";
    std::cout << "Allocations size: " << Allocs::get()._size << "\n";
#endif
    observable::range(0)
        .transform([](int v) { return float(v); })
        .filter([](float) { return true; })
        .take(0)
        .subscribe(observer::make(
            [](float v)
    {
        printf("[stop on filter & tranform] %f\n", v);
    }
            , []()
    {
        printf("[stop on filter & transform] completed\n");
    }));
#endif

    return status;
}
