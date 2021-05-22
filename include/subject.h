#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"

#include <memory>
#include <cassert>

namespace xrx
{
    template<typename Value, typename Error = void>
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
                auto shared = std::exchange(_shared_weak, {}).lock();
                if (not shared)
                {
                    return false;
                }
                auto guard = std::lock_guard(shared->_assert_mutex);
                return shared->_subscriptions.erase(_handle);
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit Subject_()
            : _shared(std::make_shared<SharedImpl_>())
        {
        }

        explicit Subject_(std::shared_ptr<SharedImpl_> impl)
            : _shared(XRX_MOV(impl))
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
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
            {
                static_assert(not std::is_lvalue_reference_v<Observer>);
                if (auto shared = _shared_weak.lock(); shared)
                {
                    AnyObserver<value_type, error_type> erased(XRX_MOV(observer));
                    auto guard = std::lock_guard(shared->_assert_mutex);
                    Unsubscriber unsubscriber;
                    unsubscriber._shared_weak = _shared_weak;
                    unsubscriber._handle = shared->_subscriptions.push_back(XRX_MOV(erased));
                    return unsubscriber;
                }
                return Unsubscriber();
            }

            auto fork() { return SourceObservable(_shared_weak); }
        };

        using Observable = ::xrx::detail::Observable_<SourceObservable>;
        using Observer = Subject_;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            return as_observable().subscribe(XRX_MOV(observer));
        }

        Observable as_observable()
        {
            return ::xrx::detail::Observable_<SourceObservable>(SourceObservable(_shared));
        }

        Observer as_observer()
        {
            return Observer(_shared);
        }

        void on_next(XRX_RVALUE(value_type&&) v)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer, Handle handle)
                {
                    bool do_unsubscribe = false;
                    {
                        auto guard = debug::ScopeUnlock(lock);
                        const auto action = observer.on_next(value_type(v)); // copy.
                        do_unsubscribe = action._stop;
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

        template<typename... Es>
        void on_error(Es&&... errors)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                    auto guard = debug::ScopeUnlock(lock);
                    if constexpr (sizeof...(errors) == 0)
                    {
                        observer.on_error();
                    }
                    else
                    {
                        static_assert(sizeof...(errors) == 1);
                        observer.on_error(Es(errors)...); // copy.
                    }
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
                    auto guard = debug::ScopeUnlock(lock);
                    observer.on_completed();
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }
    };
} // namespace xrx
