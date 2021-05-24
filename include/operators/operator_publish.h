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
#include <mutex>
#include <optional>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct ConnectObservableState_
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;
        using AnyObserver_ = AnyObserver<value_type, error_type>;
        using Subscriptions = detail::HandleVector<AnyObserver_>;
        using Handle = typename Subscriptions::Handle;
        using SourceDetach = typename SourceObservable::DetachHandle;

        struct SharedImpl_;

        struct ConnectObserver_
        {
            std::shared_ptr<SharedImpl_> _shared;
            bool _do_refcount = false;

            void on_next(value_type&& v);
            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e);
            void on_completed();
        };

        struct Detach_Child
        {
            using has_effect = std::true_type;

            std::shared_ptr<SharedImpl_> _shared;
            Handle _handle;

            bool operator()(bool do_refcount = false);
        };

        struct Detach_Connect
        {
            using has_effect = std::true_type;

            std::shared_ptr<SharedImpl_> _shared;

            bool operator()();

            explicit operator bool() const
            {
                return !!_shared;
            }
        };

        struct Detach_RefCount
        {
            Detach_Child _child;

            bool operator()()
            {
                return std::exchange(_child, {})(true/*do_refcount*/);
            }
        };

        struct RefCountObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using is_async = IsAsyncObservable<SourceObservable>;
            using DetachHandle = Detach_RefCount;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
            auto subscribe(Observer&& observer) &&;

            auto fork() && { return RefCountObservable_(XRX_MOV(_shared)); }
            auto fork()  & { return RefCountObservable_(_shared); }
        };

        struct SharedImpl_ : public std::enable_shared_from_this<SharedImpl_>
        {
            std::recursive_mutex _mutex;
            SourceObservable _source;
            Subscriptions _subscriptions;
            std::optional<SourceDetach> _connected;

            explicit SharedImpl_(XRX_RVALUE(SourceObservable&&) source)
                : _mutex()
                , _source(XRX_MOV(source))
                , _subscriptions()
                , _connected(std::nullopt)
            {
            }

            Detach_Connect connect_once(bool do_refcount)
            {
                auto guard = std::unique_lock(_mutex);
                if (not _connected)
                {
                    auto self = this->shared_from_this();
                    _connected.emplace(_source.fork().subscribe(ConnectObserver_(self, do_refcount)));
                    return Detach_Connect(XRX_MOV(self));
                }
                return Detach_Connect();
            }

            bool disconnect_once()
            {
                auto guard = std::unique_lock(_mutex);
                if (_connected)
                {
                    return (*std::exchange(_connected, {}))();
                }
                return false;
            }

            template<typename Observer>
            Detach_Child subscribe_child(XRX_RVALUE(Observer&&) observer, bool do_refcount)
            {
                XRX_CHECK_RVALUE(observer);
                AnyObserver_ erased_observer(XRX_MOV(observer));
                auto guard = std::lock_guard(_mutex);
                std::size_t count_before = _subscriptions.size();
                auto handle = _subscriptions.push_back(XRX_MOV(erased_observer));
                if (do_refcount && (count_before == 0))
                {
                    (void)connect_once(do_refcount);
                }
                return Detach_Child(this->shared_from_this(), XRX_MOV(handle));
            }

            bool unsubscribe_child(Handle handle, bool do_refcount)
            {
                auto guard = std::lock_guard(_mutex);
                const std::size_t count_before = _subscriptions.size();
                const bool ok = _subscriptions.erase(handle);
                const std::size_t count_now = _subscriptions.size();
                if (do_refcount && (count_now == 0) && (count_before > 0))
                {
                    (void)disconnect_once();
                }
                return ok;
            }

            void on_next_impl(XRX_RVALUE(value_type&&) v, bool do_refcount)
            {
                auto guard = std::unique_lock(_mutex);
                for (auto&& [observer, handle] : _subscriptions.iterate_with_handle())
                {
                    const auto action = observer.on_next(value_type(v)); // copy.
                    if (action._stop)
                    {
                        // #XXX: when refcount is active AND child is last one,
                        // this will trigger disconnect_once() that will trigger
                        // our Observer's DetachHandle; that is self-destroy ?
                        // Just increase self-shared_ptr ref-count ?
                        unsubscribe_child(handle, do_refcount);
                    }
                }
            }
            template<typename... VoidOrError>
            void on_error_impl(XRX_RVALUE(VoidOrError&&)... e)
            {
                auto guard = std::unique_lock(_mutex);
                for (AnyObserver_& observer : _subscriptions)
                {
                    if constexpr ((sizeof...(e)) == 0)
                    {
                        observer.on_error();
                    }
                    else
                    {
                        observer.on_error(error_type(e)...); // copy error.
                    }
                }
            }
            void on_completed_impl()
            {
                auto guard = std::unique_lock(_mutex);
                for (AnyObserver_& observer : _subscriptions)
                {
                    observer.on_completed();
                }
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit ConnectObservableState_(XRX_RVALUE(SourceObservable&&) source)
            : _shared(std::make_shared<SharedImpl_>(XRX_MOV(source)))
        {
        }
    };

    template<typename SourceObservable>
    struct ConnectObservableSource_
    {
        using State_ = ConnectObservableState_<SourceObservable>;
        using SharedImpl_ = typename State_::SharedImpl_;
        using Detach_Child = typename State_::Detach_Child;
        using DetachHandle = Detach_Child;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;

        std::weak_ptr<SharedImpl_> _shared_weak;

        template<typename Observer>
        Detach_Child subscribe(XRX_RVALUE(Observer&&) observer)
        {
            XRX_CHECK_RVALUE(observer);
            if (auto shared = _shared_weak.lock(); shared)
            {
                return shared->subscribe_child(XRX_MOV(observer), false/*no refcount*/);
            }
            return Detach_Child();
        }

        ConnectObservableSource_ fork() { return ConnectObservableSource_(_shared_weak); }
    };

    template<typename SourceObservable
        , typename SharedState_ = ConnectObservableState_<SourceObservable>
        , typename ConnectObservableSource = ConnectObservableSource_<SourceObservable>
        , typename ConnectObservableInterface = Observable_<ConnectObservableSource>>
    struct ConnectObservable_
        : private SharedState_
        , public ConnectObservableInterface
    {
        using value_type = typename ConnectObservableInterface::value_type;
        using error_type = typename ConnectObservableInterface::error_type;
        using SharedImpl_ = typename SharedState_::SharedImpl_;
        using RefCountObservable_ = typename SharedState_::RefCountObservable_;
        using Detach_Connect = typename SharedState_::Detach_Connect;
        using DetachHandle = Detach_Connect;
        using RefCountObservableInterface = Observable_<RefCountObservable_>;

        XRX_FORCEINLINE() SharedState_& state() { return static_cast<SharedState_&>(*this); }

        explicit ConnectObservable_(XRX_RVALUE(SourceObservable&&) source)
            : SharedState_(XRX_MOV(source))
            , ConnectObservableInterface(ConnectObservableSource(state()._shared))
        {
        }

        Detach_Connect connect()
        {
            return state()._shared->connect_once(false/*no refcount*/);
        }

        RefCountObservableInterface ref_count()
        {
            return RefCountObservableInterface(RefCountObservable_(state()._shared));
        }
    };

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_next(XRX_RVALUE(value_type&&) v)
    {
        assert(_shared);
        _shared->on_next_impl(XRX_MOV(v), _do_refcount);
    }

    template<typename SourceObservable>
    template<typename... VoidOrError>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_error(XRX_RVALUE(VoidOrError&&)... e)
    {
        assert(_shared);
        _shared->on_error_impl(XRX_MOV(e)...);
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_completed()
    {
        assert(_shared);
        _shared->on_completed_impl();
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Detach_Child::operator()(bool do_refcount /*= false*/)
    {
        auto shared = std::exchange(_shared, {});
        if (shared)
        {
            return shared->unsubscribe_child(_handle, do_refcount);
        }
        return false;
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Detach_Connect::operator()()
    {
        auto shared = std::exchange(_shared, {});
        if (shared)
        {
            return shared->disconnect_once();
        }
        return false;
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_::subscribe(
        XRX_RVALUE(Observer&&) observer) &&
    {
        XRX_CHECK_RVALUE(observer);
        assert(_shared);
        return Detach_RefCount(_shared->subscribe_child(
            XRX_MOV(observer), true/*refcount*/));
    }

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Publish
        , XRX_RVALUE(SourceObservable&&) source)
            requires ConceptObservable<SourceObservable>
    {
        return ConnectObservable_<SourceObservable>(XRX_MOV(source));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto publish()
    {
        return [](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Publish()
                , XRX_MOV(source));
        };
    }
} // namespace xrx
