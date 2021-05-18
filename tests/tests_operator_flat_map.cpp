#include "observables_factory.h"
#include "operators/operator_flat_map.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"
#include "debug/assert_mutex.h"
#include "subject.h"

#include <string>
#include <thread>
#include <algorithm>

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;

TEST(FlatMap, CollapseSingleRangeWithSingleElement)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1)
        | flat_map([](int v) { return observable::from(int(v)); })
        | subscribe(observer.ref());
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
        | subscribe(observer.ref());
}

TEST(FlatMap, WithMap)
{
    using pair = std::tuple<int, int>;
    ObserverMockAny<pair> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(pair(1, 1 * 2))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(1, 1 * 3))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, 2 * 2))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, 2 * 3))).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1, 2)
        | flat_map([](int source)
        {
            return observable::from(source * 2, source * 3);
        }
            , [](int source, int inner)
        {
            return pair(source, inner);
        })
        | subscribe(observer.ref());
}

TEST(FlatMap, WithMap_DifferentTypes)
{
    using pair = std::tuple<int, std::string>;
    ObserverMockAny<pair> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(pair(1, "2"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(1, "3"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, "4"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, "6"))).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(1, 2)
        | flat_map([](int source)
        {
            return observable::from(
                  std::to_string(source * 2)
                , std::to_string(source * 3));
        }
        , [](int source, std::string inner)
        {
            return pair(source, inner);
        })
        | subscribe(observer.ref());
}

TEST(FlatMap, SyncSource_WithAsyncProducer)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<int> inner;
    auto unsubscriber = observable::from(1)
        | flat_map([&](int)
    {
        return inner.as_observable();
    })
        | subscribe(observer.ref());
    
    inner.on_next(42);
    inner.on_next(43);
    unsubscriber.detach();
    inner.on_next(44);
    // Unsubscribed. Should not be called.
    inner.on_completed();
}

TEST(FlatMap, SyncSource_WithAsyncProducer_MultipleChildren)
{
    ObserverMockAny<std::string> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next("42")).InSequence(s);
    EXPECT_CALL(observer, on_next("43")).InSequence(s);
    EXPECT_CALL(observer, on_next("47")).InSequence(s);
    EXPECT_CALL(observer, on_next("48")).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<std::string> inner1;
    Subject_<std::string> inner2;
    auto unsubscriber = observable::from(1, 2)
        | flat_map([&](int v)
    {
        if (v == 1)
        {
            return inner1.as_observable();
        }
        return inner2.as_observable();
    })
        | subscribe(observer.ref());
    
    inner1.on_next("42");
    inner1.on_next("43");
    inner1.on_completed();

    inner2.on_next("47");
    inner2.on_next("48");
    inner2.on_completed();
}

TEST(FlatMap, SyncSource_WithAsyncProducer_Interleaving)
{
    using pair = std::tuple<int, std::string>;
    ObserverMockAny<pair> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(pair(1, "42"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, "47"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(1, "43"))).InSequence(s);
    EXPECT_CALL(observer, on_next(pair(2, "48"))).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<std::string> inner1;
    Subject_<std::string> inner2;
    auto unsubscriber = observable::from(1, 2)
        | flat_map([&](int v)
    {
        if (v == 1)
        {
            return inner1.as_observable();
        }
        return inner2.as_observable();
    }
        , [](int source, std::string inner)
    {
        return pair(source, inner);
    })
        | subscribe(observer.ref());
    
    inner1.on_next("42");
    inner2.on_next("47");
    inner1.on_next("43");
    inner2.on_next("48");

    inner2.on_completed();
    inner1.on_completed();
}

TEST(FlatMap, SyncSource_WithAsyncProducer_ParallelEmits_Are_Serialized)
{
    Subject_<int> inner1;
    Subject_<int> inner2;
    std::vector<int> observed;
    std::size_t completed_count = 0;
    std::size_t size_on_complete = 0;
    debug::AssertMutex lock_assert;

    auto unsubscriber = observable::from(1, 2)
        | flat_map([&](int v)
    {
        if (v == 1)
        {
            return inner1.as_observable();
        }
        return inner2.as_observable();
    })
        | subscribe(observer::make([&](int v)
    {
        auto lock = std::lock_guard(lock_assert);
        observed.push_back(v);
    }
        , [&]()
    {
        auto lock = std::lock_guard(lock_assert);
        ++completed_count;
        size_on_complete = observed.size();
    }));

    // Spin separate thread.
    std::thread worker1([&]()
    {
        for(int i = 0; i < 1000; ++i)
        {
            inner1.on_next(int(i));
        }
        inner1.on_completed();
    });
    // Emit values in parallel.
    for (int i = 1000; i < 2000; ++i)
    {
        inner2.on_next(int(i));
    }
    inner2.on_completed();
    worker1.join();

    // Output is some permutation of [0; 2000) range.
    std::vector<int> expected;
    for (int i = 0; i < 2000; ++i)
    {
        expected.push_back(i);
    }

    // Final complete was called once.
    ASSERT_EQ(std::size_t(1), completed_count);
    // And when complete was invoked, we had all values already emited.
    ASSERT_EQ(expected.size(), size_on_complete);
    ASSERT_EQ(expected.size(), observed.size());
    const bool all_observed = std::is_permutation(
        std::begin(expected), std::end(expected)
        , std::begin(observed));
    ASSERT_TRUE(all_observed);
}

TEST(FlatMap, AsyncSource_SyncInner)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(61)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(62)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<int> source;

    source.as_observable().flat_map([](int v)
    {
        return observable::from(int(v), 60 + v);
    })
        .subscribe(observer.ref());

    source.on_next(1);
    source.on_next(2);
    source.on_completed();
}
