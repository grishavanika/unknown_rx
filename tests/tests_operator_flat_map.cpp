#include "observables_factory.h"
#include "operators/operator_flat_map.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace testing;

using namespace xrx;
using namespace xrx::detail;

struct ObserverMock
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, ());
};

TEST(FlatMap, CollapseSingleRangeWithSingleElement)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1)
        | flat_map([](int v) { return observable::from(v); })
        | subscribe(observer::ref(observer));
}

TEST(FlatMap, TerminateAfterSingleElementEmit)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(5)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1, 2, 3)
        | flat_map([](int) { return observable::from(5, 2, 3); })
        | take(1)
        | subscribe(observer::ref(observer));
}