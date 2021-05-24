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

#include "simple_scheduler_event_loop.h"

#include <string>
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
    using DetachHandle = NoopDetach;

    template<typename Observer>
    DetachHandle subscribe(Observer&& observer) &&
    {
        // #XXX: handle unsubscribe.
        (void)::xrx::detail::on_next(observer, 1);
        (void)::xrx::detail::on_next(observer, 2);
        (void)::xrx::detail::on_next(observer, 3);
        (void)::xrx::detail::on_next(observer, 4);
        (void)::xrx::detail::on_completed_optional(std::move(observer));
        return DetachHandle();
    }

    InitialSourceObservable_ fork() { return *this; }
};

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    const int status = RUN_ALL_TESTS();
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
        using Unsubscriber = typename decltype(unsubscriber);
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::DetachHandle>);
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
        using Unsubscriber = typename decltype(unsubscriber);
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::DetachHandle>);
        static_assert(not Unsubscriber::has_effect());
    }

    {
        Subject_<int, int> subject;
        using ExpectedUnsubscriber = typename decltype(subject)::DetachHandle;

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

        unsubscriber();

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
        unsubscriber();
        subject.on_next(2);
    }

    {
        debug::EventLoop event_loop;
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
                unsubscriber();
            }
        }
    }

    {
        debug::EventLoop event_loop;
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
                    unregister();
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
        unsubscriber();
    }

    {
        debug::EventLoop event_loop;
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
        unsubscriber();
        assert(event_loop.work_count() == 0);
    }

    {
        InitialSourceObservable_ temp;
        using O = Observable_<InitialSourceObservable_>;
        O cold(std::move(temp));

        std::cout << std::this_thread::get_id() << "[subscribe_on] start (main)" << "\n";

        debug::EventLoop event_loop;
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
