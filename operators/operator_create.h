#pragma once
#include "tag_invoke.hpp"
#include "meta_utils.h"
#include "concepts_observer.h"
#include "concepts_observable.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include <utility>

namespace xrx::observable
{
    namespace detail
    {
        // #TODO: allow any unsubscriber, just with `bool detach()`.
        // Add default `using has_effect = std::true_type` implementation in that case.
        // #TODO: in case `ExternalUnsubscriber` is full ConceptUnsubscriber<>
        // just return it; do not create unnecesary wrapper.
        template<typename ExternalUnsubscriber>
            // requires ConceptUnsubscriber<ExternalUnsubscriber>
        struct CreateUnsubscriber : ExternalUnsubscriber
        {
            explicit CreateUnsubscriber(ExternalUnsubscriber subscriber)
                : ExternalUnsubscriber(std::move(subscriber))
            {
            }

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                return CreateUnsubscriber(std::forward<F>(on_subscribe)
                    (std::forward<Observer>(observer)));
            }
        };

        template<>
        struct CreateUnsubscriber<void>
        {
            using has_effect = std::false_type;
            bool detach() { return false; }

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                (void)std::forward<F>(on_subscribe)(std::forward<Observer>(observer));
                return CreateUnsubscriber();
            }
        };

        template<typename Value, typename Error, typename F>
        struct CreateObservable_
        {
            F _on_subscribe;

            using value_type = Value;
            using error_type = Error;
            
            using ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>;
            using CreateReturn = decltype(std::move(_on_subscribe)(ObserverArchetype()));
            static_assert(std::is_same_v<void, CreateReturn> or ConceptUnsubscriber<CreateReturn>
                , "Return value of create() callback should be Unsubscriber-like.");

            using Unsubscriber = CreateUnsubscriber<CreateReturn>;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                auto strict = ::xrx::observer::make_complete(std::move(observer));
                return Unsubscriber::invoke_(std::move(_on_subscribe), std::move(strict));
            }

            auto fork() && { return CreateObservable_(std::move(_on_subscribe)); }
            auto fork() &  { return CreateObservable_(_on_subscribe); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Value, typename Error, typename F
        , typename ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::Create<Value, Error>
        , F on_subscribe)
            requires requires(ObserverArchetype observer)
        {
            on_subscribe(observer);
        }
    {
        using Impl = ::xrx::observable::detail::CreateObservable_<Value, Error, F>;
        return Observable_<Impl>(Impl(std::move(on_subscribe)));
    }
} // namespace xrx::detail::operator_tag
