#pragma once
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
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
            // Needs to have shared state so destination Observer
            // can be only move_construcrible.
            // Also, on_next() and on_completed() calls need to target same Observer.
            // #TODO: don't do this if Observer is stateless.
            struct Shared_
            {
                Scheduler _sheduler;
                DestinationObserver_ _target;
                // #TODO: nicer memory orders; sequential is not needed.
                std::atomic_bool _unsubscribed = false;
            };
            std::shared_ptr<Shared_> _shared;
            
            static std::shared_ptr<Shared_> make_state(Scheduler scheduler, DestinationObserver_ target)
            {
                return std::make_shared<Shared_>(std::move(scheduler), std::move(target));
            }

            ::xrx::unsubscribe on_next(Value v)
            {
                if (_shared->_unsubscribed)
                {
                    return ::xrx::unsubscribe(true);
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, v_ = std::forward<Value>(v)]() mutable
                {
                    if (self->_unsubscribed)
                    {
                        return;
                    }
                    const auto action = ::xrx::detail::on_next_with_action(
                        self->_target.on_next(std::forward<Value>(v_)));
                    if (action._unsubscribe)
                    {
                        self->_unsubscribed = true;
                    }
                });
                (void)handle;
            }
            // #TODO: support void Error.
            void on_error(Error e)
            {
                if (_shared->_unsubscribed)
                {
                    return;
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, e_ = std::forward<Error>(e)]() mutable
                {
                    if (not self->_unsubscribed)
                    {
                        self->_target.on_error(std::forward<Error>(e_));
                    }
                });
                (void)handle;
            }
            void on_completed()
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
                        self->_target.on_completed();
                    }
                });
                (void)handle;
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct ObserveOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                SourceUnsubscriber _unsubscriber;
                // #TODO: does shared_ptr needed to be there.
                // Looks like weak should work in this case.
                std::shared_ptr<std::atomic_bool> _unsubscribed;

                bool detach()
                {
                    if (_unsubscribed)
                    {
                        *_unsubscribed = true;
                        return _unsubscriber.detach();
                    }
                    return false;
                }
            };

            SourceObservable _source;
            // #TODO: define concept for `StreamScheduler`, see stream_scheduler().
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                auto strict = xrx::observer::make_complete(std::move(observer));
                using CompleteObserver_ = decltype(strict);
                using Observer_ = ObserveOnObserver_<
                    value_type, error_type, Scheduler, CompleteObserver_>;
                auto shared = Observer_::make_state(std::move(_scheduler), std::move(strict));
                auto handle = std::move(_source).subscribe(Observer_(shared));
                Unsubscriber unsubscriber;
                unsubscriber._unsubscribed = std::shared_ptr<std::atomic_bool>(shared, &shared->_unsubscribed);
                unsubscriber._unsubscriber = handle;
                return unsubscriber;
            }

            auto fork() && { return ObserveOnObservable_(std::move(_source), std::move(_scheduler)); }
            auto fork() &  { return ObserveOnObservable_(_source.fork(), _scheduler); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::ObserveOn
        , SourceObservable source, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::ObserveOnObservable_<SourceObservable, Scheduler>;
        // #TODO: add overload that directly accepts `StreamScheduler` so
        // client can pass more narrow interface.
        auto stream = std::move(scheduler).stream_scheduler();
        return Observable_<Impl>(Impl(std::move(source), std::move(stream)));
    }
} // namespace xrx::detail::operator_tag
