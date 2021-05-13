#include "observables_factory.h"
#include "operators/operator_concat.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"

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

TEST(Concat, ConcatTwoStreams_WithSingleNumber)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    auto o1 = observable::range(1, 1); // [1, 1]
    auto o2 = observable::range(2, 2); // [2, 2]
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2]
    static_assert(ConceptObservable<decltype(merged)>);

    merged.fork().subscribe(observer);
}

TEST(Concat, ConcatTwoStreams_ProcessStreamsInOrder)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(10)).InSequence(s);
    EXPECT_CALL(observer, on_next(11)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    auto o1 = observable::range(1).take(2);  // [1, 2]
    auto o2 = observable::range(10).take(2); // [10, 11]
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2, 10, 11]
    merged.fork().subscribe(observer);
}

TEST(Concat, CanStopWhileProcessingFirstStream)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(1));

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    auto o1 = observable::range(1); // infinity
    auto o2 = observable::range(10);// infinity
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2, 10, 11]
    merged.fork().subscribe(observer::make(
          [&](int v) { observer.on_next(v); return unsubscribe(true); }
        , [&]() { observer.on_completed(); }
        , [&]() { observer.on_error(); }));
}
