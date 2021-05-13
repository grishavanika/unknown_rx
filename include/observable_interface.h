#pragma once
#include "concepts_observer.h"
#include "concepts_observable.h"
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "utils_fast_FWD.h"

#include <type_traits>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct Observable_
    {
        static_assert(ConceptObservable<SourceObservable>
            , "SourceObservable should satisfy Observable concept.");

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        using is_async = IsAsyncObservable<SourceObservable>;

        SourceObservable _source;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            return std::move(_source).subscribe(std::forward<Observer>(observer));
        }

        Observable_ fork() { return Observable_(_source.fork()); }

        template<typename Scheduler>
        auto subscribe_on(Scheduler&& scheduler) &&
        {
            return make_operator(detail::operator_tag::SubscribeOn()
                , std::move(*this), std::forward<Scheduler>(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(Scheduler&& scheduler) &&
        {
            return make_operator(detail::operator_tag::ObserveOn()
                , std::move(*this), std::forward<Scheduler>(scheduler));
        }

        auto publish() &&
        {
            return make_operator(detail::operator_tag::Publish()
                , std::move(*this));
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
        auto take(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Take()
                , std::move(*this), std::size_t(count));
        }
        auto repeat(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , std::move(*this), std::size_t(count), std::false_type()/*not infinity*/);
        }
        auto repeat() &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , std::move(*this), std::size_t(0), std::true_type()/*infinity*/);
        }
        template<typename Produce>
        auto flat_map(Produce&& produce) &&
        {
            return make_operator(detail::operator_tag::FlatMap()
                , std::move(*this), XRX_FWD(produce));
        }
    };
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename Observer>
        struct RememberSubscribe
        {
            Observer _observer;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires requires { XRX_MOV(source).subscribe(XRX_MOV(_observer)); }
            {
                return XRX_MOV(source).subscribe(XRX_MOV(_observer));
            }
        };
    } // namespace detail

    template<typename Observer>
    inline auto subscribe(Observer observer)
    {
        return detail::RememberSubscribe<Observer>(XRX_MOV(observer));
    }
} // namespace xrx

template<typename SourceObservable, typename PipeConnect>
auto operator|(::xrx::detail::Observable_<SourceObservable>&& source_rvalue, PipeConnect connect)
    requires requires { XRX_MOV(connect).pipe_(XRX_MOV(source_rvalue)); }
{
    return XRX_MOV(connect).pipe_(XRX_MOV(source_rvalue));
}
