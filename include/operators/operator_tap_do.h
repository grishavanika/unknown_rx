#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_wrappers.h"
#include "observable_interface.h"
#include "xrx_prologue.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, typename Listener>
    struct TapOrDoObservable
    {
        SourceObservable _source;
        Listener _listener;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async   = IsAsyncObservable<SourceObservable>;
        using DetachHandle     = typename SourceObservable::DetachHandle;

        TapOrDoObservable fork() && { return TapOrDoObservable(XRX_MOV(_source), XRX_MOV(_listener)); }
        TapOrDoObservable fork()  & { return TapOrDoObservable(_source.fork(), _listener); }

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct ListenerObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Listener> _listener;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(value_type&&) v)
            {
                (void)::xrx::detail::on_next(_listener.get(), value_type(v)); // copy.
                return on_next_with_action(_observer.get(), XRX_MOV(v));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                (void)on_completed_optional(XRX_MOV(_listener.get()));
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(error_type&&) e)
            {
                (void)on_error_optional(XRX_MOV(_listener.get()), error_type(e));
                return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            return XRX_MOV(_source).subscribe(ListenerObserver<Observer>(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_listener)));
        }
    };

    template<typename SourceObservable, typename Observer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::TapOrDo
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(observer);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Observer);
        using Impl = TapOrDoObservable<SourceObservable, Observer>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(observer)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename Observer>
    auto tap(XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(observer);
        return [_observer = XRX_MOV(observer)](XRX_RVALUE(auto&&) source) mutable
        {
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::TapOrDo()
                , XRX_MOV(source), XRX_MOV(_observer));
        };
    }
} // namespace xrx

#include "xrx_epilogue.h"
