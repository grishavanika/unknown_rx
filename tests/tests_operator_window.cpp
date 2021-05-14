#include "operators/operator_window.h"
#include "operators/operator_from.h"
#include "observables_factory.h"
#include "observable_interface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace testing;
using namespace ::xrx;
using namespace ::xrx::detail;

struct ObserverMock
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, ());
};

TEST(Window, SimpleSequence_SplitInTwo)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1, 2, 3, 4)
        | window(2)
        | subscribe([&](auto part)
    {
        part.fork().subscribe(observer::ref(observer));
    });
}


TEST(Window, SimpleSequence_EmitedAll_WhenFinalPartIsNotComplete)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1, 2, 3, 4)
        | window(3)
        | subscribe([&](auto part)
    {
        part.fork().subscribe(observer::ref(observer));
    });
}
