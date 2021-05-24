#include "subject.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "observer_mock.h"
#include "noop_archetypes.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;

TEST(Subject, HappyPath)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<int> subject;
    auto unsubscriber = subject.subscribe(observer.ref());
    ASSERT_TRUE(unsubscriber);

    subject.on_next(1);
    subject.on_next(2);
    subject.on_completed();
}

TEST(Subject, DoesntEmitAfterUnsubscribe)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<int> subject;
    auto DetachHandle = subject.subscribe(observer.ref());

    subject.on_next(1);
    subject.on_next(2);
    ASSERT_TRUE(DetachHandle);
    ASSERT_TRUE(DetachHandle());
    ASSERT_FALSE(DetachHandle);
    subject.on_next(42);
    subject.on_completed();
}

TEST(Subject, MT_Serialized)
{
    Subject_<int> subject;
    debug::AssertMutex assert_mutex;
    std::vector<int> observed;
    subject.as_observable().subscribe([&](int v)
    {
        // Must never fire.
        auto guard = std::lock_guard(assert_mutex);
        observed.push_back(v);
    });

    // Producer 1.
    std::thread worker([observer = subject.as_observer()]() mutable
    {
        for (int i = 0; i < 1'000; ++i)
        {
            observer.on_next(int(i));
        }
    });
    // Producer 2.
    for (int i = 1'000; i < 2'000; ++i)
    {
        subject.on_next(int(i));
    }
    worker.join();

    std::vector<int> expected;
    for (int i = 0; i < 2000; ++i)
    {
        expected.push_back(i);
    }
    ASSERT_EQ(expected.size(), observed.size());
    const bool all_observed = std::is_permutation(
          std::begin(expected), std::end(expected)
        , std::begin(observed));
    ASSERT_TRUE(all_observed);
}
