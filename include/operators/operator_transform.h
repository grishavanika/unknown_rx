#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_wrappers.h"
#include "utils_defines.h"
#include "observable_interface.h"
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
        using Unsubscriber = typename SourceObservable::Unsubscriber;

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

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e...));
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using TransformObserver = TransformObserver<Observer_>;
            return XRX_MOV(_source).subscribe(TransformObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_transform)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using F_ = std::remove_reference_t<F>;
        using Impl = TransformObservable<SourceObservable_, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(transform)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberTransform
        {
            F _transform;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Transform
                    , SourceObservable, F>
            {
                return make_operator(operator_tag::Transform()
                    , XRX_MOV(source), XRX_MOV(_transform));
            }
        };
    } // namespace detail

    template<typename F>
    auto transform(XRX_RVALUE(F&&) transform_)
    {
        using F_ = std::remove_reference_t<F>;
        return detail::RememberTransform<F_>(XRX_MOV(transform_));
    }
} // namespace xrx
