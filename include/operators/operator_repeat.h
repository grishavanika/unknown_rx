#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "utils_ref_tracking_observer.h"
#include "xrx_prologue.h"
#include <utility>
#include <mutex>
#include <type_traits>
#include <variant>
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
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;
        using SourceDetach = typename SourceObservable::DetachHandle;
        static_assert(not SourceDetach::has_effect::value
            , "Sync Observable's Detach should have no effect.");

        SourceObservable _source;
        int _max_repeats;

        template<typename Observer>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using RefObserver_ = RefTrackingObserver_<Observer, false/*do not call on_complete*/>;

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
                (void)_source.fork()
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopDetach();
                }
            }
            if (_max_repeats > 0)
            {
                // Last loop - move-forget source Observable.
                RefObserverState state;
                (void)_source.fork_move() // Difference.
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopDetach();
                }
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return NoopDetach();
        }

        RepeatObservable fork() && { return RepeatObservable(XRX_MOV(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    struct RepeatObservable<SourceObservable, Endless, true/*Async*/>
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using SourceDetach = typename SourceObservable::DetachHandle;
        using is_async = std::true_type;

        SourceObservable _source;
        int _max_repeats;

        struct DetachBlock
        {
            std::recursive_mutex _mutex;
            SourceDetach _source;
            bool _do_detach = false;
        };

        struct Detach
        {
            using has_effect = std::true_type;
            using State = std::variant<std::monostate
                , SourceDetach
                , std::shared_ptr<DetachBlock>>;
            State _state;
            explicit Detach() = default;
            explicit Detach(XRX_RVALUE(SourceDetach&&) source_detach)
                : _state(XRX_MOV(source_detach))
            {}
            explicit Detach(XRX_RVALUE(std::shared_ptr<DetachBlock>&&) block)
                : _state(XRX_MOV(block))
            {}

            bool operator()()
            {
                auto state = std::exchange(_state, {});
                if (std::get_if<std::monostate>(&state))
                {
                    return false;
                }
                else if (auto* source_detach = std::get_if<SourceDetach>(&state))
                {
                    return (*source_detach)();
                }
                else if (auto* block_ptr = std::get_if<std::shared_ptr<DetachBlock>>(&state))
                {
                    DetachBlock& block = **block_ptr;
                    auto guard = std::lock_guard(block._mutex);
                    block._do_detach = true;
                    return block._source();
                }
                assert(false);
                return false;
            }
        };

        using DetachHandle = Detach;

        template<typename Observer>
        struct Shared_
        {
            Observer _observer;
            SourceObservable _source;
            DetachBlock _detach;
            int _max_repeats;
            int _repeats;
            explicit Shared_(XRX_RVALUE(Observer&&) observer, XRX_RVALUE(SourceObservable&&) source, int max_repeats)
                : _observer(XRX_MOV(observer))
                , _source(XRX_MOV(source))
                , _detach()
                , _max_repeats(max_repeats)
                , _repeats(0)
            {
            }
            std::recursive_mutex& mutex()
            {
                return _detach._mutex;
            }

            struct OneRepeatObserver
            {
                std::shared_ptr<Shared_> _shared;

                auto on_next(value_type&& v)
                {
                    auto guard = std::lock_guard(_shared->mutex());
                    return on_next_with_action(_shared->_observer, XRX_FWD(v));
                }

                XRX_FORCEINLINE() void on_completed()
                {
                    auto self = _shared;
                    self->try_subscribe_next(XRX_MOV(_shared));
                }

                void on_error(XRX_RVALUE(error_type&&) e)
                {
                    auto guard = std::lock_guard(_shared->mutex());
                    on_error_optional(XRX_MOV(_shared->_observer), XRX_MOV(e));
                }
            };

            static void try_subscribe_next(std::shared_ptr<Shared_>&& self)
            {
                auto guard = std::lock_guard(self->mutex());
                if (self->_detach._do_detach)
                {
                    // Just unsubscribed.
                    return;
                }

                auto repeat_copy = [&]()
                {
                    auto copy = self;
                    // Repeat-on-the-middle, copy source.
                    self->_detach._source = self->_source.fork()
                        .subscribe(OneRepeatObserver(XRX_MOV(copy)));
                };

                if constexpr (Endless)
                {
                    repeat_copy();
                }
                else
                {
                    ++self->_repeats;
                    if (self->_repeats > self->_max_repeats)
                    {
                        // Done.
                        (void)on_completed_optional(XRX_MOV(self->_observer));
                        return;
                    }
                    if (self->_repeats == self->_max_repeats)
                    {
                        // Last repeat, move-destroy source.
                        auto copy = self;
                        self->_detach._source = self->_source.fork_move()
                            .subscribe(OneRepeatObserver(XRX_MOV(copy)));
                    }
                    else
                    {
                        assert(self->_repeats < self->_max_repeats);
                        repeat_copy();
                    }
                }
            }
        };

        template<typename Observer>
        Detach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using Shared = Shared_<Observer>;
            if (not Endless)
            {
                if (_max_repeats == 0)
                {
                    (void)on_completed_optional(XRX_MOV(observer));
                    return Detach();
                }
                if (_max_repeats == 1)
                {
                    auto source_detach = XRX_MOV(_source).subscribe(XRX_MOV(observer));
                    return Detach(XRX_MOV(source_detach));
                }
            }
            // Endless or with repeats >= 2. Need to create shared state.
            auto shared = std::make_shared<Shared>(XRX_MOV(observer), XRX_MOV(_source), _max_repeats);
            std::shared_ptr<DetachBlock> DetachHandle(shared, &shared->_detach);
            auto self = shared;
            shared->try_subscribe_next(XRX_MOV(self));
            return Detach(XRX_MOV(DetachHandle));
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
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        using IsAsync_ = IsAsyncObservable<SourceObservable>;
        using Impl = RepeatObservable<SourceObservable, Endless, IsAsync_::value>;
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
                , XRX_MOV(source), 0, std::true_type());
        };
    }

    inline auto repeat(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Repeat()
                , XRX_MOV(source), _count, std::false_type());
        };
    }
} // namespace xrx

#include "xrx_epilogue.h"
