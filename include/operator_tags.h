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

    struct SubscribeOn
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .subscribe_on() operator implementation. "
                  "Missing `operators/operator_subscribe_on.h` include ?");
        };
    };

    struct ObserveOn
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .observe_on() operator implementation. "
                  "Missing `operators/operator_observe_on.h` include ?");
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

    template<typename Value, typename Error>
    struct Create
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .create() operator implementation. "
                  "Missing `operators/operator_create.h` include ?");
        };
    };

    struct Range
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .range() operator implementation. "
                  "Missing `operators/operator_range.h` include ?");
        };
    };

    struct Take
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .take() operator implementation. "
                  "Missing `operators/operator_take.h` include ?");
        };
    };

    struct From
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .from() operator implementation. "
                  "Missing `operators/operator_from.h` include ?");
        };
    };

    struct Repeat
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .repeat() operator implementation. "
                  "Missing `operators/operator_repeat.h` include ?");
        };
    };

    struct Concat
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .concat() operator implementation. "
                  "Missing `operators/operator_concat.h` include ?");
        };
    };

    struct FlatMap
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .flat_map() operator implementation. "
                  "Missing `operators/operator_flat_map.h` include ?");
        };
    };

    struct Window
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .window() operator implementation. "
                  "Missing `operators/operator_window.h` include ?");
        };
    };

    struct Reduce
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .reduce() operator implementation. "
                  "Missing `operators/operator_reduce.h` include ?");
        };
    };

    struct Iterate
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .iterate() operator implementation. "
                  "Missing `operators/operator_iterate.h` include ?");
        };
    };

    struct TapOrDo
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .tap()/.do_() operator implementation. "
                  "Missing `operators/operator_tap_do.h` include ?");
        };
    };

    struct WindowToggle
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .window_toggle() operator implementation. "
                  "Missing `operators/operator_window_toggle.h` include ?");
        };
    };

    struct StartsWith
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .start_with() operator implementation. "
                  "Missing `operators/operator_start_with.h` include ?");
        };
    };

} // namespace xrx::detail::operator_tag
