#pragma once
#include "concepts_observer.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"

#include <type_traits>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct Observable_
    {
        static_assert(std::is_copy_constructible_v<SourceObservable>
            , "Observable required to be copyable; acts like a handle to a source stream.");
        static_assert(std::is_move_constructible_v<SourceObservable>
            , "Observable required to be movable; acts like a handle to a source stream.");

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            return std::move(_source).subscribe(std::forward<Observer>(observer));
        }

        template<typename F>
        auto filter(F&& f) &&
        {
            return make_operator(detail::operator_tag::Filter()
                , std::move(*this), std::forward<F>(f));
        }
        template<typename F>
        auto transform(F&& f) &&
        {
            return make_operator(detail::operator_tag::Transform()
                , std::move(*this), std::forward<F>(f));
        }
    };
} // namespace xrx::detail
