#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "xrx_prologue.h"
#include <utility>
#include <type_traits>
#include <concepts>
#include <cassert>

namespace xrx::detail
{
    template<typename Container>
    struct IterateObservable
    {
        using value_type = std::remove_reference_t<decltype(*std::begin(std::declval<Container>()))>;
        using error_type = none_tag;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;

        Container _vs;

        template<typename Observer>
        DetachHandle subscribe(Observer&& observer) &&
        {
            for (auto& v : _vs)
            {
                const auto action = on_next_with_action(observer, XRX_MOV(v));
                if (action._stop)
                {
                    return DetachHandle();
                }
            }
            (void)on_completed_optional(XRX_FWD(observer));
            return DetachHandle();
        }

        IterateObservable fork() && { return IterateObservable(XRX_MOV(_vs)); }
        IterateObservable fork() &  { return IterateObservable(_vs); }
    };

    template<typename Container>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Iterate
        , XRX_RVALUE(Container&&) values)
    {
        XRX_CHECK_RVALUE(values);
        XRX_CHECK_TYPE_NOT_REF(Container);
        using Iterator = decltype(std::begin(values));
        static_assert(std::forward_iterator<Iterator>);

        using Impl = IterateObservable<Container>;
        return Observable_<Impl>(Impl(XRX_MOV(values)));
    }
} // namespace xrx::detail

#include "xrx_epilogue.h"
