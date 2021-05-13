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
    template<typename Tuple>
    struct FromObservable
    {
        using value_type = typename std::tuple_element<0, Tuple>::type;
        using error_type = none_tag;
        using Unsubscriber = NoopUnsubscriber;

        Tuple _tuple;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            auto invoke_ = [](auto&& observer, auto&& value)
            {
                const auto action = on_next_with_action(XRX_FWD(observer), XRX_FWD(value));
                return (not action._unsubscribe);
            };

            std::apply([&](auto&&... vs) { (invoke_(observer, XRX_FWD(vs)) && ...); }, std::move(_tuple));
            // #XXX: should not be called if unsubscribed.
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
