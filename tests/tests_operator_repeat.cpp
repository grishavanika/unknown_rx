#include "observables_factory.h"
#include "operators/operator_repeat.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "meta_utils.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;

TEST(Repeat, Pipe)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(42)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        | repeat(0)
        | subscribe(observer.ref());
}

TEST(Repeat, Endless)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(42);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(3)
        | repeat()
        | take(42)
        | subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedZeroTimes)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(42)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        .repeat(0)
        .subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedOnce)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        .repeat(1)
        .subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedN)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(10).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        .repeat(10)
        .subscribe(observer.ref());
}

TEST(Repeat, SingleElement_Terminated)
{
    ObserverMock observer;
    
    EXPECT_CALL(observer, on_next(42)).Times(1);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::from(42)
        .repeat(10)
        .subscribe(observer::make([&](int v)
    {
        observer.on_next(v);
        return unsubscribe(true);
    }
        , [&]() { observer.on_completed(); }
        , [&]() { observer.on_error(); }));
}

TEST(Repeat, TwoElemens_RepeatedZeroTime)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(0, 1)
        .repeat(0)
        .subscribe(observer.ref());
}

TEST(Repeat, TwoElemens_RepeatedOnce)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(3, 4)
        .repeat(1)
        .subscribe(observer.ref());
}

TEST(Repeat, TwoElemens_RepeatedTwice)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(3, 4)
        .repeat(2)
        .subscribe(observer.ref());
}

TEST(Repeat, Terminated_OnFirstPass)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(3)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    // #XXX: accept Observers that are std::reference_wrapper<>.
    observable::range(3, 4)
        .repeat(2)
        .take(1)
        .subscribe(observer.ref());
}
