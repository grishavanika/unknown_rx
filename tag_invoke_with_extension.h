#pragma once
#include "tag_invoke.hpp"

#include <utility>

namespace xrx::detail
{
    template<typename CPO, typename... Args>
    concept ConceptCPOInvocable = requires(CPO& cpo, Args&&... args)
    {
        cpo(std::forward<Args>(args)...);
    };

    template<typename CPO, typename... Args>
    constexpr bool is_cpo_invocable_v = ConceptCPOInvocable<CPO, Args...>;
} // namespace xrx::detail
