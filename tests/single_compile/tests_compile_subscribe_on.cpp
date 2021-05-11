#include "operators/operator_subscribe_on.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::SubscribeOn;

TEST(Compile, SubscribeOn)
{
    auto o = make_operator(SubscribeOn(), Noop_Observable<int, int>(), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}
