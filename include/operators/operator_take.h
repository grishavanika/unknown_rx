#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "concepts_observable.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include <type_traits>
#include <utility>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct TakeObservable
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async   = IsAsyncObservable<SourceObservable>;
        using DetachHandle     = typename SourceObservable::DetachHandle;

        template<typename Observer>
        struct TakeObserver_
        {
            Observer _observer;
            std::size_t _max = 0;
            std::size_t _taken = 0;

            // #TODO: on_error(void) support.
            OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                assert(_max > 0);
                assert(_taken < _max);
                const auto action = ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
                if (++_taken >= _max)
                {
                    ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
                    return OnNextAction{._stop = true};
                }
                return action;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                (void)::xrx::detail::on_completed_optional(XRX_MOV(observer));
                return DetachHandle();
            }
            using TakeObserver_ = TakeObserver_<Observer>;
            return XRX_MOV(_source).subscribe(TakeObserver_(XRX_MOV(observer), _count));
        }

        TakeObservable fork() && { return TakeObservable(XRX_MOV(_source), _count); };
        TakeObservable fork() &  { return TakeObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Take
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using Impl = TakeObservable<SourceObservable>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto take(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Take()
                , XRX_MOV(source), std::size_t(_count));
        };
    }
} // namespace xrx
