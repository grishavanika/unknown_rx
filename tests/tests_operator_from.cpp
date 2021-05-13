#include "observables_factory.h"
#include "operators/operator_from.h"
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

TEST(From, SingleElement_InvokedOnce)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        .subscribe(observer);
}

TEST(From, AllRangePassedToOnNext)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(45)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(46)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(47)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42, 43, 44, 45, 46, 47)
        .subscribe(observer);
}
