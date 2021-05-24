#pragma once
#include <type_traits>

namespace xrx::detail
{
    struct NoopDetach
    {
        using has_effect = std::false_type;
        constexpr bool operator()() const noexcept { return false; }
    };
} // namespace xrx::detail
