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
        template<typename Value, typename Error
            , typename Scheduler
            , typename DestinationObserver_>
            // requires std::copy_constructible<DestinationObserver_>
        struct ObserveOnObserver_
        {
            // Needs to have shared state so 
            // `DestinationObserver_` can be only move_construcrible.
            // Also, on_next() and on_completed() calls need to target same Observer.
            struct Shared_
            {
                Scheduler _sheduler;
                DestinationObserver_ _target;
            };
            std::shared_ptr<Shared_> _shared;
            
            explicit ObserveOnObserver_(Scheduler scheduler, DestinationObserver_ target)
                : _shared(std::make_shared<Shared_>(Shared_(std::move(scheduler), std::move(target))))
            {
            }

            void on_next(Value v)
            {
                // #TODO: what if unsubscribe happened,
                // but some tasks are in flight ? Add `_cancel` flag ?
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, v_ = std::forward<Value>(v)]() mutable
                {
                    self->_target.on_next(std::forward<Value>(v_));
                });
                (void)handle;
            }
            void on_error(Error e)
            {
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, e_ = std::forward<Error>(e)]() mutable
                {
                    self->_target.on_error(std::forward<Error>(e_));
                });
                (void)handle;
            }
            void on_completed()
            {
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self]()
                {
                    self->_target.on_completed();
                });
                (void)handle;
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct ObserveOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Unsubscriber = typename SourceObservable::Unsubscriber;

            SourceObservable _source;
            Scheduler _scheduler; // stream scheduler

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                auto strict = xrx::observer::make_complete(std::move(observer));
                using CompleteObserver_ = decltype(strict);
                using Observer_ = ObserveOnObserver_<
                    value_type
                    , error_type
                    , Scheduler
                    , CompleteObserver_>;
                return std::move(_source).subscribe(
                    Observer_(std::move(_scheduler), std::move(strict)));
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
        auto stream = std::move(scheduler).stream_scheduler();
        return Observable_<Impl>(Impl(std::move(source), std::move(stream)));
    }
} // namespace xrx::detail::operator_tag
