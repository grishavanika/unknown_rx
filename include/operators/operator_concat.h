#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Tuple, bool IsAsync>
    struct ConcatObservable;

    template<typename Tuple>
    struct ConcatObservable<Tuple, false/*synchronous*/>
    {
        static_assert(std::tuple_size_v<Tuple> >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        // #XXX: check that all Unsubscribers from all Observables do not have effect.

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(std::move(_tuple)); }
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        template<typename Observer>
        NoopUnsubscriber subscribe(Observer&& observer) &&
        {
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                bool unsubscribed = false;
                bool end_with_error = false;
                bool completed = false;
                auto unsubscribe = XRX_FWD(observable).subscribe(observer::make(
                      [&](value_type value)
                {
                    assert(not unsubscribed);
                    const auto action = on_next_with_action(observer, XRX_FWD(value));
                    unsubscribed = action._stop;
                    return action;
                }
                    , [&]() { completed = true; } // on_completed(): nothing to do, move to next observable.
                    , [&](error_type error)
                {
                    end_with_error = true;
                    return on_error_optional(observer, XRX_FWD(error));
                }));
                static_assert(not decltype(unsubscribe)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (unsubscribed || end_with_error);
                assert((completed || stop)
                    && "Sync Observable should be ended after .subscribe() return.");
                return (not stop);
            };

            const bool all_processed = std::apply([&](auto&&... observables)
            {
                return (invoke_(observer, XRX_FWD(observables)) && ...);
            }
                , XRX_MOV(_tuple));
            if (all_processed)
            {
                on_completed_optional(observer);
            }
            return NoopUnsubscriber();
        }
    };

    template<typename Observable1, typename Observable2, bool>
    struct HaveSameStreamTypes
    {
        static constexpr bool value =
               std::is_same_v<typename Observable1::value_type
                            , typename Observable2::value_type>
            && std::is_same_v<typename Observable1::error_type
                            , typename Observable2::error_type>;
    };

    template<typename Observable1, typename Observable2>
    struct HaveSameStreamTypes<Observable1, Observable2
        , false> // not Observables.
            : std::false_type
    {
    };

    // AreConcatCompatible<> is a combination of different workarounds
    // for MSVC and GCC. We just need to check if value_type (+error_type) are same
    // for both types, if they are Observables.
    template<typename Observable1, typename Observable2>
    struct AreConcatCompatible
    {
        static constexpr bool are_observables =
               ConceptObservable<Observable1>
            && ConceptObservable<Observable2>;
        static constexpr bool value =
            HaveSameStreamTypes<
                Observable1
              , Observable2
              , are_observables>::value;
    };

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Concat
        , Observable1 observable1, Observable2 observable2, ObservablesRest... observables)
            requires  (AreConcatCompatible<Observable1, Observable2>::value
                   && (AreConcatCompatible<Observable1, ObservablesRest>::value && ...))
    {
        constexpr bool IsAnyAsync = false
            or ( IsAsyncObservable<Observable1>())
            or ( IsAsyncObservable<Observable2>())
            or ((IsAsyncObservable<ObservablesRest>()) or ...);

        using Tuple = std::tuple<Observable1, Observable2, ObservablesRest...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(XRX_MOV(observable1), XRX_MOV(observable2), XRX_MOV(observables)...)));
    }
} // namespace xrx::detail
