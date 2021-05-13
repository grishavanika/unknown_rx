#pragma once
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "debug/assert_flag.h"
#include <utility>
#include <concepts>

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
                , _filter(std::move(f))
                , _disconnected() {}
            Filter _filter;
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            OnNextAction on_next(Value&& v)
            {
                _disconnected.check_not_set();
                if (not _filter(v))
                {
                    return OnNextAction();
                }
                return ::xrx::detail::ensure_action_state(
                    ::xrx::detail::on_next_with_action(observer(), std::forward<Value>(v))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using FilterObserver = FilterObserver<Observer_>;
            return std::move(_source).subscribe(FilterObserver(std::forward<Observer>(observer), std::move(_filter)));
        }

        FilterObservable fork()
        {
            return FilterObservable(_source.fork(), _filter);
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , SourceObservable source, F filter)
            requires ConceptObservable<SourceObservable>
                && std::predicate<F, typename SourceObservable::value_type>
    {
        using Impl = FilterObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(std::move(source), std::move(filter)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberFilter
        {
            F _f;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Filter
                    , SourceObservable, F>
            {
                return make_operator(operator_tag::Filter()
                    , XRX_MOV(source), XRX_MOV(_f));
            }
        };
    } // namespace detail

    template<typename F>
    auto filter(F filter)
    {
        return detail::RememberFilter<F>(XRX_MOV(filter));
    }
} // namespace xrx
