#include "operators/operator_transform.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Transform;

TEST(Compile, Transform)
{
    auto transform = [](int v) { return v; };
    auto o = make_operator(Transform(), Noop_Observable<int, void>(), transform);
    static_assert(ConceptObservable<decltype(o)>);
}
