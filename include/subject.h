#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "debug/assert_mutex.h"

#include <variant>
#include <mutex>
#include <memory>
#include <cassert>

namespace xrx
{
    namespace detail
    {
        template<typename ToAvoidVoid>
        struct OnErrorWithValue_ { ToAvoidVoid _error; };
        template<>
        struct OnErrorWithValue_<void> { };
    } // namespace detail

    template<typename Value, typename Error = void>
    struct Subject_
    {
        using Subscriptions = detail::HandleVector<AnyObserver<Value, Error>>;
        using Handle = typename Subscriptions::Handle;

        struct InProgress_ {};
        struct OnCompleted_ {};
        using OnErrorWithValue_ = detail::OnErrorWithValue_<Error>;

        using State = std::variant<InProgress_
            , OnCompleted_
            , OnErrorWithValue_>;

        struct SharedImpl_
        {
            std::mutex _mutex;
            Subscriptions _subscriptions;
            State _state;
        };

        using value_type = Value;
        using error_type = Error;

        struct Detach
        {
            using has_effect = std::true_type;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // Nothing to unsubscribe from.
            std::weak_ptr<SharedImpl_> _shared_weak;
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
            using DetachHandle = Subject_::DetachHandle;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // The stream is dangling; any new Observer will not get any event.
            std::weak_ptr<SharedImpl_> _shared_weak;

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
                else if (auto* error_copy = std::get_if<OnErrorWithValue_>(&shared->_state))
                {
                    // on_error() was already called.
                    // Finalize this Observer.
                    if constexpr (std::is_same_v<Error, void>)
                    {
                        detail::on_error_optional(XRX_MOV(observer));
                    }
                    else
                    {
                        detail::on_error_optional(XRX_MOV(observer), Error(error_copy->_error));
                    }
                    return DetachHandle();
                }
                assert(std::get_if<InProgress_>(&shared->_state));

                AnyObserver<value_type, error_type> erased(XRX_MOV(observer));
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

        template<typename... Es>
             requires ((std::is_same_v<Error, void> && (sizeof...(Es) == 0))
                || (not std::is_same_v<Error, void> && (sizeof...(Es) == 1)))
        void on_error(Es&&... errors)
        {
            auto lock = std::unique_lock(_shared->_mutex);
            if (not std::get_if<InProgress_>(&_shared->_state))
            {
                // Double-call to on_error() or call to on_error() after on_completed().
                return;
            }
            if constexpr (sizeof...(Es) == 0)
            {
                _shared->_state.template emplace<OnErrorWithValue_>();
                for (AnyObserver<Value, Error>& observer : _shared->_subscriptions)
                {
                    observer.on_error();
                }
            }
            else
            {
                const Error& error = _shared->_state.template emplace<OnErrorWithValue_>(XRX_FWD(errors)...)._error;
                for (AnyObserver<Value, Error>& observer : _shared->_subscriptions)
                {
                    observer.on_error(Error(error)); // copy.
                }
                // Clean-up everything, not needed anymore.
                _shared->_subscriptions = {};
            }
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
            for (AnyObserver<Value, Error>& observer : _shared->_subscriptions)
            {
                observer.on_completed();
            }
            // Clean-up everything, not needed anymore.
            _shared->_subscriptions = {};
        }
    };
} // namespace xrx
