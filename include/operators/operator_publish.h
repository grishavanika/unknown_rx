#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "utils_containers.h"
#include "debug/assert_mutex.h"
#include "any_observer.h"
#include <type_traits>
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
        using AnyObserver_ = AnyObserver<value_type, error_type>;
        using Subscriptions = detail::HandleVector<AnyObserver_>;
        using Handle = typename Subscriptions::Handle;
        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

        struct SharedImpl_;

        struct Observer_
        {
            std::shared_ptr<SharedImpl_> _shared;
            bool _do_refcount = false;

            void on_next(value_type v);
            void on_error(error_type e);
            void on_completed();
        };

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            // #XXX: note strong reference, not weak one.
            // #TODO: add a test that makes sure we can't use weak there.
            std::shared_ptr<SharedImpl_> _shared;
            Handle _handle;

            bool detach(bool do_refcount = false);
        };

        struct RefCountUnsubscriber
        {
            Unsubscriber _unsubscriber;
            explicit RefCountUnsubscriber(Unsubscriber unsubscriber)
                : _unsubscriber(std::move(unsubscriber))
            {
            }

            bool detach()
            {
                const bool do_refcount = true;
                return _unsubscriber.detach(do_refcount);
            }
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

            [[no_unique_address]] debug::AssertMutex<> _assert_mutex;
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

            SourceUnsubscriber connect(bool do_refcount)
            {
                auto _ = std::lock_guard(_assert_mutex);
                if (not _connected)
                {
                    // #XXX: circular dependency ?
                    // `_source` that remembers Observer that takes strong reference to this.
                    auto unsubscriber = _source.fork().subscribe(
                        Observer_(this->shared_from_this(), do_refcount));
                    _connected = unsubscriber;
                    return unsubscriber;
                }
                return SourceUnsubscriber();
            }

            template<typename Observer>
            Unsubscriber subscribe(Observer&& observer, bool do_refcount = false)
            {
                std::size_t count_before = 0;
                AnyObserver_ erased_observer(std::forward<Observer>(observer));
                Unsubscriber unsubscriber;
                unsubscriber._shared = Base::shared_from_this();
                {
                    auto _ = std::lock_guard(_assert_mutex);
                    count_before = _subscriptions.size();
                    unsubscriber._handle = _subscriptions.push_back(std::move(erased_observer));
                }
                if (do_refcount && (count_before == 0))
                {
                    connect(do_refcount);
                }
                return unsubscriber;
            }

            bool unsubscribe(Handle handle, bool do_refcount)
            {
                auto _ = std::lock_guard(_assert_mutex);
                const std::size_t count_before = _subscriptions.size();
                const bool ok = _subscriptions.erase(handle);
                const std::size_t count_now = _subscriptions.size();
                if (do_refcount && (count_now == 0) && (count_before > 0))
                {
                    if (_connected)
                    {
                        _connected->detach();
                        _connected.reset();
                    }
                }
                return ok;
            }

            void on_next_impl(value_type v, bool do_refcount)
            {
                auto lock = std::unique_lock(_assert_mutex);
                _subscriptions.for_each([&](AnyObserver_& observer, Handle handle)
                {
                    auto _ = debug::ScopeUnlock(lock);
                    const auto action = observer.on_next(v);
                    if (action._unsubscribe)
                    {
                        unsubscribe(handle, do_refcount);
                    }
                });
                // #XXX: should we return there for caller's on_next() ?
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit ConnectObservableState_(SourceObservable source)
            : _shared(std::make_shared<SharedImpl_>(std::move(source)))
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
            const bool do_refcount = false;
            return state()._shared->connect(do_refcount);
        }

        auto ref_count()
        {
            using RefCountObservable_ = typename SharedState_::RefCountObservable_;
            return Observable_<RefCountObservable_>(RefCountObservable_(state()._shared));
        }
    };

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_next(value_type v)
    {
        _shared->on_next_impl(std::forward<value_type>(v), _do_refcount);
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_error(error_type e)
    {
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto _ = debug::ScopeUnlock(lock);
            observer.on_error(e);
        });
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_completed()
    {
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto _ = debug::ScopeUnlock(lock);
            observer.on_completed();
        });
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Unsubscriber::detach(bool do_refcount /*= false*/)
    {
        return _shared->unsubscribe(_handle, do_refcount);
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_
        ::subscribe(Observer&& observer) &&
    {
        const bool do_refcount = true;
        return RefCountUnsubscriber(_shared->subscribe(
            std::forward<Observer>(observer), do_refcount));
    }

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Publish
        , SourceObservable source)
    {
        return ConnectObservable_<SourceObservable>(std::move(source));
    }
} // namespace xrx::detail
