#pragma once
#include "xrx_prologue.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include <utility>

namespace xrx::observable
{
    template<typename Duration, typename Scheduler>
    auto interval(Duration period, XRX_RVALUE(Scheduler&&) scheduler)
    {
        XRX_CHECK_RVALUE(scheduler);
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Interval()
            , XRX_MOV(period), XRX_MOV(scheduler));
    }

    template<typename Value, typename Error = void_, typename F>
    auto create(XRX_RVALUE(F&&) on_subscribe)
    {
        static_assert(not std::is_same_v<Value, void>);
        static_assert(not std::is_same_v<Error, void>);
        XRX_CHECK_RVALUE(on_subscribe);
        using Tag_ = xrx::detail::operator_tag::Create<Value, Error>;
        return ::xrx::detail::make_operator(Tag_()
            , XRX_MOV(on_subscribe));
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
    auto from(V&& v0, Vs&&... vs)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::From()
            , XRX_FWD(v0), XRX_FWD(vs)...);
    }

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto concat(XRX_RVALUE(Observable1&&) observable1, XRX_RVALUE(Observable2&&) observable2, XRX_RVALUE(ObservablesRest&&)... observables)
    {
        XRX_CHECK_RVALUE(observable1);
        XRX_CHECK_RVALUE(observable2);
        static_assert(((not std::is_lvalue_reference_v<ObservablesRest>) && ...));
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Concat()
            , XRX_MOV(observable1), XRX_MOV(observable2), XRX_MOV(observables)...);
    }

    template<typename Container>
    auto iterate(XRX_RVALUE(Container&&) values)
    {
        XRX_CHECK_RVALUE(values);
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Iterate()
            , XRX_MOV(values));
    }
} // namespace xrx::observable

#include "xrx_epilogue.h"
