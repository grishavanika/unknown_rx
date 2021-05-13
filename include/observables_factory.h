#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_fast_FWD.h"
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
    auto range(Integer first)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, first/*last*/, 1/*step*/, std::true_type()/*endless*/);
    }
    template<typename Integer>
    auto range(Integer first, Integer last)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, last/*last*/, 1/*step*/, std::false_type()/*endless*/);
    }
    template<typename Integer>
    auto range(Integer first, Integer last, std::intmax_t step)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, last/*last*/, step/*step*/, std::false_type()/*endless*/);
    }

    template<typename V, typename... Vs>
    auto from(V v0, Vs... vs)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::From()
            , std::move(v0), std::move(vs)...);
    }

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto concat(Observable1&& observable1, Observable2&& observable2, ObservablesRest&&... observables)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Concat()
            , XRX_MOV(observable1), XRX_MOV(observable2), XRX_MOV(observables)...);
    }
} // namespace xrx::observable
