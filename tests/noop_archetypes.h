#pragma once
#include <type_traits>
#include <chrono>

#include "concepts_observable.h"
#include "concepts_observer.h"

namespace tests
{
    struct Noop_Unsubscriber
    {
        using has_effect = std::true_type;
        constexpr bool detach() const noexcept { return false; }
    };
    static_assert(::xrx::detail::ConceptUnsubscriber<Noop_Unsubscriber>);

    template<typename V, typename E>
    struct Noop_Observable
    {
        using value_type = V;
        using error_type = E;
        using Unsubscriber = Noop_Unsubscriber;
    
        template<typename O>
        Unsubscriber subscribe(O) { return Unsubscriber(); }
        auto fork() { return *this; }
    };
    static_assert(::xrx::detail::ConceptObservable<Noop_Observable<int, int>>);

    template<typename V, typename E>
    struct Noop_Observer
    {
        void on_next(V) {}
        void on_error(E) {}
        void on_completed() {}
    };
    static_assert(::xrx::ConceptObserverOf<Noop_Observer<int, int>, int, int>);

    struct Noop_Scheduler
    {
        using clock = std::chrono::steady_clock;
        using clock_duration = clock::duration;
        using clock_point = clock::time_point;

        using ActionHandle = int;
        using TaskHandle = int;

        template<typename F>
        ActionHandle tick_every(clock_point, clock_duration, F)
        {
            return ActionHandle();
        }

        bool tick_cancel(ActionHandle)
        {
            return false;
        }

        template<typename F>
        TaskHandle task_post(F)
        {
            return TaskHandle();
        }

        bool task_cancel(TaskHandle)
        {
            return false;
        }

        Noop_Scheduler stream_scheduler() { return *this; }
    };

} // namespace tests
