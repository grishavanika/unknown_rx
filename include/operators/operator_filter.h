#pragma once
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_fast_FWD.h"
#include "utils_defines.h"
#include "utils_wrappers.h"
#include "observable_interface.h"
#include <utility>
#include <concepts>

namespace xrx::detail
{
    template<typename SourceObservable, typename Filter>
    struct FilterObservable
    {
        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

        SourceObservable _source;
        Filter _filter;

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct FilterObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Filter> _filter;

            XRX_FORCEINLINE() OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                if (_filter.get()(v))
                {
                    return ::xrx::detail::on_next_with_action(_observer.get(), XRX_MOV(v));
                }
                return OnNextAction();
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        FilterObservable fork() && { return FilterObservable(XRX_MOV(_source), XRX_MOV(_filter)); }
        FilterObservable fork() &  { return FilterObservable(_source.fork(), _filter); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        auto subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            return XRX_MOV(_source).subscribe(FilterObserver<Observer>(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_filter)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) filter)
            requires ConceptObservable<SourceObservable>
                && std::predicate<F, typename SourceObservable::value_type>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(filter);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(F);
        using Impl = FilterObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(filter)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename F>
    auto filter(XRX_RVALUE(F&&) f)
    {
        XRX_CHECK_RVALUE(f);
        return [_f = XRX_MOV(f)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Filter()
                , XRX_MOV(source), XRX_MOV(_f));
        };
    }
} // namespace xrx
