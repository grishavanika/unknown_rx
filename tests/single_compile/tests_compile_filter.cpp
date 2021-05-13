#include "operators/operator_filter.h"
#include "tests/noop_archetypes.h"
#include "observable_interface.h"
#include <gtest/gtest.h>

using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Filter;

TEST(Compile, Filter)
{
    auto filter = [](int) { return false; };
    auto o = make_operator(Filter(), Noop_Observable<int, void>(), filter);
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, FilterPipe)
{
    using O = Observable_<Noop_Observable<int, void>>;
    auto o = O() | filter([](int) { return false; });
    (void)o.fork().subscribe(Noop_Observer<int, void>());
    static_assert(ConceptObservable<decltype(o)>);
}
