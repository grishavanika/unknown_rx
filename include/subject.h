#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"
#include "xrx_prologue.h"

#include <variant>
#include <mutex>
#include <memory>
#include <cassert>

namespace xrx
{
    template<typename Value, typename Error = void_
        , typename Alloc = std::allocator<void>>
    struct Subject_
    {
        using allocator_type = Alloc;
        using value_type = Value;
        using error_type = Error;
        using AnyObserver_ = AnyObserver<Value, Error, Alloc>;
        using SubscriptionAlloc = std::allocator_traits<Alloc>::template rebind_alloc<AnyObserver_>;
        using Subscriptions = detail::HandleVector<AnyObserver_, SubscriptionAlloc>;
        using Handle = typename Subscriptions::Handle;

        struct InProgress_ {};
        struct OnCompleted_ {};
        struct OnError_ { Error _error; };

        using State = std::variant<InProgress_
            , OnCompleted_
            , OnError_>;

        struct SharedState
        {
            [[no_unique_address]] Alloc _alloc;
            std::mutex _mutex;
            Subscriptions _subscriptions;
            State _state;

            explicit SharedState(const Alloc& alloc = {})
                : _alloc(alloc)
                , _mutex()
                , _subscriptions(SubscriptionAlloc(alloc))
                , _state()
            {
            }
        };

        using SharedAlloc = std::allocator_traits<Alloc>::template rebind_alloc<SharedState>;

        struct Detach
        {
            using has_effect = std::true_type;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // Nothing to unsubscribe from.
            std::weak_ptr<SharedState> _shared_weak;
            Handle _handle;

            bool operator()()
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
        using DetachHandle = Detach;

        std::shared_ptr<SharedState> _shared;

        explicit Subject_()
            : Subject_(Alloc())
        {
        }

        explicit Subject_(const Alloc& alloc)
            : _shared(std::allocate_shared<SharedState>(
                  SharedAlloc(alloc) // for allocate_shared<>.
                , alloc)) // for SharedState.
        {
        }

        struct SourceObservable
        {
            using value_type   = Subject_::value_type;
            using error_type   = Subject_::error_type;
            using DetachHandle = Subject_::DetachHandle;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // The stream is dangling; any new Observer will not get any event.
            std::weak_ptr<SharedState> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                auto shared = _shared_weak.lock();
                if (not shared)
                {
                    // No Producer/Subject is alive anymore. Can't have any values emitted.
                    return DetachHandle();
                }

                auto guard = std::lock_guard(shared->_mutex);
                if (std::get_if<OnCompleted_>(&shared->_state))
                {
                    // on_completed() was already called.
                    // Finalize this Observer.
                    detail::on_completed_optional(XRX_MOV(observer));
                    return DetachHandle();
                }
                else if (auto* error_copy = std::get_if<OnError_>(&shared->_state))
                {
                    // on_error() was already called.
                    // Finalize this Observer.
                    detail::on_error_optional(XRX_MOV(observer), Error(error_copy->_error));
                    return DetachHandle();
                }
                assert(std::get_if<InProgress_>(&shared->_state));

                AnyObserver_ erased(XRX_MOV(observer), shared->_alloc);
                DetachHandle detach;
                detach._shared_weak = _shared_weak;
                detach._handle = shared->_subscriptions.push_back(XRX_MOV(erased));
                return detach;
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
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer)
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
            auto lock = std::unique_lock(_shared->_mutex);
            if (not std::get_if<InProgress_>(&_shared->_state))
            {
                // We are already finalized, nothing to do there.
                return;
            }

            for (auto&& [observer, handle] : _shared->_subscriptions.iterate_with_handle())
            {
                const auto action = observer.on_next(value_type(v)); // copy.
                if (action._stop)
                {
                    _shared->_subscriptions.erase(handle);
                }
            }
        }

        void on_error(XRX_RVALUE(error_type&&) e)
        {
            auto lock = std::unique_lock(_shared->_mutex);
            if (not std::get_if<InProgress_>(&_shared->_state))
            {
                // Double-call to on_error() or call to on_error() after on_completed().
                return;
            }
            const error_type& error = _shared->_state.template emplace<OnError_>(XRX_MOV(e))._error;
            for (AnyObserver_& observer : _shared->_subscriptions)
            {
                observer.on_error(error_type(error)); // copy.
            }
            // Clean-up everything, not needed anymore.
            _shared->_subscriptions = {};
        }

        void on_completed()
        {
            auto lock = std::unique_lock(_shared->_mutex);
            if (not std::get_if<InProgress_>(&_shared->_state))
            {
                // Double-call to on_completed() or call to on_completed() after on_error().
                return;
            }
            _shared->_state.template emplace<OnCompleted_>();
            for (AnyObserver_& observer : _shared->_subscriptions)
            {
                observer.on_completed();
            }
            // Clean-up everything, not needed anymore.
            _shared->_subscriptions = {};
        }

    private:
        explicit Subject_(std::shared_ptr<SharedState> impl)
            : _shared(XRX_MOV(impl))
        {
        }
    };
} // namespace xrx

#include "xrx_epilogue.h"
