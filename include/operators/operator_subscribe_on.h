#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include <utility>
#include <variant>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cassert>

namespace xrx::observable
{
    namespace detail
    {
        template<typename SourceObservable, typename Scheduler>
        struct SubscribeStateShared_
            : public std::enable_shared_from_this<SubscribeStateShared_<SourceObservable, Scheduler>>
        {
            using TaskHandle = typename Scheduler::TaskHandle;
            using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

            struct SubscribeInProgress
            {
                std::int32_t _waiting_unsubsribers = 0;
                std::mutex _mutex;
                std::condition_variable _on_finish;
                std::optional<SourceUnsubscriber> _unsubscriber;
            };

            struct SubscribeEnded
            {
                SourceUnsubscriber _unsubscriber;
            };

            struct Subscribed
            {
                std::variant<std::monostate
                    , SubscribeInProgress
                    , SubscribeEnded> _state;
            };

            std::mutex _mutex;
            Scheduler _scheduler;
            bool _try_cancel_subscribe = false;
            std::variant<std::monostate
                , TaskHandle
                , Subscribed> _unsubscribers;

            explicit SubscribeStateShared_(Scheduler sheduler)
                : _mutex()
                , _scheduler(std::move(sheduler))
                , _try_cancel_subscribe(false)
                , _unsubscribers()
            {
            }

            static std::shared_ptr<SubscribeStateShared_> make(Scheduler scheduler)
            {
                return std::make_shared<SubscribeStateShared_>(std::move(scheduler));
            }

            template<typename Observer>
                // requires ConceptValueObserverOf<Observer, value_type>
            void subscribe_impl(SourceObservable source, Observer observer
                , std::true_type) // source unsubscriber does have unsubscribe effect; remember it.
            {
                assert(std::get_if<std::monostate>(&_unsubscribers)); // Not yet initialized.
                // The trick there (I hope correct one) is that we remember weak reference
                // to our state. If at the time of task_post() execution there will be
                // no strong reference, this would mean there is no *active* Unsubscriber.
                // If there is no Unsubscriber, there is no need to remember anything.
                auto shared_weak = this->weak_from_this();

                const TaskHandle task_handle = _scheduler.task_post(
                    [self = shared_weak, source_ = std::move(source), observer_ = std::move(observer)]() mutable
                {
                    auto shared = self.lock(); // remember & extend lifetime.
                    SubscribeInProgress* in_progress = nullptr;
                    if (shared)
                    {
                        const std::lock_guard lock(shared->_mutex);
                        if (shared->_try_cancel_subscribe)
                        {
                            // Unsubscribe was requested just before task schedule.
                            // Do nothing & return.
                            return;
                        }

                        Subscribed* subscribed = nullptr;
                        if (auto* not_initialized = std::get_if<std::monostate>(&shared->_unsubscribers); not_initialized)
                        {
                            // We are first; task_post() executed before subscribe_impl() end.
                            subscribed = &shared->_unsubscribers.template emplace<Subscribed>();
                        }
                        else if (auto* task_handle = std::get_if<TaskHandle>(&shared->_unsubscribers); task_handle)
                        {
                            // We are second; task_post() executed *after* subscribe_impl() end.
                            // Invalidate task handle reference. We can't cancel that anymore.
                            subscribed = &shared->_unsubscribers.template emplace<Subscribed>();
                        }
                        else if (auto* already_subscribing = std::get_if<Subscribed>(&shared->_unsubscribers))
                        {
                            assert(false && "No-one should be able to subscribe to single state twice.");
                        }
                        if (subscribed)
                        {
                            in_progress = &subscribed->_state.template emplace<SubscribeInProgress>();
                        }
                    }

                    // #XXX: we should pass our own observer.
                    // And do check if unsubscribe happened while this
                    // .subscribe() is in progress.
                    SourceUnsubscriber unsubscriber = std::move(source_).subscribe(std::move(observer_));

                    if (in_progress)
                    {
                        assert(shared);
                        const std::lock_guard lock(shared->_mutex);

                        if (shared->_try_cancel_subscribe)
                        {
                            // Racy unsubscribe. Detach now, but also process & resume everyone waiting.
                            unsubscriber.detach();
                            unsubscriber = {};
                        }

                        if (in_progress->_waiting_unsubsribers > 0)
                        {
                            // We were too late. Can't destroy `SubscribeInProgress`.
                            const std::lock_guard lock2(in_progress->_mutex);
                            // Set the data everyone is waiting for.
                            in_progress->_unsubscriber.emplace(std::move(unsubscriber));
                            in_progress->_on_finish.notify_all();
                        }
                        else
                        {
                            // No-one is waiting yet. We can destroy temporary in-progress data
                            // and simply put final Unsubscriber.
                            Subscribed* subscribed = std::get_if<Subscribed>(&shared->_unsubscribers);
                            assert(subscribed && "No-one should change Subscribed state when it was already set");
                            subscribed->_state.template emplace<SubscribeEnded>(std::move(unsubscriber));
                        }
                    }
                });
                {
                    const std::lock_guard lock(_mutex);
                    if (auto* not_initialized = std::get_if<std::monostate>(&_unsubscribers); not_initialized)
                    {
                        _unsubscribers.template emplace<TaskHandle>(std::move(task_handle));
                    }
                    else if (auto* already_subscribing = std::get_if<Subscribed>(&_unsubscribers); already_subscribing)
                    {
                        // No need to remember `task_handle`. We already executed the task, nothing to cancel.
                        (void)task_handle;
                    }
                    else if (std::get_if<TaskHandle>(&_unsubscribers))
                    {
                        assert(false && "No-one should be able to subscribe to single state twice.");
                    }
                }
            }

            bool detach_impl()
            {
                std::unique_lock lock(_mutex);
                _try_cancel_subscribe = true;

                if (auto* not_initialized = std::get_if<std::monostate>(&_unsubscribers); not_initialized)
                {
                    assert(false && "Unexpected. Calling detach() without valid subscription state");
                    return false;
                }
                else if (auto* task_handle = std::get_if<TaskHandle>(&_unsubscribers); task_handle)
                {
                    // We are not yet in subscribe_impl() task, but still
                    // task cancel can be racy on the Scheduler implementation side.
                    // May subscribe be started. Handled on the subscribe_impl() side with
                    // `_try_cancel_subscribe`. Return value can be incorrect in this case.
                    // (We will unsubscribe for sure, but task is already executed and can't be canceled).
                    return _scheduler.task_cancel(*task_handle);
                }

                Subscribed* subscribed = std::get_if<Subscribed>(&_unsubscribers);
                assert(subscribed);
                if (auto* not_initialized = std::get_if<std::monostate>(&subscribed->_state); not_initialized)
                {
                    assert(false && "Unexpected. Subscribed state with invalid invariant.");
                    return false;
                }
                else if (auto* in_progress = std::get_if<SubscribeInProgress>(&subscribed->_state); in_progress)
                {
                    ++in_progress->_waiting_unsubsribers;
                    lock.unlock(); // Unlock main data.

                    std::optional<SourceUnsubscriber> unsubscriber;
                    {
                        std::unique_lock wait_lock(in_progress->_mutex);
                        in_progress->_on_finish.wait(wait_lock, [in_progress]()
                        {
                            return !!in_progress->_unsubscriber;
                        });
                        // Don't move unsubscriber, there may be multiple unsubscribers waiting.
                        unsubscriber = in_progress->_unsubscriber;
                    }
                    if (unsubscriber)
                    {
                        return unsubscriber->detach();
                    }
                    return false;
                }
                else if (auto* ended = std::get_if<SubscribeEnded>(&subscribed->_state); ended)
                {
                    return ended->_unsubscriber.detach();
                }

                assert(false);
                return false;
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct SubscribeOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Handle = typename Scheduler::TaskHandle;
            using StateShared_ = SubscribeStateShared_<SourceObservable, Scheduler>;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                std::shared_ptr<StateShared_> _shared;

                bool detach()
                {
                    if (_shared)
                    {
                        return _shared->detach_impl();
                    }
                    return false;
                }
            };

            SourceObservable _source;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer&& observer) &&
            {
                auto shared = StateShared_::make(std::move(_scheduler));
#if (0)
                // #TODO: implement subscribe_impl(..., std::false_type);
                using remember_source = typename StateShared_::SourceUnsubscriber::has_effect;
#else
                using remember_source = std::true_type;
#endif
                shared->subscribe_impl(std::move(_source), std::forward<Observer>(observer)
                    , remember_source());
                return Unsubscriber(shared);
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
