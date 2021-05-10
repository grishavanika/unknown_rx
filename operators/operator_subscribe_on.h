#pragma once
#include "tag_invoke.hpp"
#include "meta_utils.h"
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include <utility>

namespace xrx::observable
{
    namespace detail
    {
        template<typename SourceObservable, typename Scheduler>
        struct SubscribeOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Handle = typename Scheduler::TaskHandle;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                Handle _handle;
                Scheduler _scheduler;

                bool detach()
                {
                    return _scheduler.tick_cancel(_handle);
                }
            };

            SourceObservable _source;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                const Handle handle = _scheduler.task_post(
                    [source_ = std::move(_source), observer_ = std::move(observer)]() mutable
                {
                    return std::move(source_).subscribe(std::move(observer_));
                });
                return Unsubscriber(handle, std::move(_scheduler));
            }

            auto fork()
            {
                return SubscribeOnObservable_(_source.fork(), _scheduler);
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::SubscribeOn
        , SourceObservable source, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::SubscribeOnObservable_<SourceObservable, Scheduler>;
        return Observable_<Impl>(Impl(std::move(source), std::move(scheduler)));
    }
} // namespace xrx::detail::operator_tag
