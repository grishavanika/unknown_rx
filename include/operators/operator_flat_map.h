#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "observable_interface.h"
#include "debug/assert_flag.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , bool IsSourceAsync
        , bool IsProducedAsync>
    struct FlatMapObservable;

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , false/*synchronous source Observable*/
        , false/*synchronous Observable that was produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        SourceObservable _source;
        Produce _produce;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(std::is_same_v<source_error, produce_error>
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using value_type = produce_type;
        using error_type = source_error;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(Observer&& destination_) &&
        {
            auto invoke_ = [](auto&& destination_, auto&& inner)
            {
                bool unsubscribed = false;
                bool end_with_error = false;
                bool completed = false;
                auto unsubscribe = XRX_FWD(inner).subscribe(::xrx::observer::make(
                      [&](produce_type value)
                {
                    assert(not unsubscribed);
                    const auto action = on_next_with_action(destination_, XRX_FWD(value));
                    unsubscribed = action._unsubscribe;
                    return action;
                }
                    , [&]()
                {
                    assert(not unsubscribed);
                    // on_completed(): nothing to do, move to next observable.
                    completed = true;
                }
                    , [&](produce_error error)
                {
                    assert(not unsubscribed);
                    end_with_error = true;
                    return on_error_optional(destination_, XRX_FWD(error));
                }));
                static_assert(not decltype(unsubscribe)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (unsubscribed || end_with_error);
                assert((completed || stop)
                    && "Sync Observable should be ended after .subscribe() return.");
                return (not stop);
            };

            bool unsubscribed = false;
            bool end_with_error = false;
            bool completed = false;
            auto unsubscribe = XRX_MOV(_source).subscribe(::xrx::observer::make(
                    [&](source_type source_value)
            {
                assert(not unsubscribed);
                const bool continue_ = invoke_(destination_, _produce(XRX_FWD(source_value)));
                unsubscribed = (not continue_);
                return ::xrx::unsubscribe(unsubscribed);
            }
                , [&]()
            {
                assert(not unsubscribed);
                completed = true;
                return on_completed_optional(destination_);
            }
                , [&](error_type error)
            {
                assert(not unsubscribed);
                end_with_error = true;
                return on_error_optional(destination_, XRX_FWD(error));
            }));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (unsubscribed || end_with_error);
            (void)stop;
            assert((completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }
    };

    template<typename SourceObservable, typename Produce>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , SourceObservable source, Produce produce)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable::value_type>()));
        using Impl = FlatMapObservable<
            SourceObservable
            , ProducedObservable
            , Produce
            , IsAsyncObservable<SourceObservable>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberFlatMap
        {
            F _produce;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::FlatMap
                    , SourceObservable, F>
            {
                return make_operator(operator_tag::FlatMap()
                    , XRX_MOV(source), XRX_MOV(_produce));
            }
        };
    } // namespace detail

    template<typename F>
    auto flat_map(F produce)
    {
        return detail::RememberFlatMap<F>(XRX_MOV(produce));
    }
} // namespace xrx
