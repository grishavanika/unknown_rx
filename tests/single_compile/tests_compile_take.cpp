#include "operators/operator_take.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Take;

TEST(Compile, Take)
{
    auto o = make_operator(Take(), Noop_Observable<int, int>(), 0);
    static_assert(ConceptObservable<decltype(o)>);
}
