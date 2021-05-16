#include "operators/operator_subscribe_on.h"
#include "noop_archetypes.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;
using xrx::detail::operator_tag::SubscribeOn;

TEST(Compile, SubscribeOn)
{
    auto o = make_operator(SubscribeOn(), Noop_Observable<int, int>(), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}
