#include "operators/operator_take.h"
#include "noop_archetypes.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;
using xrx::detail::operator_tag::Take;

TEST(Compile, Take)
{
    auto o = make_operator(Take(), Noop_Observable<int, int>(), 0);
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, TakePipe)
{
    using O = Observable_<Noop_Observable<int, int>>;
    auto o = O() | take(1);
    static_assert(ConceptObservable<decltype(o)>);
}

