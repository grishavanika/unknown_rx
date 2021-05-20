#include "operators/operator_iterate.h"
#include "observables_factory.h"
#include "utils_observers.h"
#include "debug_copy_move.h"

#include <array>

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;

TEST(Iterate, EmptyVector)
{
    ObserverMock observer;
    EXPECT_CALL(observer, on_next(_)).Times(0);
    EXPECT_CALL(observer, on_completed());
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::iterate(std::vector<int>())
        .subscribe(observer.ref());
}

TEST(Iterate, Array)
{
    ObserverMock observer;
    Sequence s;
    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    std::array<int, 3> vs{{1, 2, 3}};
    observable::iterate(XRX_MOV(vs))
        .subscribe(observer.ref());
}
