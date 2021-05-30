#include "observables_factory.h"
#include "operators/operator_concat.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"
#include "subject.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;

TEST(Concat, ConcatTwoStreams_WithSingleNumber)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    auto o1 = observable::range(1, 1); // [1, 1]
    auto o2 = observable::range(2, 2); // [2, 2]
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2]
    static_assert(ConceptObservable<decltype(merged)>);

    merged.fork().subscribe(observer.ref());
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
    EXPECT_CALL(observer, on_error(_)).Times(0);

    auto o1 = observable::range(1).take(2);  // [1, 2]
    auto o2 = observable::range(10).take(2); // [10, 11]
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2, 10, 11]
    merged.fork().subscribe(observer.ref());
}

TEST(Concat, CanStopWhileProcessingFirstStream)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(1));

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    auto o1 = observable::range(1); // infinity
    auto o2 = observable::range(10);// infinity
    auto merged = observable::concat(o1.fork(), o2.fork()); // [1, 2, 10, 11]
    merged.fork().subscribe(observer::make(
          [&](int v) { observer.on_next(v); return unsubscribe(true); }
        , [&]() { observer.on_completed(); }
        , [&](void_) { observer.on_error(void_()); }));
}

TEST(Concat, AsyncConcat_Simple)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).InSequence(s);
    EXPECT_CALL(observer, on_next(45)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> o1;
    Subject_<int> o2;
    auto merged = observable::concat(o1.as_observable(), o2.as_observable());
    merged.fork_move().subscribe(observer.ref());

    o1.on_next(42);
    o1.on_next(43);
    o1.on_completed();
    o2.on_next(44);
    o2.on_next(45);
    o2.on_completed();
}

TEST(Concat, AsyncConcat_WhenOneIsSync)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).InSequence(s);
    EXPECT_CALL(observer, on_next(45)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> o2;
    auto merged = observable::concat(observable::from(42, 43), o2.as_observable());
    merged.fork_move().subscribe(observer.ref());
    o2.on_next(44);
    o2.on_next(45);
    o2.on_completed();
}

TEST(Concat, AsyncConcat_Unsubscribe_FirstObservable)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> o1;
    Subject_<int> o2;
    auto merged = observable::concat(o1.as_observable(), o2.as_observable());
    auto unsubscriber = merged.fork_move().subscribe(observer.ref());

    o1.on_next(42);
    ASSERT_TRUE(unsubscriber());
    o1.on_next(43);
    o1.on_completed();
    o2.on_next(44);
    o2.on_next(45);
    o2.on_completed();
}

TEST(Concat, AsyncConcat_Unsubscribe_SecondObservable)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> o1;
    Subject_<int> o2;
    auto merged = observable::concat(o1.as_observable(), o2.as_observable());
    auto unsubscriber = merged.fork_move().subscribe(observer.ref());

    o1.on_next(42);
    o1.on_next(43);
    o1.on_completed();
    o2.on_next(44);
    ASSERT_TRUE(unsubscriber());
    o2.on_next(45);
    o2.on_completed();
}

TEST(Concat, AsyncConcat_Behaves_AsSingleObservable)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    Subject_<int> o1;
    Subject_<int> o2;
    auto merged = observable::concat(o1.as_observable(), o2.as_observable());
    merged.fork_move()
        .take(1)
        .subscribe(observer.ref());

    o1.on_next(42);
    o1.on_next(43);
    o1.on_completed();
    o2.on_next(44);
    o2.on_next(45);
    o2.on_completed();
}
