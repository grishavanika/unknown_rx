#pragma once
#include "tag_invoke.hpp"
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "utils_containers.h"
#include <utility>
#include <memory>
#include <optional>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct ConnectObservableState_
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using AnyObserver = AnyObserver<value_type, error_type>;
        using Subscriptions = detail::HandleVector<AnyObserver>;
        using Handle = typename Subscriptions::Handle;
        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

        struct SharedImpl_;

        struct Observer_
        {
            std::shared_ptr<SharedImpl_> _shared;

            void on_next(value_type v);
            void on_error(error_type e);
            void on_completed();
        };

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            // #XXX: note strong reference, not weak one.
            std::shared_ptr<SharedImpl_> _shared;
            Handle _handle;

            bool detach();
        };

        struct RefCountObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Unsubscriber = typename ConnectObservableState_::Unsubscriber;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
            auto subscribe(Observer&& observer) &&;

            auto fork()
            {
                return RefCountObservable_(_shared);
            }
        };

        struct SharedImpl_ : public std::enable_shared_from_this<SharedImpl_>
        {
            using Base = std::enable_shared_from_this<SharedImpl_>;

            SourceObservable _source;
            Subscriptions _subscriptions;
            std::optional<SourceUnsubscriber> _connected;

            explicit SharedImpl_(SourceObservable source)
                : Base()
                , _source(std::move(source))
                , _subscriptions()
                , _connected(std::nullopt)
            {
            }

            SourceUnsubscriber connect()
            {
                if (not _connected)
                {
                    // #XXX: reference cycle ? _source that remembers Observer
                    // that takes strong reference to this ?
                    auto unsubscriber = _source.fork().subscribe(
                        Observer_(Base::shared_from_this()));
                    _connected = unsubscriber;
                    return unsubscriber;
                }
                return SourceUnsubscriber();
            }

            template<typename Observer>
            Unsubscriber subscribe(Observer&& observer)
            {
                const std::size_t count_before = _subscriptions.size();
                Unsubscriber unsubscriber;
                unsubscriber._shared = Base::shared_from_this();
                unsubscriber._handle = _subscriptions.push_back(
                    observer::make_complete(std::forward<Observer>(observer)));
                if (count_before == 0)
                {
                    if (not _connected)
                    {
                        connect();
                    }
                }
                return unsubscriber;
            }

            bool unsubscribe(Handle handle)
            {
                const std::size_t count_before = _subscriptions.size();
                const bool ok = _subscriptions.erase(handle);
                const std::size_t count_now = _subscriptions.size();
                if ((count_now == 0) && (count_before > 0))
                {
                    if (_connected)
                    {
                        _connected->detach();
                        _connected.reset();
                    }
                }
                return ok;
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit ConnectObservableState_(SourceObservable source)
            : _shared(std::make_shared<SharedImpl_>(SharedImpl_(std::move(source))))
        {
        }
    };

    template<typename SourceObservable>
    struct ConnectObservableSource_
    {
        using State_ = ConnectObservableState_<SourceObservable>;
        using SharedImpl_ = typename State_::SharedImpl_;
        using Unsubscriber = typename State_::Unsubscriber;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;

        std::weak_ptr<SharedImpl_> _shared_weak;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer)
        {
            if (auto shared = _shared_weak.lock(); shared)
            {
                return shared->subscribe(std::forward<Observer>(observer));
            }
            return Unsubscriber();
        }

        ConnectObservableSource_ fork() { return ConnectObservableSource_(_shared_weak); }
    };

    template<typename SourceObservable
        , typename SharedState_ = ConnectObservableState_<SourceObservable>
        , typename Source_      = ConnectObservableSource_<SourceObservable>>
    struct ConnectObservable_
        : private SharedState_
        , public  Observable_<Source_>
    {
        using ObservableImpl_ = Observable_<Source_>;
        using value_type = typename ObservableImpl_::value_type;
        using error_type = typename ObservableImpl_::error_type;
        using SharedImpl_ = typename SharedState_::SharedImpl_;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SharedState_& state() { return static_cast<SharedState_&>(*this); }

        explicit ConnectObservable_(SourceObservable source)
            : SharedState_(std::move(source))
            , ObservableImpl_(Source_(state()._shared))
        {
        }

        Unsubscriber connect()
        {
            return state()._shared->connect();;
        }

        auto ref_count()
        {
            return Observable_<SharedState_::RefCountObservable_>(
                SharedState_::RefCountObservable_(state()._shared));
        }
    };

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_next(value_type v)
    {
        _shared->_subscriptions.for_each([&v](AnyObserver& observer)
        {
            observer.on_next(v);
        });
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_error(error_type e)
    {
        _shared->_subscriptions.for_each([&e](AnyObserver& observer)
        {
            observer.on_error(e);
        });
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_completed()
    {
        _shared->_subscriptions.for_each([](AnyObserver& observer)
        {
            observer.on_completed();
        });
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Unsubscriber::detach()
    {
        return _shared->unsubscribe(_handle);
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_
        ::subscribe(Observer&& observer) &&
    {
        return _shared->subscribe(std::forward<Observer>(observer));
    }

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Publish
        , SourceObservable source)
    {
        return ConnectObservable_<SourceObservable>(std::move(source));
    }
} // namespace xrx::detail
