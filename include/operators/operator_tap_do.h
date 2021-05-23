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
    template<typename SourceObservable, typename Listener>
    struct TapOrDoObservable
    {
        SourceObservable _source;
        Listener _listener;

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

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

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    (void)on_error_optional(XRX_MOV(_listener.get()));
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    auto copy = [](auto e) { return XRX_MOV(e); };
                    (void)on_error_optional(XRX_MOV(_listener.get()), copy(e)...);
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using ListenerObserver = ListenerObserver<Observer_>;
            return XRX_MOV(_source).subscribe(ListenerObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_listener)));
        }
    };

    template<typename SourceObservable, typename Observer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::TapOrDo
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Observer&&) observer)
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Observer_ = std::remove_reference_t<Observer>;
        using Impl = TapOrDoObservable<SourceObservable_, Observer_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(observer)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename Observer>
        struct RememberTapOrDo
        {
            Observer _observer;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::TapOrDo
                    , SourceObservable, Observer>
            {
                return make_operator(operator_tag::TapOrDo()
                    , XRX_MOV(source), XRX_MOV(_observer));
            }
        };
    } // namespace detail

    template<typename Observer>
    auto tap(XRX_RVALUE(Observer&&) observer)
    {
        static_assert(not std::is_lvalue_reference_v<Observer>);
        using Observer_ = std::remove_reference_t<Observer>;
        return detail::RememberTapOrDo<Observer_>(XRX_MOV(observer));
    }
} // namespace xrx
