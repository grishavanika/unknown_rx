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
    template<typename SourceObservable, typename Tuple>
    struct StartWithObservable
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

        SourceObservable _source;
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
            if (not all_processed)
            {
                return DetachHandle();
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
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(v0);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(V);
        static_assert(((not std::is_reference_v<Vs>) && ...)
            , ".start_with(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<V, Vs...>;
        // #XXX: finalize this.
#if (0)
        static_assert(std::is_same_v<typename std::tuple_element<0, Tuple>::type
            , typename SourceObservable::value_type>);
#endif

        using Impl = StartWithObservable<SourceObservable, Tuple>;
        return Observable_<Impl>(Impl(XRX_MOV(source), Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail

#include "xrx_epilogue.h"
