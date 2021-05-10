#include "concepts_observer.h"
#include "utils_observers.h"
#include "subject.h"
#include "observable_interface.h"
#include "observables_factory.h"
#include "operators/operator_filter.h"
#include "operators/operator_transform.h"
#include "operators/operator_publish.h"
#include "operators/operator_interval.h"

#include <string>
#include <functional>
#include <thread>
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

    using ActionCallback = std::function<void()>;
    struct TickAction
    {
        clock_point _last_tick;
        clock_duration _period;
        ActionCallback _action;
    };
    using TickActions = HandleVector<TickAction>;
    using Handle = typename TickActions::Handle;
    TickActions _tick_actions;

    template<typename F>
    Handle tick_every(clock_point start_from, clock_duration period, F f)
    {
        Handle handle = _tick_actions.push_back(TickAction(
            start_from
            , clock_duration(period)
            , ActionCallback(std::move(f))));
        return handle;
    }

    bool tick_cancel(Handle handle)
    {
        return _tick_actions.erase(handle);
    }

    std::size_t poll_one()
    {
        clock_duration smallest = clock_duration::max();
        Handle to_execute;
        _tick_actions.for_each([&](TickAction& action, Handle handle)
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

    std::size_t actions_count() const { return _tick_actions.size(); }

    struct Scheduler
    {
        using clock = EventLoop::clock;
        using clock_duration = EventLoop::clock_duration;
        using clock_point = EventLoop::clock_point;
        using Handle = EventLoop::Handle;

        EventLoop* _event_loop = nullptr;

        template<typename F>
        Handle tick_every(clock_point start_from, clock_duration period, F f)
        {
            assert(_event_loop);
            return _event_loop->tick_every(
                std::move(start_from)
                , std::move(period)
                , std::move(f));
        }

        bool tick_cancel(Handle handle)
        {
            assert(_event_loop);
            return _event_loop->tick_cancel(handle);
        }
    };

    Scheduler scheduler()
    {
        return Scheduler{this};
    }
};

int main()
{
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
        while (event_loop.actions_count() > 0)
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
}
