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
        static_assert(not std::is_reference_v<SourceObservable>);
        static_assert(ConceptObservable<SourceObservable>
            , "SourceObservable should satisfy Observable concept.");

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;
        using is_async = IsAsyncObservable<SourceObservable>;

        static_assert(not std::is_reference_v<value_type>
            , "Observable<> does not support emitting values of reference type.");
        static_assert(not std::is_reference_v<error_type>
            , "Observable<> does not support emitting error of reference type.");

        SourceObservable _source;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            // #XXX: we can require nothing if is_async == false, I guess.
            static_assert(std::is_move_constructible_v<Observer>);
            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        Observable_&& fork() &&      { return XRX_MOV(*this); }
        Observable_   fork() &       { return Observable_(_source.fork()); }
        Observable_&& fork_move() && { return XRX_MOV(*this); }
        Observable_   fork_move() &  { return XRX_MOV(*this); }

        template<typename Scheduler>
        auto subscribe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            static_assert(not std::is_lvalue_reference_v<Scheduler>);
            return make_operator(detail::operator_tag::SubscribeOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            static_assert(not std::is_lvalue_reference_v<Scheduler>);
            return make_operator(detail::operator_tag::ObserveOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        auto publish() &&
        {
            return make_operator(detail::operator_tag::Publish()
                , XRX_MOV(*this));
        }

        template<typename F>
        auto filter(F&& f) &&
        {
            return make_operator(detail::operator_tag::Filter()
                , XRX_MOV(*this), XRX_FWD(f));
        }
        template<typename F>
        auto transform(F&& f) &&
        {
            return make_operator(detail::operator_tag::Transform()
                , XRX_MOV(*this), XRX_FWD(f));
        }
        auto take(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Take()
                , XRX_MOV(*this), std::size_t(count));
        }
        auto repeat(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , XRX_MOV(*this), std::size_t(count), std::false_type()/*not infinity*/);
        }
        auto repeat() &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , XRX_MOV(*this), std::size_t(0), std::true_type()/*infinity*/);
        }
        template<typename Produce>
        auto flat_map(Produce&& produce) &&
        {
            return make_operator(detail::operator_tag::FlatMap()
                , XRX_MOV(*this), XRX_FWD(produce));
        }
        template<typename Produce, typename Map>
        auto flat_map(Produce&& produce, Map&& map) &&
        {
            return make_operator(detail::operator_tag::FlatMap()
                , XRX_MOV(*this), XRX_FWD(produce), XRX_FWD(map));
        }
        auto window(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Window()
                , XRX_MOV(*this), std::size_t(count));
        }
        template<typename Value, typename Op>
        auto reduce(Value&& initial, Op&& op) &&
        {
            return make_operator(detail::operator_tag::Reduce()
                , XRX_MOV(*this), XRX_FWD(initial), XRX_FWD(op));
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
