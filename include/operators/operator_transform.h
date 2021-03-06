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
    template<typename SourceObservable, typename Transform>
    struct TransformObservable
    {
        SourceObservable _source;
        Transform _transform;

        using source_type  = typename SourceObservable::value_type;
        using value_type   = decltype(_transform(std::declval<source_type>()));
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

        static_assert(not std::is_reference_v<value_type>);

        TransformObservable fork() && { return TransformObservable(XRX_MOV(_source), XRX_MOV(_transform)); }
        TransformObservable fork()  & { return TransformObservable(_source.fork(), _transform); }

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct TransformObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Transform> _transform;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(source_type&&) v)
            {
                return on_next_with_action(_observer.get()
                    , _transform.get()(XRX_MOV(v)));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(error_type&&) e)
            {
                return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e));
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using TransformObserver = TransformObserver<Observer>;
            return XRX_MOV(_source).subscribe(TransformObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_transform)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(transform);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(F);
        using Impl = TransformObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(transform)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename F>
    auto transform(XRX_RVALUE(F&&) transform_)
    {
        XRX_CHECK_RVALUE(transform_);
        return [_transform = XRX_MOV(transform_)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Transform()
                , XRX_MOV(source), XRX_MOV(_transform));
        };
    }
} // namespace xrx

#include "xrx_epilogue.h"
