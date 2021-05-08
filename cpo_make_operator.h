#pragma once
#include "tag_invoke.hpp"

#include <utility>

namespace xrx::detail
{
    struct make_operator_fn
    {
        template<typename Tag, typename... Args>
        constexpr decltype(auto) operator()(Tag&& tag, Args&&... args) const
            noexcept(noexcept(tag_invoke(*this, std::forward<Tag>(tag), std::forward<Args>(args)...)))
                requires tag_invocable<make_operator_fn, Tag, Args...>
        {
            return tag_invoke(*this, std::forward<Tag>(tag), std::forward<Args>(args)...);
        }

        template<typename Tag, typename... Args>
        constexpr decltype(auto) operator()(Tag, Args&&...) const noexcept
            requires (not tag_invocable<make_operator_fn, Tag, Args...>)
        {
            using NotFound = typename Tag::template NotFound<int/*any placeholder type*/>;
            return NotFound();
        }
    };

    inline const make_operator_fn make_operator;
} // namespace xrx::detail
