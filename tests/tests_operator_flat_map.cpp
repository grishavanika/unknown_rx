#include "observables_factory.h"
#include "operators/operator_flat_map.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"
#include "subject.h"

#include <string>

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
