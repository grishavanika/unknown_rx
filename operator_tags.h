#pragma once
#include "meta_utils.h"

namespace xrx::detail::operator_tag
{
    struct Publish
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .publish() operator implementation. "
                  "Missing `operators/operator_publish.h` include ?");
        };
    };

    struct Filter
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .filter() operator implementation. "
                  "Missing `operators/operator_filter.h` include ?");
        };
    };

    struct Transform
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .transform() operator implementation. "
                  "Missing `operators/operator_transform.h` include ?");
        };
    };

    struct Interval
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .interval() operator implementation. "
                  "Missing `operators/operator_interval.h` include ?");
        };
    };
} // namespace xrx::detail::operator_tag
