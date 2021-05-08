#pragma once
#include "tag_invoke.hpp"
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, typename Filter>
    struct FilterObservable
    {
        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;
        Filter _filter;

        template<typename Observer>
        struct FilterObserver : Observer
        {
            explicit FilterObserver(Observer&& o, Filter&& f)
                : Observer(std::move(o))
                , _filter(std::move(f)) {}
            Filter _filter;
            Observer& observer() { return *this; }

            template<typename Value>
            void on_next(Value&& v)
            {
                if (_filter(v))
                {
                    (void)observer().on_next(std::forward<Value>(v));
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            auto strict = observer::make_complete(std::forward<Observer>(observer));
            using Observer_ = decltype(strict);
            using FilterObserver = FilterObserver<Observer_>;
            return std::move(_source).subscribe(FilterObserver(std::move(strict), std::move(_filter)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , SourceObservable source, F filter)
            requires std::predicate<F, typename SourceObservable::value_type>
    {
        using Impl = FilterObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(std::move(source), std::move(filter)));
    }
} // namespace xrx::detail
