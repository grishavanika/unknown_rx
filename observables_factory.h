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

    template<typename Value, typename Error = void, typename F>
    auto create(F on_subscribe)
    {
        using Tag_ = xrx::detail::operator_tag::Create<Value, Error>;
        return ::xrx::detail::make_operator(Tag_()
            , std::move(on_subscribe));
    }

    template<typename Integer>
    auto range(Integer start = Integer())
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , start, 1, 1);
    }
} // namespace xrx::observable
