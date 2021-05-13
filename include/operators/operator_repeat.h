#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <optional>
#include <vector>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable, bool Endless, bool EndsInUnsubscribe>
    struct RepeatObservable
    {
        static_assert(EndsInUnsubscribe
            , "Asynchronous Observable .repeat() is not implemented yet.");

        struct NoUnsubscription
        {
            using has_effect = std::false_type;
            constexpr bool detach() const noexcept { return false; }
        };

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
        using Unsubscriber = NoUnsubscription;

        static_assert(!EndsInUnsubscribe
            || !SourceUnsubscriber::has_effect::value
            , "If Observable is synchronous, its unsubscriber should have no effect.");

        using ends_in_subscribe = std::bool_constant<EndsInUnsubscribe>;

        SourceObservable _source;
        std::size_t _max_repeats;

        struct SyncState_
        {
            // #XXX: what if value_type is reference ?
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

            OnNextAction on_next(value_type v)
            {
                if (_state->_unsubscribed)
                {
                    assert(false && "Call to on_next() even when unsubscribe() was requested.");
                    return OnNextAction{._unsubscribe = true};
                }
                const auto action = on_next_with_action(*_observer, XRX_FWD(v));
                if (action._unsubscribe)
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

            void on_error(error_type e)
            {
                if (not _state->_unsubscribed)
                {
                    // Trigger an error there to avoid a need to remember it.
                    on_error_optional(XRX_MOV(*_observer), XRX_FWD(e));
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
        NoUnsubscription subscribe(Observer&& observer) &&
        {
            if (_max_repeats == 0)
            {
                (void)on_completed_optional(XRX_FWD(observer));
                return NoUnsubscription();
            }
            using ObserverImpl = SyncObserver_<std::remove_reference_t<Observer>>;
            SyncState_ state;
            // Ignore unsubscriber since it should haven no effect since we are synchronous.
            (void)std::move(_source).subscribe(ObserverImpl(&state, &observer, _max_repeats));
            if (state._unsubscribed)
            {
                // Either unsubscribed or error.
                return NoUnsubscription();
            }
            assert(state._completed && "Sync. Observable is not completed after .subscribe() end.");
            if (state._values)
            {
                for (std::size_t i = 1; i < _max_repeats; ++i)
                {
                    for (const value_type& v : *state._values)
                    {
                        const auto action = on_next_with_action(observer, v);
                        if (action._unsubscribe)
                        {
                            return NoUnsubscription();
                        }
                    }
                }
            }
            on_completed_optional(XRX_MOV(observer));
            return NoUnsubscription();
        }

        RepeatObservable fork() && { return RepeatObservable(std::move(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Repeat
        , SourceObservable source, std::size_t count, std::bool_constant<Endless>)
    {
        using EndsInUnsubscribe = decltype(detect_ends_in_subscribe<SourceObservable>());
        using Impl = RepeatObservable<SourceObservable, Endless, EndsInUnsubscribe::value>;
        return Observable_<Impl>(Impl(std::move(source), count));
    }
} // namespace xrx::detail
