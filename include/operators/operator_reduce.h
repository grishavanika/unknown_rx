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
    template<typename SourceObservable, typename Value, typename Op>
    struct ReduceObservable
    {
        static_assert(not std::is_reference_v<SourceObservable>);
        static_assert(not std::is_reference_v<Op>);

        using source_type = typename SourceObservable::value_type;
        using value_type   = Value;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

        SourceObservable _source;
        Value _initial;
        Op _op;

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct ReduceObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Op> _op;
            Value _value;

            XRX_FORCEINLINE() void on_next(XRX_RVALUE(source_type&&) v)
            {
                _value = _op.get()(XRX_MOV(_value), XRX_MOV(v));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                const auto action = ::xrx::detail::on_next_with_action(_observer.get(), XRX_MOV(_value));
                if (not action._stop)
                {
                    on_completed_optional(XRX_MOV(_observer.get()));
                }
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

        ReduceObservable fork() && { return ReduceObservable(XRX_MOV(_source), XRX_MOV(_op)); }
        ReduceObservable fork() &  { return ReduceObservable(_source.fork(), _op); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            using Observer_ = std::remove_reference_t<Observer>;
            using ReduceObserver_ = ReduceObserver<Observer_>;
            return XRX_MOV(_source).subscribe(ReduceObserver_(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_op), XRX_MOV(_initial)));
        }
    };

    template<typename SourceObservable, typename Value, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Reduce
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Value&&) value, XRX_RVALUE(F&&) op)
            requires ConceptObservable<SourceObservable>
                && requires { { op(XRX_MOV(value), std::declval<typename SourceObservable::value_type>()) } -> std::convertible_to<Value>; }
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(value);
        XRX_CHECK_RVALUE(op);
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using F_ = std::remove_reference_t<F>;
        using Value_ = std::remove_reference_t<Value>;
        using Impl = ReduceObservable<SourceObservable_, Value_, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(value), XRX_MOV(op)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename Value, typename F>
    auto reduce(XRX_RVALUE(Value&&) initial, XRX_RVALUE(F&&) op)
    {
        XRX_CHECK_RVALUE(initial);
        XRX_CHECK_RVALUE(op);
        return [_value = XRX_MOV(initial), _f = XRX_MOV(op)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Reduce()
                , XRX_MOV(source), XRX_MOV(_value), XRX_MOV(_f));
        };
    }
} // namespace xrx
