#pragma once
#include "tag_invoke.hpp"
#include "meta_utils.h"
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include <utility>
#include <chrono>
#include <cstdint>

namespace xrx::observable
{
    namespace detail
    {
        template<typename Duration, typename Scheduler>
        struct IntervalObservable_
        {
            using value_type = std::uint64_t;
            using error_type = ::xrx::detail::none_tag;
            using clock = typename Scheduler::clock;
            using clock_duration = typename Scheduler::clock_duration;
            using clock_point = typename Scheduler::clock_point;
            using Handle = typename Scheduler::Handle;

            struct Unsubscriber
            {
                Handle _handle;
                Scheduler _scheduler;
                bool detach()
                {
                    return _scheduler.tick_cancel(_handle);
                }
            };

            Duration _period;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                const clock_duration period(_period);
                // To get zero tick at first.
                const clock_point start_from = clock::now() - period;
                const Handle handle = _scheduler.tick_every(start_from, period
                    , [ticks = value_type(0), observer_ = std::move(observer)]() mutable
                {
                    ::xrx::detail::on_next(observer_, ticks);
                    ++ticks;
                });
                return Unsubscriber(handle, _scheduler);
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Duration, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::Interval
        , Duration period, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::IntervalObservable_<Duration, Scheduler>;
        return Observable_<Impl>(Impl(std::move(period), std::move(scheduler)));
    }
} // namespace xrx::detail::operator_tag
