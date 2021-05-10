#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include <utility>

namespace xrx::observable
{
    template<typename Duration, typename Scheduler>
    auto interval(Duration period, Scheduler scheduler)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Interval()
            , std::move(period), std::move(scheduler));
    }
} // namespace xrx::observable
