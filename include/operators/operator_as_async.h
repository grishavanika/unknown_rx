#pragma once
#include "observable_interface.h"
#include "xrx_prologue.h"

namespace xrx::detail
{
    template<typename SourceObservable>
    struct AsyncObservable
    {
        SourceObservable _source;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using DetachHandle = typename SourceObservable::DetachHandle;
        using is_async = std::true_type; // Disregard Source. That's the whole point.

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        AsyncObservable fork() && { return AsyncObservable(XRX_MOV(_source)); }
        AsyncObservable fork() &  { return AsyncObservable(_source.fork()); }
    };
} // namespace xrx::detail

namespace xrx
{
    inline auto as_async()
    {
        return [](XRX_RVALUE(auto&&) source)
        {
            using SourceObservable = std::remove_reference_t<decltype(source)>;
            XRX_CHECK_RVALUE(source);
            XRX_CHECK_TYPE_NOT_REF(SourceObservable);
            using Impl = ::xrx::detail::AsyncObservable<SourceObservable>;
            return Observable_<Impl>(Impl(XRX_MOV(source)));
        };
    }
} // namespace xrx

#include "xrx_epilogue.h"
