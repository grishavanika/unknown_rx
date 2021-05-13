#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include <utility>
#include <type_traits>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Integer, bool Endless>
    struct RangeObservable
    {
        using value_type   = Integer;
        using error_type   = none_tag;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        explicit RangeObservable(Integer first, Integer last, std::intmax_t step)
            : _first(first)
            , _last(last)
            , _step(step)
        {
            assert(((step >= 0) && (last  >= first))
                || ((step <  0) && (first >= last)));
        }

        const Integer _first;
        const Integer _last;
        const std::intmax_t _step;

        static bool compare_(Integer, Integer, std::intmax_t, std::true_type/*endless*/)
        {
            return true;
        }
        static bool compare_(Integer current, Integer last, std::intmax_t step, std::false_type/*endless*/)
        {
            if (step >= 0)
            {
                return (current <= last);
            }
            return (current >= last);
        }
        static Integer do_step_(Integer current, std::intmax_t step)
        {
            if (step >= 0)
            {
                return Integer(current + Integer(step));
            }
            return Integer(std::intmax_t(current) + step);
        }

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            constexpr std::bool_constant<Endless> _edless;
            Integer current = _first;
            while (compare_(current, _last, _step, _edless))
            {
                const OnNextAction action = on_next_with_action(observer, Integer(current));
                if (action._unsubscribe)
                {
                    return Unsubscriber();
                }
                current = do_step_(current, _step);
            }
            (void)on_completed_optional(std::forward<Observer>(observer));
            return Unsubscriber();
        }

        RangeObservable fork()
        {
            return *this;
        }
    };

    template<typename Integer, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Range
        , Integer first, Integer last, std::intmax_t step, std::bool_constant<Endless>)
    {
        using Impl = RangeObservable<Integer, Endless>;
        return Observable_<Impl>(Impl(first, last, step));
    }
} // namespace xrx::detail
