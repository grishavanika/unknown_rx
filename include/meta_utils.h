#pragma once
#include <type_traits>

namespace xrx::detail
{
    template<unsigned I>
    struct priority_tag
        : priority_tag<I - 1> {};
    template<>
    struct priority_tag<0> {};

    template<typename>
    struct AlwaysFalse : std::false_type {};
} // namespace xrx::detail

namespace xrx
{
    struct void_ {};
} // namespace xrx
