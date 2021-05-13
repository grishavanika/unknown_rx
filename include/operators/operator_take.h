#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "concepts_observable.h"
#include "observable_interface.h"
#include <type_traits>
#include <utility>

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
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            OnNextAction on_next(Value&& v)
            {
                _disconnected.check_not_set();
                if (++_taken > _max)
                {
                    ::xrx::detail::on_completed_optional(std::move(observer()));
                    _disconnected.raise();
                    return OnNextAction{._unsubscribe = true};
                }
                return xrx::detail::ensure_action_state(
                    ::xrx::detail::on_next_with_action(observer(), std::forward<Value>(v))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer observer) &&
        {
            using TakeObserver_ = TakeObserver_<Observer>;
            return std::move(_source).subscribe(TakeObserver_(std::move(observer), _count));
        }

        TakeObservable fork() && { return TakeObservable(std::move(_source), _count); };
        TakeObservable fork() &  { return TakeObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Take
        , SourceObservable source, std::size_t count)
    {
        using Impl = TakeObservable<SourceObservable>;
        return Observable_<Impl>(Impl(std::move(source), count));
    }
} // namespace xrx::detail
