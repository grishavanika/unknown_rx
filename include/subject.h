#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"

#include <mutex>
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
            std::mutex _mutex;
            Subscriptions _subscriptions;
        };

        using value_type = Value;
        using error_type = Error;

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // Nothing to unsubscribe from.
            std::weak_ptr<SharedImpl_> _shared_weak;
            Handle _handle;

            bool detach()
            {
                auto shared = std::exchange(_shared_weak, {}).lock();
                if (not shared)
                {
                    return false;
                }
                auto guard = std::lock_guard(shared->_mutex);
                return shared->_subscriptions.erase(_handle);
            }

            explicit operator bool() const
            {
                return (not _shared_weak.expired());
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

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // The stream is dangling; any new Observer will not get any event.
            std::weak_ptr<SharedImpl_> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                auto shared = _shared_weak.lock();
                if (not shared)
                {
                    // #XXX: this is also the case when we try to
                    // subscribe on the subject that is already completed.
                    // Shoul we assert ? What's the expected behavior ?
                    return Unsubscriber();
                }
                AnyObserver<value_type, error_type> erased(XRX_MOV(observer));
                auto guard = std::lock_guard(shared->_mutex);
                Unsubscriber unsubscriber;
                unsubscriber._shared_weak = _shared_weak;
                unsubscriber._handle = shared->_subscriptions.push_back(XRX_MOV(erased));
                return unsubscriber;
            }

            auto fork() && { return SourceObservable(XRX_MOV(_shared_weak)); }
            auto fork() &  { return SourceObservable(_shared_weak); }
        };

        using Observable = ::xrx::detail::Observable_<SourceObservable>;
        using Observer = Subject_;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        // This subscribe() is not ref-qualified (like the rest of Observables)
        // since it doesn't make sense for Subject<> use-case:
        // once subscribed, _same_ Subject instance is used to emit values.
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
        {
            XRX_CHECK_RVALUE(observer);
            return as_observable().subscribe(XRX_MOV(observer));
        }

        Observable as_observable()
        {
            return Observable(SourceObservable(_shared));
        }

        Observer as_observer()
        {
            return Observer(_shared);
        }

        void on_next(XRX_RVALUE(value_type&&) v)
        {
            // Note: on_next() and {on_error(), on_completed()} should not be called in-parallel.
            if (not _shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(_shared->_mutex);
            for (auto&& [observer, handle] : _shared->_subscriptions.iterate_with_handle())
            {
                const auto action = observer.on_next(value_type(v)); // copy.
                if (action._stop)
                {
                    // Notice: we have mutex lock.
                    _shared->_subscriptions.erase(handle);
                }
            }
            // Note: should we unsubscribe if there are no observers/subscriptions anymore ?
            // Seems like nope, someone can subscribe later.
        }

        template<typename... Es>
        void on_error(Es&&... errors)
        {
            // Note: on_completed() should not be called in-parallel.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(shared->_mutex);
            for (AnyObserver<Value, Error>& observer : shared->_subscriptions)
            {
                if constexpr (sizeof...(errors) == 0)
                {
                    observer.on_error();
                }
                else
                {
                    static_assert(sizeof...(errors) == 1);
                    observer.on_error(Es(errors)...); // copy.
                }
            }
        }

        void on_completed()
        {
            // Note: on_completed() should not be called in-parallel.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(shared->_mutex);
            for (AnyObserver<Value, Error>& observer : shared->_subscriptions)
            {
                observer.on_completed();
            }
        }
    };
} // namespace xrx
