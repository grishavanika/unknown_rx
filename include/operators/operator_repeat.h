#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_ref_tracking_observer.h"
#include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <optional>
#include <vector>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable, bool Endless, bool IsSourceAsync>
    struct RepeatObservable;

    template<typename SourceObservable, bool Endless>
    struct RepeatObservable<SourceObservable, Endless, false/*Sync*/>
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using Unsubscriber = NoopUnsubscriber;
        using is_async = std::false_type;

        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
        static_assert(not SourceUnsubscriber::has_effect::value
            , "Sync Observable's Unsubscriber should have no effect.");

        SourceObservable _source;
        int _max_repeats;

        template<typename Observer>
        NoopUnsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            using Observer_ = std::remove_reference_t<Observer>;
            using RefObserver_ = RefTrackingObserver_<Observer_, false/*do not call on_complete*/>;

            auto check_repeat = [this, index = 0]() mutable
            {
                if constexpr (Endless)
                {
                    return true;
                }
                else
                {
                    return (index++ < (_max_repeats - 1));
                }
            };
            // Iterate N - 1 times; last iteration is "optimization":
            // _source.fork_move() is used instead of _source.fork().
            while (check_repeat())
            {
                RefObserverState state;
                const auto unsubscriber = _source.fork()
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopUnsubscriber();
                }
            }
            if (_max_repeats > 0)
            {
                // Last loop - move-forget source Observable.
                RefObserverState state;
                const auto unsubscriber = _source.fork_move() // Difference.
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopUnsubscriber();
                }
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return NoopUnsubscriber();
        }

        RepeatObservable fork() && { return RepeatObservable(XRX_MOV(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Repeat
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count, std::bool_constant<Endless>)
            requires ConceptObservable<SourceObservable>
    {
        XRX_CHECK_RVALUE(source);
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using IsAsync_ = IsAsyncObservable<SourceObservable_>;
        using Impl = RepeatObservable<SourceObservable_, Endless, IsAsync_::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), int(count)));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto repeat()
    {
        return [](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Repeat()
                , XRX_MOV(source), 0, std::bool_constant<true/*endless*/>());
        };
    }

    inline auto repeat(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Repeat()
                , XRX_MOV(source), _count, std::bool_constant<false/*not endless*/>());
        };
    }
} // namespace xrx
