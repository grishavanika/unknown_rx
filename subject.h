#pragma once
#include "any_observer.h"
#include "utils_containers.h"
#include "observable_interface.h"

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
            Subscriptions _subscriptions;
        };

        using value_type = Value;
        using error_type = Error;

        struct Unsubsriber
        {
            using has_effect = std::true_type;

            std::weak_ptr<SharedImpl_> _shared_weak;
            Handle _handle;

            bool detach()
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
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
            using Unsubscriber = Subject_::Unsubsriber;

            std::weak_ptr<SharedImpl_> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            Unsubsriber subscribe(Observer&& observer)
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
                    Unsubsriber unsubscriber;
                    unsubscriber._shared_weak = _shared_weak;
                    unsubscriber._handle = shared->_subscriptions.push_back(
                        observer::make_complete(std::forward<Observer>(observer)));
                    return unsubscriber;
                }
                return Unsubsriber();
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        Unsubsriber subscribe(Observer&& observer) const
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
                _shared->_subscriptions.for_each([&v](AnyObserver<Value, Error>& observer)
                {
                    observer.on_next(v);
                });
            }
            // else: already completed
        }

        void on_error(Error e)
        {
            if (_shared)
            {
                _shared->_subscriptions.for_each([&e](AnyObserver<Value, Error>& observer)
                {
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
                _shared->_subscriptions.for_each([](AnyObserver<Value, Error>& observer)
                {
                    observer.on_completed();
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }
    };
} // namespace xrx
