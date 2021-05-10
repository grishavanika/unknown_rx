#pragma once
#include <type_traits>

namespace xrx
{
    struct NoopUnsubscriber
    {
        using has_effect = std::false_type;
        constexpr bool detach() const noexcept { return false; }
    };
} // namespace xrx
