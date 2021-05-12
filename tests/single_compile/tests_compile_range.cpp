#include "operators/operator_range.h"
#include "utils_observers.h"
#include "tests/noop_archetypes.h"
#include <gtest/gtest.h>

using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Range;

TEST(Compile, Range_Constructor)
{
    auto o = make_operator(Range(), 0, 0, 0, std::true_type());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, Range_Subscribe_OnlyOnNext)
{
    auto o = make_operator(Range(), 0, 0, 0, std::true_type());
    o.fork().subscribe([](int) {});
}

TEST(Compile, Range_Subscribe_OnNextOnCompleted)
{
    auto o = make_operator(Range(), 0, 0, 0, std::true_type());
    o.fork().subscribe(observer::make([](int) {}, [] {}));
}
