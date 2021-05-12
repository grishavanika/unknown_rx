#include "observables_factory.h"
#include "operators/operator_range.h"
#include "utils_observers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace testing;

using namespace xrx;

struct ObserverMock
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, ());
};

TEST(Range, WhenBoundariesAreEqual_InvokedOnce)
{
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0)
            .subscribe(observer);
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0, 1)
            .subscribe(observer);
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error()).Times(0);

        observable::range(0, 0, -1)
            .subscribe(observer);
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
        .subscribe(observer);
}

TEST(Range, BoundariesAreInclusive_DirectionLeft)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(_)).Times(11).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(10, 0, -1)
        .subscribe(observer);
}
