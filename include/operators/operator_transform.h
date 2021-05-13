#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "observable_interface.h"
#include "debug/assert_flag.h"
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
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;


        template<typename Observer>
        struct TransformObserver : Observer
        {
            explicit TransformObserver(Observer&& o, Transform&& f)
                : Observer(std::move(o))
                , _transform(std::move(f))
                , _disconnected() {}
            Transform _transform;
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            auto on_next(Value&& v)
            {
                _disconnected.check_not_set();
                return xrx::detail::ensure_action_state(
                    xrx::detail::on_next_with_action(observer()
                        , _transform(std::forward<Value>(v)))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using TransformObserver = TransformObserver<Observer_>;
            return std::move(_source).subscribe(TransformObserver(
                std::forward<Observer>(observer), std::move(_transform)));
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

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberTransform
        {
            F _transform;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Transform
                    , SourceObservable, F>
            {
                return make_operator(operator_tag::Transform()
                    , XRX_MOV(source), XRX_MOV(_transform));
            }
        };
    } // namespace detail

    template<typename F>
    auto transform(F transform_)
    {
        return detail::RememberTransform<F>(XRX_MOV(transform_));
    }
} // namespace xrx
