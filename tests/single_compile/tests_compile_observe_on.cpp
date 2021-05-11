#include "operators/operator_observe_on.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::ObserveOn;

TEST(Compile, ObserveOn)
{
    auto o = make_operator(ObserveOn(), Noop_Observable<int, int>(), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}
