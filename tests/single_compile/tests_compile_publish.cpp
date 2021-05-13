#include "operators/operator_publish.h"
#include "tests/noop_archetypes.h"
#include "observable_interface.h"
#include <gtest/gtest.h>

using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Publish;

TEST(Compile, Publish)
{
    auto o = make_operator(Publish(), Noop_Observable<int, int>());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, PublishPipe)
{
    using O = Observable_<Noop_Observable<int, int>>;
    auto o = O() | publish();
    (void)o.fork().subscribe(Noop_Observer<int, int>());
    static_assert(ConceptObservable<decltype(o)>);
}
