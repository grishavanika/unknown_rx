#include "observables_factory.h"
#include "operators/operator_range.h"
#include "utils_observers.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;

using namespace xrx;

TEST(Range, WhenBoundariesAreEqual_InvokedOnce)
{
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0)
            .subscribe(observer.ref());
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0, 1)
            .subscribe(observer.ref());
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0, -1)
            .subscribe(observer.ref());
    }
}

TEST(Range, BoundariesAreInclusive_DirectionRight)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(_)).Times(11).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(0, 10)
        .subscribe(observer.ref());
}

TEST(Range, BoundariesAreInclusive_DirectionLeft)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(_)).Times(11).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(10, 0, -1)
        .subscribe(observer.ref());
}
