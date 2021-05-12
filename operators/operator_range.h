#pragma once
#include "operator_tags.h"
#include "cpo_make_operator.h"
#include "observable_interface.h"
#include "utils_observers.h"
#include <utility>
#include <type_traits>
#include <cstdint>

namespace xrx::detail
{
    template<typename Integer>
    struct RangeObservable
    {
        struct NoUnsubscription
        {
            using has_effect = std::false_type;
            constexpr bool detach() const noexcept { return false; }
        };

        using value_type   = Integer;
        using error_type   = none_tag;
        using Unsubscriber = NoUnsubscription;


        const Integer _first;
        const Integer _last;
        const std::intmax_t _step;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            Integer value = _first;
            while (true)
            {
                const OnNextAction action = ::xrx::detail::on_next_with_action(observer, Integer(value));
                if (action._unsubscribe)
                {
                    break;
                }
                value = Integer(std::intmax_t(value) + _step);
            }
            return Unsubscriber();
        }

        RangeObservable fork()
        {
            return *this;
        }
    };

    template<typename Integer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Range
        , Integer first, Integer last, std::intmax_t step)
    {
        using Impl = RangeObservable<Integer>;
        return Observable_<Impl>(Impl(first, last, step));
    }
} // namespace xrx::detail
