#pragma once
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "xrx_prologue.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <atomic>

namespace xrx::observable
{
    namespace detail
    {
        template<typename Value, typename Error
            , typename Scheduler
            , typename DestinationObserver_>
        struct ObserveOnObserver_
        {
            // Shared state, so when cancel happens we can
            // terminate task that should-be-canceled,
            // but was already scheduled.
            // #TODO: can this be done without shared state
            // or, perhaps, without creating a task per each item ?
            // Maybe we can Scheduler to schedule whole source Observable ?
            // (This should minimize amount of state/overhead per item).
            struct Shared_
            {
                Scheduler _sheduler;
                DestinationObserver_ _target;
                // #TODO: nicer memory orders; sequential is not needed.
                std::atomic_bool _unsubscribed = false;
            };
            std::shared_ptr<Shared_> _shared;
            
            static std::shared_ptr<Shared_> make_state(
                XRX_RVALUE(Scheduler&&) scheduler
                , XRX_RVALUE(DestinationObserver_&&) target)
            {
                return std::make_shared<Shared_>(XRX_MOV(scheduler), XRX_MOV(target));
            }

            ::xrx::unsubscribe on_next(XRX_RVALUE(Value&&) v)
            {
                if (_shared->_unsubscribed)
                {
                    return ::xrx::unsubscribe(true);
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, v_ = XRX_MOV(v)]() mutable
                {
                    if (self->_unsubscribed)
                    {
                        return;
                    }
                    const auto action = ::xrx::detail::on_next_with_action(
                        self->_target, XRX_MOV(v_));
                    if (action._stop)
                    {
                        self->_unsubscribed = true;
                    }
                });
                (void)handle;
                return ::xrx::unsubscribe(false);
            }

            void on_error(XRX_RVALUE(Error&&) e)
            {
                if constexpr (::xrx::detail::ConceptWithOnError<DestinationObserver_, Error>)
                {
                    if (_shared->_unsubscribed)
                    {
                        return;
                    }
                    auto self = _shared;
                    const auto handle = self->_sheduler.task_post(
                        [self, e_ = XRX_MOV(e)]() mutable
                    {
                        if (not self->_unsubscribed)
                        {
                            (void)::xrx::detail::on_error(XRX_MOV(self->_target), XRX_MOV(e_));
                        }
                    });
                    (void)handle;
                }
                else
                {
                    // Observer does not actually supports errors,
                    // just don't schedule empty task.
                }
            }
            void on_completed()
            {
                if constexpr (::xrx::detail::ConceptWithOnCompleted<DestinationObserver_>)
                {
                    if (_shared->_unsubscribed)
                    {
                        return;
                    }
                    auto self = _shared;
                    const auto handle = self->_sheduler.task_post(
                        [self]()
                    {
                        if (not self->_unsubscribed)
                        {
                            (void)xrx::detail::on_completed(self->_target);
                        }
                    });
                    (void)handle;
                }
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct ObserveOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            // Even if `SourceObservable` is NOT Async, every item 
            // from it will be re-scheduled thru some abstract interface.
            // At the end of our .subscribe() we can't know at compile time
            // if all scheduled items where processed.
            using is_async = std::true_type;

            SourceObservable _source;
            // #TODO: define concept for `StreamScheduler`, see stream_scheduler().
            Scheduler _scheduler;

            auto fork() && { return ObserveOnObservable_(XRX_MOV(_source), XRX_MOV(_scheduler)); }
            auto fork() &  { return ObserveOnObservable_(_source.fork(), _scheduler); }

            struct Detach
            {
                using SourceDetach = typename SourceObservable::DetachHandle;
                using has_effect = std::true_type;

                SourceDetach _detach;
                // #TODO: does shared_ptr needed to be there.
                // Looks like weak should work in this case.
                std::shared_ptr<std::atomic_bool> _unsubscribed;

                bool operator()()
                {
                    auto unsubscribed = std::exchange(_unsubscribed, {});
                    if (unsubscribed)
                    {
                        *unsubscribed = true;
                        return std::exchange(_detach, {})();
                    }
                    return false;
                }
            };
            using DetachHandle = Detach;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                using ObserverImpl_ = ObserveOnObserver_<value_type, error_type, Scheduler, Observer>;
                auto shared = ObserverImpl_::make_state(XRX_MOV(_scheduler), XRX_MOV(observer));
                
                // #XXX: here, in theory, if `_source` Observable is Sync.
                // AND we know amount of items it can emit (not infinity),
                // we can cache all of them and then scheduler only _few_
                // tasks, each of which will emit several items per task.
                // (Instead of scheduling task per item).
                auto handle = XRX_MOV(_source).subscribe(ObserverImpl_(shared));

                DetachHandle detach;
                detach._detach = handle;
                // Share only stop flag with unsubscriber.
                detach._unsubscribed = {shared, &shared->_unsubscribed};
                return detach;
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::ObserveOn
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Scheduler&&) scheduler)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(scheduler);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Scheduler);
        using Impl = ::xrx::observable::detail::ObserveOnObservable_<SourceObservable, Scheduler>;
        // #TODO: add overload that directly accepts `StreamScheduler` so
        // client can pass more narrow interface.
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(scheduler).stream_scheduler()));
    }
} // namespace xrx::detail::operator_tag

#include "xrx_epilogue.h"
