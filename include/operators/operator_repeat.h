#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
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
    struct RepeatObservable
    {
        static_assert((not IsSourceAsync)
            , "Async Observable .repeat() is not implemented yet.");

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using Unsubscriber = NoopUnsubscriber;
        using is_async = std::bool_constant<IsSourceAsync>;

        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
        static_assert(IsSourceAsync
            or (not SourceUnsubscriber::has_effect::value)
            , "If Observable is Sync, its unsubscriber should have no effect.");

        SourceObservable _source;
        std::size_t _max_repeats;

        struct SyncState_
        {
            using Values = std::vector<value_type>;

            std::optional<Values> _values;
            bool _unsubscribed = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct SyncObserver_
        {
            SyncState_* _state = nullptr;
            Observer* _observer = nullptr;
            std::size_t _max_repeats = 0;

            OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                if (_state->_unsubscribed)
                {
                    assert(false && "Call to on_next() even when unsubscribe() was requested.");
                    return OnNextAction{._stop = true};
                }
                const auto action = on_next_with_action(*_observer, XRX_MOV(v));
                if (action._stop)
                {
                    _state->_unsubscribed = true;
                    return action;
                }
                // Remember values if only need to be re-emitted.
                if (_max_repeats > 1)
                {
                    if (!_state->_values)
                    {
                        _state->_values.emplace();
                    }
                    _state->_values->push_back(XRX_FWD(v));
                }
                return OnNextAction();
            }

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if (not _state->_unsubscribed)
                {
                    // Trigger an error there to avoid a need to remember it.
                    if constexpr ((sizeof...(e)) == 0)
                    {
                        on_error_optional(XRX_MOV(*_observer));
                    }
                    else
                    {
                        on_error_optional(XRX_MOV(*_observer), XRX_MOV(e)...);
                    }
                    _state->_unsubscribed = true;
                }
                else
                {
                    assert(false && "Call to on_error() even when unsubscribe() was requested.");
                }
            }

            void on_completed()
            {
                if (not _state->_unsubscribed)
                {
                    _state->_completed = true;
                }
                else
                {
                    assert(false && "Call to on_completed() even when unsubscribe() was requested.");
                }
            }
        };

        // Synchronous version.
        template<typename Observer>
        NoopUnsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            if (_max_repeats == 0)
            {
                (void)on_completed_optional(XRX_MOV(observer));
                return NoopUnsubscriber();
            }
            using ObserverImpl = SyncObserver_<std::remove_reference_t<Observer>>;
            SyncState_ state;
            // Ignore unsubscriber since it should haven no effect since we are synchronous.
            (void)XRX_MOV(_source).subscribe(ObserverImpl(&state, &observer, _max_repeats));
            if (state._unsubscribed)
            {
                // Either unsubscribed or error.
                return NoopUnsubscriber();
            }
            assert(state._completed && "Sync. Observable is not completed after .subscribe() end.");
            if (state._values)
            {
                // Emit all values (N - 1) times by copying from what we remembered.
                // Last emit iteration will move all the values.
                for (std::size_t i = 1; i < _max_repeats - 1; ++i)
                {
                    for (const value_type& v : *state._values)
                    {
                        const auto action = on_next_with_action(observer
                            , value_type(v)); // copy.
                        if (action._stop)
                        {
                            return NoopUnsubscriber();
                        }
                    }
                }

                // Move values, we don't need them anymore.
                for (value_type& v : *state._values)
                {
                    const auto action = on_next_with_action(observer, XRX_MOV(v));
                    if (action._stop)
                    {
                        return NoopUnsubscriber();
                    }
                }
            }
            on_completed_optional(XRX_MOV(observer));
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
        using IsAsync_ = IsAsyncObservable<SourceObservable>;
        using Impl = RepeatObservable<SourceObservable, Endless, IsAsync_::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
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
