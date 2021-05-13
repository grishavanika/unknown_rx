#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"

#include <memory>
#include <cassert>

namespace xrx
{
    template<typename Value, typename Error>
    struct Subject_
    {
        using Subscriptions = detail::HandleVector<AnyObserver<Value, Error>>;
        using Handle = typename Subscriptions::Handle;

        struct SharedImpl_
        {
            [[no_unique_address]] debug::AssertMutex<> _assert_mutex;
            Subscriptions _subscriptions;
        };

        using value_type = Value;
        using error_type = Error;

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            std::weak_ptr<SharedImpl_> _shared_weak;
            Handle _handle;

            bool detach()
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
                    auto _ = std::lock_guard(shared->_assert_mutex);
                    return shared->_subscriptions.erase(_handle);
                }
                return false;
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit Subject_()
            : _shared(std::make_shared<SharedImpl_>())
        {
        }

        struct SourceObservable
        {
            using value_type   = Subject_::value_type;
            using error_type   = Subject_::error_type;
            using Unsubscriber = Subject_::Unsubscriber;

            std::weak_ptr<SharedImpl_> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            Unsubscriber subscribe(Observer&& observer)
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
                    AnyObserver<value_type, error_type> erased(std::forward<Observer>(observer));
                    auto _ = std::lock_guard(shared->_assert_mutex);
                    Unsubscriber unsubscriber;
                    unsubscriber._shared_weak = _shared_weak;
                    unsubscriber._handle = shared->_subscriptions.push_back(std::move(erased));
                    return unsubscriber;
                }
                return Unsubscriber();
            }

            auto fork() { return SourceObservable(_shared_weak); }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        Unsubscriber subscribe(Observer&& observer) const
        {
            return as_observable().subscribe(std::forward<Observer>(observer));
        }

        detail::Observable_<SourceObservable> as_observable() const
        {
            return Observable_<SourceObservable>(SourceObservable(_shared));
        }

        Subject_ as_observer() const
        {
            return Subject_(_shared);
        }

        void on_next(Value v)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer, Handle handle)
                {
                    bool do_unsubscribe = false;
                    {
                        auto _ = debug::ScopeUnlock(lock);
                        const auto action = observer.on_next(v);
                        do_unsubscribe = action._unsubscribe;
                    }
                    if (do_unsubscribe)
                    {
                        // Notice: we have mutex lock.
                        _shared->_subscriptions.erase(handle);
                    }
                });
            }
            // else: already completed
        }

        void on_error(Error e)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                    auto _ = debug::ScopeUnlock(lock);
                    observer.on_error(e);
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }

        void on_completed()
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                        auto _ = debug::ScopeUnlock(lock);
                        observer.on_completed();
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }
    };
} // namespace xrx