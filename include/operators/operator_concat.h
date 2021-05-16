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
    struct ConcatObservable<Tuple, false/*Sync*/>
    {
        static_assert(std::tuple_size_v<Tuple> >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_tuple)); }
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        struct State
        {
            bool _unsubscribed = false;
            bool _end_with_error = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct OneObserver
        {
            Observer* _destination = nullptr;
            State* _state = nullptr;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(value_type&&) value)
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                const auto action = on_next_with_action(*_destination, XRX_MOV(value));
                _state->_unsubscribed = action._stop;
                return action;
            }
            XRX_FORCEINLINE() void on_completed()
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                _state->_completed = true; // on_completed(): nothing to do, move to next observable.
            }
            template<typename... VoidOrError>
            XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(*_destination));
                }
                else
                {
                    on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
                }
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
        NoopUnsubscriber subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                State state;
                auto unsubscribe = XRX_FWD(observable).subscribe(
                    OneObserver<Observer_>(&observer, &state));
                static_assert(not decltype(unsubscribe)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (state._unsubscribed || state._end_with_error);
                assert((state._completed || stop)
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
        , XRX_RVALUE(Observable1&&) observable1
        , XRX_RVALUE(Observable2&&) observable2
        , XRX_RVALUE(ObservablesRest&&)... observables)
            requires  (AreConcatCompatible<Observable1, Observable2>::value
                   && (AreConcatCompatible<Observable1, ObservablesRest>::value && ...))
    {
        constexpr bool IsAnyAsync = false
            or ( IsAsyncObservable<Observable1>())
            or ( IsAsyncObservable<Observable2>())
            or ((IsAsyncObservable<ObservablesRest>()) or ...);

        using Tuple = std::tuple<
            std::remove_reference_t<Observable1>
            , std::remove_reference_t<Observable2>
            , std::remove_reference_t<ObservablesRest>...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(
            XRX_MOV(observable1)
            , XRX_MOV(observable2)
            , XRX_MOV(observables)...)));
    }
} // namespace xrx::detail
