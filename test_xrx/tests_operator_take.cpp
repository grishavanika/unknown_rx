#include "operators/operator_take.h"
#include "operators/operator_create.h"
#include "observables_factory.h"
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

TEST(Take, One)
{
    int items_count = 0;
    observable::create<int>([](auto o)
    {
        for (int i : {42, 43, 44})
        {
            const auto action = on_next_with_action(o, int(i));
            if (action._stop)
            {
                return;
            }
        }
        (void)on_completed_optional(o);
    })
        .take(1)
        .subscribe([&](int)
    {
        ++items_count;
    });
    ASSERT_EQ(1, items_count);
}
