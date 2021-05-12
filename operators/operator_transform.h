#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, typename Transform>
    struct TransformObservable
    {
        SourceObservable _source;
        Transform _transform;

        using value_type   = decltype(_transform(std::declval<typename SourceObservable::value_type>()));
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;


        template<typename Observer>
        struct TransformObserver : Observer
        {
            explicit TransformObserver(Observer&& o, Transform&& f)
                : Observer(std::move(o))
                , _transform(std::move(f)) {}
            Transform _transform;
            Observer& observer() { return *this; }

            template<typename Value>
            void on_next(Value&& v)
            {
                // #XXX: handle unsubscribe.
                observer().on_next(
                    _transform(std::forward<Value>(v)));
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            auto strict = observer::make_complete(std::forward<Observer>(observer));
            using Observer_ = decltype(strict);
            using TransformObserver = TransformObserver<Observer_>;
            return std::move(_source).subscribe(TransformObserver(std::move(strict), std::move(_transform)));
        }

        TransformObservable fork()
        {
            return TransformObservable(_source.fork(), _transform);
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , SourceObservable source, F transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        using Impl = TransformObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(std::move(source), std::move(transform)));
    }
} // namespace xrx::detail
