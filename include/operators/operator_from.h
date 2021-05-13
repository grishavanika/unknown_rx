#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_fast_FWD.h"
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
        struct NoUnsubscription
        {
            using has_effect = std::false_type;
            constexpr bool detach() const noexcept { return false; }
        };

        using value_type = typename std::tuple_element<0, Tuple>::type;
        using error_type = none_tag;
        using Unsubscriber = NoUnsubscription;

        Tuple _tuple;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            auto invoke_ = [](auto&& o, auto&& v)
            {
                const auto action = on_next_with_action(XRX_FWD(o), XRX_FWD(v));
                return (not action._unsubscribe);
            };

            std::apply([&](auto&&... vs) { (invoke_(observer, XRX_FWD(vs)) && ...); }, std::move(_tuple));
            (void)on_completed_optional(XRX_FWD(observer));
            return Unsubscriber();
        }

        FromObservable fork() && { return FromObservable(std::move(_tuple)); }
        FromObservable fork() &  { return FromObservable(_tuple); }
    };

    template<typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::From
        , V v0, Vs... vs)
    {
        using Tuple = std::tuple<V, Vs...>;
        using Impl = FromObservable<Tuple>;
        return Observable_<Impl>(Impl(Tuple(std::move(v0), std::move(vs)...)));
    }
} // namespace xrx::detail
