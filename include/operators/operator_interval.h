#pragma once
#include "meta_utils.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "xrx_prologue.h"
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
            using error_type = void_;
            using clock = typename Scheduler::clock;
            using clock_duration = typename Scheduler::clock_duration;
            using clock_point = typename Scheduler::clock_point;
            using Handle = typename Scheduler::ActionHandle;

            struct Detach
            {
                using has_effect = std::true_type;

                Handle _handle;
                Scheduler _scheduler;

                bool operator()()
                {
                    auto handle = std::exchange(_handle, {});
                    if (handle)
                    {
                        return _scheduler.tick_cancel(handle);
                    }
                    return false;
                }
            };
            using DetachHandle = Detach;

            Duration _period;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                XRX_CHECK_TYPE_NOT_REF(Observer);

                const clock_duration period(_period);
                // To get zero tick at first. #TODO: revisit this logic.
                const clock_point start_from = (clock::now() - period);
                
                struct State
                {
                    Observer _observer;
                    value_type _ticks = 0;
                };

                const Handle handle = _scheduler.tick_every(start_from, period
                    , [](State& state) mutable
                {
                    const value_type now = state._ticks++;
                    const auto action = ::xrx::detail::on_next_with_action(state._observer, value_type(now));
                    return action._stop;
                }
                    , State(XRX_MOV(observer), value_type(0)));

                return DetachHandle(handle, XRX_MOV(_scheduler));
            }

            auto fork() && { return IntervalObservable_(XRX_MOV(_period), XRX_MOV(_scheduler)); }
            auto fork() &  { return IntervalObservable_(_period, _scheduler); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Duration, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::Interval
        , Duration period, XRX_RVALUE(Scheduler&&) scheduler)
    {
        XRX_CHECK_RVALUE(scheduler);
        XRX_CHECK_TYPE_NOT_REF(Scheduler);
        using Impl = ::xrx::observable::detail::IntervalObservable_<Duration, Scheduler>;
        return Observable_<Impl>(Impl(XRX_MOV(period), XRX_MOV(scheduler)));
    }
} // namespace xrx::detail::operator_tag

#include "xrx_epilogue.h"
