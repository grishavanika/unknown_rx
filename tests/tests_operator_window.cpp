#include "operators/operator_window.h"
#include "operators/operator_from.h"
#include "operators/operator_as_async.h"
#include "operators/operator_flat_map.h"
#include "observables_factory.h"
#include "observable_interface.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace ::xrx;
using namespace ::xrx::detail;

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

    EXPECT_CALL(observer, on_error(_)).Times(0);

    int windows_count = 0;
    observable::from(1, 2, 3, 4)
        | window(2)
        | subscribe([&](auto part)
    {
        ++windows_count;
        part.fork().subscribe(observer.ref());
    });
    // See _Async test below for a difference.
    ASSERT_EQ(2, windows_count);
}

TEST(Window, SimpleSequence_SplitInTwo_Async)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    // 3rd sequence is started, but completed immediately,
    // because source stream completed.
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> producer;
    int windows_count = 0;
    producer.as_observable()
        | window(2)
        | subscribe([&](auto part)
    {
        ++windows_count;
        part.fork().subscribe(observer.ref());
    });
    // First window is fired immediately;
    // difference in behavior to Sync source Observable.
    ASSERT_EQ(1, windows_count);

    producer.on_next(1);
    producer.on_next(2);
    // First window completed, next one is triggered.
    ASSERT_EQ(2, windows_count);

    producer.on_next(3);
    producer.on_next(4);
    ASSERT_EQ(3, windows_count);

    producer.on_completed();
}

TEST(Window, SimpleSequence_SplitInTwo_Async_FromSync)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    // 3rd sequence is started, but completed immediately,
    // because source stream completed.
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    EXPECT_CALL(observer, on_error(_)).Times(0);

    
    int windows_count = 0;
    observable::from(1, 2, 3, 4)
        | as_async()
        | window(2)
        | subscribe([&](auto part)
    {
        ++windows_count;
        part.fork().subscribe(observer.ref());
    });
    ASSERT_EQ(3, windows_count);
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

    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2, 3, 4)
        | window(3)
        | subscribe([&](auto part)
    {
        part.fork().subscribe(observer.ref());
    });
}

TEST(Window, FlatMap_ConvertsToOriginalSequence)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2, 3, 4)
        | window(2)
        | flat_map([](auto part)
        {
            return std::move(part);
        })
        | subscribe(observer.ref());
}
