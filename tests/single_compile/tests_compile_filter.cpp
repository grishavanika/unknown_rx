#include "operators/operator_filter.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Filter;

TEST(Compile, Filter)
{
    auto filter = [](int) { return false; };
    auto o = make_operator(Filter(), Noop_Observable<int, void>(), filter);
    static_assert(ConceptObservable<decltype(o)>);
}
