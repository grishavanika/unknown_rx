#include "operators/operator_reduce.h"
#include "operators/operator_from.h"
#include "observables_factory.h"
#include "subject.h"
#include "noop_archetypes.h"
#include "observer_mock.h"
#include "debug_copy_move.h"
#include <gtest/gtest.h>
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Transform;

TEST(Reduce, SimpleSumFold)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(45));
    EXPECT_CALL(observer, on_completed());
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2, 3, 4, 5, 6, 7, 8, 9)
        .reduce(int(0), std::plus<>())
        .subscribe(observer.ref());
}
