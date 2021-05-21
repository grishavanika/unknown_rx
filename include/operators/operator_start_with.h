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
    template<typename SourceObservable, typename Tuple>
    struct StartWithObservable
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;
        Tuple _tuple;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
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
            if (not all_processed)
            {
                return Unsubscriber();
            }

            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        StartWithObservable fork() && { return FromObservable(XRX_MOV(_source), XRX_MOV(_tuple)); }
        StartWithObservable fork() &  { return FromObservable(_source.fork(), _tuple); }
    };

    template<typename SourceObservable, typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::StartsWith
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(V&&) v0, XRX_RVALUE(Vs&&)... vs)
    {
        static_assert((not std::is_lvalue_reference_v<V>)
                  && ((not std::is_lvalue_reference_v<Vs>) && ...)
            , ".start_with(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<std::remove_reference_t<V>, std::remove_reference_t<Vs>...>;
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;

#if (0)
        static_assert(std::is_same_v<typename std::tuple_element<0, Tuple>::type
            , typename SourceObservable_::value_type>);
#endif

        using Impl = StartWithObservable<SourceObservable_, Tuple>;
        return Observable_<Impl>(Impl(XRX_MOV(source), Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail
