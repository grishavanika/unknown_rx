#pragma once
#include "concepts_observable.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "observable_interface.h"
#include "utils_ref_tracking_observer.h"
#include <utility>
#include <type_traits>

namespace xrx::observable
{
    namespace detail
    {
        template<typename Detach_>
        struct SubscribeDispatch
        {
            static_assert(::xrx::detail::ConceptUnsubscriber<Detach_>);
            using detach = Detach_;

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                return XRX_FWD(on_subscribe)(XRX_FWD(observer));
            }
        };

        template<>
        struct SubscribeDispatch<void>
        {
            using detach = ::xrx::detail::NoopDetach;

            template<typename F, typename Observer>
            static detach invoke_(F&& on_subscribe, Observer&& observer)
            {
                using Observer_ = std::remove_reference_t<Observer>;
                using RefObserver = ::xrx::detail::RefTrackingObserver_<Observer_>;
                ::xrx::detail::RefObserverState state;
                (void)XRX_FWD(on_subscribe)(RefObserver(&observer, &state));
                assert(state.is_finalized());
                return detach();
            }
        };

        template<typename Return_>
        constexpr auto detect_if_async_return()
        {
            if constexpr (std::is_same_v<Return_, void>)
            {
                // Assume consumer does not remember reference to observer and simply
                // returns nothing. Should be documented that stream must be complete at
                // return point.
                return std::false_type();
            }
            else if constexpr (::xrx::detail::ConceptUnsubscriber<Return_>)
            {
                // Assume completed if returned unsubscriber with no effect.
                using has_effect = typename Return_::has_effect;
                return has_effect();
            }
            else
            {
                static_assert(::xrx::detail::AlwaysFalse<Return_>()
                    , "Custom observable in .create() returns unknown type. "
                      "Either void or Unsubscriber should be returned.");
            }
        };

        template<typename Value, typename Error, typename F>
        struct CustomObservable_
        {
            F _on_subscribe;

            using value_type = Value;
            using error_type = Error;

            using ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>;
            using Return_ = decltype(XRX_MOV(_on_subscribe)(ObserverArchetype()));
            using is_async = decltype(detect_if_async_return<Return_>());
            // is_async can be false for non-void Return_ where Unsubscribed is no-op.
            // In this case we should assume that stream is completed after
            // .subscribe() end. Using CustomUnsubscriber<void> will validate those assumptions.
            using ReturnIfAsync = std::conditional_t<is_async::value
                , Return_
                , void>;
            using SubscribeDispatch_ = SubscribeDispatch<ReturnIfAsync>;
            using detach = typename SubscribeDispatch_::detach;

            auto fork() && { return CustomObservable_(XRX_MOV(_on_subscribe)); }
            auto fork() &  { return CustomObservable_(_on_subscribe); }

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            detach subscribe(Observer observer) &&
            {
                return SubscribeDispatch_::invoke_(XRX_MOV(_on_subscribe), XRX_MOV(observer));
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Value, typename Error, typename F
        , typename ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , ::xrx::detail::operator_tag::Create<Value, Error>
        , XRX_RVALUE(F&&) on_subscribe)
            requires requires(ObserverArchetype observer)
        {
            on_subscribe(XRX_MOV(observer));
        }
    {
        XRX_CHECK_RVALUE(on_subscribe);
        static_assert(not std::is_same_v<Value, void>);
        static_assert(not std::is_reference_v<Value>);
        using F_ = std::remove_reference_t<F>;
        using Impl = ::xrx::observable::detail::CustomObservable_<Value, Error, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(on_subscribe)));
    }
} // namespace xrx::detail::operator_tag
