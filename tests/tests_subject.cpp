#include "subject.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "observer_mock.h"
#include "noop_archetypes.h"
#include "debug/new_allocs_stats.h"
#include "debug/malloc_allocator.h"
#include "debug/tracking_allocator.h"
using namespace testing;
using namespace xrx;
using namespace xrx::debug;
using namespace xrx::detail;
using namespace tests;

TEST(Subject, HappyPath)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

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

TEST(Subject, Subscribe_After_OnCompleted_Invokes_OnCompleted)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(3).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> subject;
    subject.subscribe(observer.ref());
    subject.on_next(1);
    subject.on_completed();
    
    subject.subscribe(observer.ref());
    subject.subscribe(observer.ref());
}

TEST(Subject, Subscribe_After_OnError_Invokes_OnError)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(3).InSequence(s);

    Subject_<int> subject;
    subject.subscribe(observer.ref());
    subject.on_next(1);
    subject.on_error(void_());

    subject.subscribe(observer.ref());
    subject.subscribe(observer.ref());
}


TEST(Subject, Subscribe_After_OnError_Invokes_OnError_WithSameError)
{
    ObserverMock_Error observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(-2)).Times(3).InSequence(s);

    Subject_<int, int> subject;
    subject.subscribe(observer.ref());
    subject.on_next(1);
    subject.on_error(-2);

    subject.subscribe(observer.ref());
    subject.subscribe(observer.ref());
}

TEST(Subject, OnNext_Ignore_AfterOnCompleted)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> subject;
    subject.subscribe(observer.ref());

    subject.on_next(1);
    subject.on_next(2);
    subject.on_completed();
    subject.on_next(-1);
    subject.on_next(-2);
}

TEST(Subject, OnNext_Ignore_AfterOnError)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).InSequence(s);

    Subject_<int> subject;
    subject.subscribe(observer.ref());

    subject.on_next(1);
    subject.on_next(2);
    subject.on_error(void_());
    subject.on_next(-1);
    subject.on_next(-2);
}

// Tests should not run in parallel.
TEST(Subject, Construct_Default_Allocates)
{
    NewAllocsStats::get().reset();
    Subject_<int> subject;
    (void)subject;
    ASSERT_NE(0, int(NewAllocsStats::get()._count));
}

TEST(Subject, Construct_WithCustomAlloc)
{
    using Alloc = TrackingAllocator<int, MallocAllocator<int>>;
    TrackingAllocatorState state;
    Alloc alloc(&state);

    NewAllocsStats::get().reset();
    Subject_<int, int, Alloc> subject(alloc);
    (void)subject;
    // No global new allocations.
    ASSERT_EQ(0, int(NewAllocsStats::get()._count));
    // All went to custom allocator.
    ASSERT_NE(0, int(state._allocs_count));
}

TEST(Subject, Construct_WithCustomAlloc_WithObserver)
{
    using Alloc = TrackingAllocator<int, MallocAllocator<int>>;
    TrackingAllocatorState state;

    Alloc alloc(&state);
    ObserverMock observer;

    NewAllocsStats::get().reset();
    Subject_<int, void_, Alloc> subject(alloc);
    subject.subscribe(observer.ref());
    // No global new allocations.
    ASSERT_EQ(0, int(NewAllocsStats::get()._count));
    // All went to custom allocator.
    ASSERT_NE(0, int(state._allocs_count));
}
