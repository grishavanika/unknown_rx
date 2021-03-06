#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "xrx_prologue.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Tuple>
    struct FromObservable
    {
        using value_type = typename std::tuple_element<0, Tuple>::type;
        using error_type = void_;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;

        Tuple _tuple;

        template<typename Observer>
        DetachHandle subscribe(Observer&& observer) &&
        {
            auto invoke_ = [](auto& observer, auto&& value)
            {
                const auto action = on_next_with_action(observer, XRX_FWD(value));
                return (not action._stop);
            };

            const bool all_processed = std::apply([&](auto&&... vs)
            {
                return (invoke_(observer, XRX_FWD(vs)) && ...);
            }
                , XRX_MOV(_tuple));
            if (all_processed)
            {
                (void)on_completed_optional(XRX_FWD(observer));
            }
            return DetachHandle();
        }

        FromObservable fork() && { return FromObservable(XRX_MOV(_tuple)); }
        FromObservable fork() &  { return FromObservable(_tuple); }
    };

    template<typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::From
        , XRX_RVALUE(V&&) v0, XRX_RVALUE(Vs&&)... vs)
    {
        XRX_CHECK_RVALUE(v0);
        XRX_CHECK_TYPE_NOT_REF(V);
        static_assert(((not std::is_lvalue_reference_v<Vs>) && ...)
            , ".from(Vs...) requires owns passed values.");
        static_assert(((not std::is_reference_v<Vs>) && ...)
            , ".from(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<V, Vs...>;
        using Impl = FromObservable<Tuple>;
        return Observable_<Impl>(Impl(Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail

#include "xrx_epilogue.h"
