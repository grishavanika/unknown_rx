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
            XRX_CHECK_RVALUE(observer);
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
            XRX_CHECK_RVALUE(scheduler);
            return make_operator(detail::operator_tag::SubscribeOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            XRX_CHECK_RVALUE(scheduler);
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
        template<typename Observer>
        auto tap(Observer&& observer) &&
        {
            return make_operator(detail::operator_tag::TapOrDo()
                , XRX_MOV(*this), XRX_FWD(observer));
        }
        template<typename Observer> // same as tap().
        auto do_(Observer&& observer) &&
        {
            return make_operator(detail::operator_tag::TapOrDo()
                , XRX_MOV(*this), XRX_FWD(observer));
        }
        template<typename OpeningsObservable, typename CloseObservableProducer>
        auto window_toggle(OpeningsObservable&& openings, CloseObservableProducer&& close_producer) &&
        {
            return make_operator(detail::operator_tag::WindowToggle()
                , XRX_MOV(*this), XRX_FWD(openings), XRX_FWD(close_producer));
        }
        template<typename V, typename... Vs>
        auto starts_with(V&& v0, Vs&&... vs)
        {
            return ::xrx::detail::make_operator(xrx::detail::operator_tag::StartsWith()
                , XRX_MOV(*this), XRX_FWD(v0), XRX_FWD(vs)...);
        }
    };
} // namespace xrx::detail

namespace xrx
{
    template<typename Observer>
    inline auto subscribe(XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(observer);
        return [_observer = XRX_MOV(observer)](XRX_RVALUE(auto&&) source) mutable
        {
            return XRX_MOV(source).subscribe(XRX_MOV(_observer));
        };
    }
} // namespace xrx

template<typename SourceObservable, typename PipeConnect>
auto operator|(XRX_RVALUE(::xrx::detail::Observable_<SourceObservable>&&) source_rvalue, XRX_RVALUE(PipeConnect&&) connect)
    requires requires
    {
        // { XRX_MOV(connect)(XRX_MOV(source_rvalue)) } -> ::xrx::detail::ConceptObservable;
        XRX_MOV(connect)(XRX_MOV(source_rvalue));
    }
{
    XRX_CHECK_RVALUE(source_rvalue);
    XRX_CHECK_RVALUE(connect);
    return XRX_MOV(connect)(XRX_MOV(source_rvalue));
}

namespace xrx::detail
{
    struct PipeFold_
    {
        template<typename O>
        auto operator()(O&& source)
        {
            XRX_CHECK_RVALUE(source);
            return XRX_MOV(source);
        }
        template<typename O, typename F, typename... Fs>
        auto operator()(O&& source, F&& f, Fs&&... fs)
        {
            XRX_CHECK_RVALUE(source);
            XRX_CHECK_RVALUE(f);
            return (*this)(XRX_MOV(f)(XRX_MOV(source)), XRX_MOV(fs)...);
        }
    };
} // namespace xrx::detail

template<typename F, typename... Fs>
auto pipe(XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs)
{
    using PipeFold = ::xrx::detail::PipeFold_;
    return [_f = XRX_MOV(f), ..._fs = XRX_MOV(fs)](XRX_RVALUE(auto&&) source) mutable
    {
        return PipeFold()(XRX_MOV(_f)(XRX_MOV(source)), XRX_MOV(_fs)...);
    };
}

template<typename SourceObservable, typename F, typename... Fs>
auto pipe(XRX_RVALUE(::xrx::detail::Observable_<SourceObservable>&&) source
    , XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs)
{
    using PipeFold = ::xrx::detail::PipeFold_;
    return PipeFold()(XRX_MOV(f)(XRX_MOV(source)), XRX_MOV(fs)...);
}
