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
            using value_type   = Value;
            using error_type   = Error;
            using Unsubscriber = Subject_::Unsubsriber;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            Unsubsriber subscribe(Observer&& observer)
            {
                assert(_shared);
                Unsubsriber unsubscriber;
                unsubscriber._shared_weak = _shared;
                unsubscriber._handle = _shared->_subscriptions.push_back(
                    observer::make_complete(std::forward<Observer>(observer)));
                return unsubscriber;
            }
        };

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
            assert(_shared);
            _shared->_subscriptions.for_each([&v](AnyObserver<Value, Error>& observer)
            {
                observer.on_next(v);
            });
        }

        void on_error(Error e)
        {
            assert(_shared);
            _shared->_subscriptions.for_each([&e](AnyObserver<Value, Error>& observer)
            {
                observer.on_error(e);
            });
        }

        void on_completed()
        {
            assert(_shared);
            _shared->_subscriptions.for_each([](AnyObserver<Value, Error>& observer)
            {
                observer.on_completed();
            });
        }
    };
} // namespace xrx
