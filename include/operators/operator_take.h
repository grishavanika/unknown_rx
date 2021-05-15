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

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        template<typename Observer>
        struct TakeObserver_ : Observer
        {
            explicit TakeObserver_(Observer&& o, std::size_t count)
                : Observer(std::move(o))
                , _max(count)
                , _taken(0) {}
            std::size_t _max;
            std::size_t _taken;
            Observer& observer() { return *this; }

            template<typename Value>
            OnNextAction on_next(Value&& v)
            {
                assert(_max > 0);
                assert(_taken < _max);
                const auto action = ::xrx::detail::on_next_with_action(observer(), XRX_FWD(v));
                if (++_taken >= _max)
                {
                    ::xrx::detail::on_completed_optional(XRX_MOV(observer()));
                    return OnNextAction{._unsubscribe = true};
                }
                return action;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(Observer observer) &&
        {
            if (_count == 0)
            {
                (void)::xrx::detail::on_completed_optional(XRX_MOV(observer));
                return Unsubscriber();
            }
            using TakeObserver_ = TakeObserver_<Observer>;
            return std::move(_source).subscribe(TakeObserver_(std::move(observer), _count));
        }

        TakeObservable fork() && { return TakeObservable(std::move(_source), _count); };
        TakeObservable fork() &  { return TakeObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Take
        , SourceObservable source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using Impl = TakeObservable<SourceObservable>;
        return Observable_<Impl>(Impl(std::move(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        struct RememberTake
        {
            std::size_t _count = 0;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Take
                    , SourceObservable, std::size_t>
            {
                return make_operator(operator_tag::Take(), XRX_MOV(source), _count);
            }
        };
    } // namespace detail

    inline auto take(std::size_t count)
    {
        return detail::RememberTake(count);
    }
} // namespace xrx
