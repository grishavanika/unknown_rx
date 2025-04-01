#include "observables_factory.h"
#include "operators/operator_repeat.h"
#include "operators/operator_interval.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"
#include "operators/operator_from.h"
#include "simple_scheduler_event_loop.h"
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
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42)
        | repeat(0)
        | subscribe(observer.ref());
}

TEST(Repeat, Endless)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(42);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42)
        .repeat(10)
        .subscribe(observer.ref());
}

TEST(Repeat, SingleElement_Terminated)
{
    ObserverMock observer;
    
    EXPECT_CALL(observer, on_next(42)).Times(1);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42)
        .repeat(10)
        .subscribe(observer::make([&](int v)
    {
        observer.on_next(v);
        return unsubscribe(true);
    }
        , [&]()      { observer.on_completed(); }
        , [&](void_) { observer.on_error(void_()); }));
}

TEST(Repeat, TwoElemens_RepeatedZeroTime)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

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
    EXPECT_CALL(observer, on_error(_)).Times(0);

    // #XXX: accept Observers that are std::reference_wrapper<>.
    observable::range(3, 4)
        .repeat(2)
        .take(1)
        .subscribe(observer.ref());
}

TEST(Repeat, Async_Simple_ZeroRepeat)
{
    ObserverMockAny<std::uint64_t> observer;
    EXPECT_CALL(observer, on_next(_)).Times(0);
    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    debug::EventLoop event_loop;
    auto async = observable::interval(std::chrono::milliseconds(1), event_loop.scheduler());
    async.fork_move()
        .take(2)
        .repeat(0) // No repeats.
        .subscribe(observer.ref());

    while (event_loop.work_count() > 0)
    {
        event_loop.poll_one();
    }
}

TEST(Repeat, Async_Simple_RepeatOnce)
{
    ObserverMockAny<std::uint64_t> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    debug::EventLoop event_loop;
    auto async = observable::interval(std::chrono::milliseconds(1), event_loop.scheduler());
    async.fork_move()
        .take(2)
        .repeat(1) // Once.
        .subscribe(observer.ref());

    while (event_loop.work_count() > 0)
    {
        event_loop.poll_one();
    }
}

TEST(Repeat, Async_Simple_RepeatTwice)
{
    ObserverMockAny<std::uint64_t> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    debug::EventLoop event_loop;
    auto async = observable::interval(std::chrono::milliseconds(1), event_loop.scheduler());
    async.fork_move()
        .take(2)
        .repeat(2) // Two times !
        .subscribe(observer.ref());

    while (event_loop.work_count() > 0)
    {
        event_loop.poll_one();
    }
}

TEST(Repeat, Async_Infinity)
{
    ObserverMockAny<std::uint64_t> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(0)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    debug::EventLoop event_loop;
    auto async = observable::interval(std::chrono::milliseconds(1), event_loop.scheduler());
    async.fork_move()
        .take(2)
        .repeat() // Endless.
        .take(3)
        .subscribe(observer.ref());

    while (event_loop.work_count() > 0)
    {
        event_loop.poll_one();
    }
}
