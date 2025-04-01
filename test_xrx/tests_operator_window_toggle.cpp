#include "operators/operator_window_toggle.h"
#include "operators/operator_flat_map.h"
#include "observable_interface.h"
#include "subject.h"
#include "noop_archetypes.h"
#include "observer_mock.h"
#include "debug_copy_move.h"
#include <gtest/gtest.h>
#include <cstdio>
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;

TEST(WindowToggle, Compile)
{
    Subject_<int> source;
    Subject_<int> openings;
    Subject_<int> closings;

    source.as_observable()
        .window_toggle(openings.as_observable()
            , [&](int open)
        {
            (void)open;
            return closings.as_observable();
        })
        .flat_map([](auto window)
        {
            return std::move(window);
        })
        .subscribe([](int v)
        {
            std::printf("%i\n", v);
        });

    source.on_next(-1);
    openings.on_next(0);
    source.on_next(1);
    source.on_next(2);
    closings.on_next(0);
    source.on_next(-2);
}
