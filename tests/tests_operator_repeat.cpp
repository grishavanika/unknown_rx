#include "observables_factory.h"
#include "operators/operator_repeat.h"
#include "operators/operator_range.h"
#include "operators/operator_take.h"
#include "utils_observers.h"
#include "utils_observable.h"
#include "meta_utils.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;

struct Observable1
{
    using value_type = int;
    using error_type = none_tag;
    using is_async = std::false_type;
    using Unsubscriber = NoopUnsubscriber;

    int _value;

    explicit Observable1(int value) : _value(value) {}

    template<typename Observer>
    Unsubscriber subscribe(Observer&& observer) &&
    {
        const auto action = on_next_with_action(observer, _value);
        if (not action._stop)
        {
            on_completed_optional(XRX_FWD(observer));
        }
        return Unsubscriber();
    }

    Observable1 fork() { return *this; }
};

TEST(Repeat, Pipe)
{
    Observable_<Observable1> o(Observable1(42));
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(42)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    o.fork() | repeat(0) | subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedZeroTimes)
{
    Observable_<Observable1> o(Observable1(42));
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(42)).Times(0);

    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error()).Times(0);

    o.fork().repeat(0).subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedOnce)
{
    Observable_<Observable1> o(Observable1(42));
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    o.fork().repeat(1).subscribe(observer.ref());
}

TEST(Repeat, SingleElement_RepeatedN)
{
    Observable_<Observable1> o(Observable1(42));
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(10).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    o.fork().repeat(10).subscribe(observer.ref());
}

TEST(Repeat, SingleElement_Terminated)
{
    Observable_<Observable1> o(Observable1(42));
    ObserverMock observer;
    
    EXPECT_CALL(observer, on_next(42)).Times(1);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    o.fork().repeat(10).subscribe(observer::make([&](int v)
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

    observable::range(0, 1).repeat(0).subscribe(observer.ref());
}

TEST(Repeat, TwoElemens_RepeatedOnce)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(4)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    observable::range(3, 4).repeat(1).subscribe(observer.ref());
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

    observable::range(3, 4).repeat(2).subscribe(observer.ref());
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
