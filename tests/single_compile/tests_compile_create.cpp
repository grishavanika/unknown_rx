#include "operators/operator_create.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Create;

TEST(Compile, Create)
{
    auto on_subscribe = [](auto) {};
    auto o = make_operator(Create<int, int>(), on_subscribe);
    static_assert(ConceptObservable<decltype(o)>);
}
