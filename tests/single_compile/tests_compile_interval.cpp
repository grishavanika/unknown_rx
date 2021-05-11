#include "operators/operator_interval.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Interval;

TEST(Compile, Interval)
{
    auto o = make_operator(Interval(), std::chrono::seconds(1), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}
