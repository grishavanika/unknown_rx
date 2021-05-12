#include "operators/operator_range.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Range;

TEST(Compile, Range)
{
    auto o = make_operator(Range(), 0, 0, 0, std::true_type());
    static_assert(ConceptObservable<decltype(o)>);
}
