#include "observables_factory.h"
#include "operators/operator_take.h"
#include "operators/operator_range.h"
#include "operators/operator_filter.h"
#include "operators/operator_from.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace ::xrx;
using namespace ::xrx::detail;

TEST(Pipe, Compose)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    auto pipe_ = pipe(take(1), filter([](int) { return true; }));

    observable::range(0)
        | std::move(pipe_)
        | subscribe(observer.ref());
}

TEST(Pipe, WithObservable)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    pipe(observable::range(0)
        , take(1), filter([](int) { return true; })
        , subscribe(observer.ref()));
}

TEST(Pipe, WithObservable_SeparateSubscribe)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    pipe(observable::range(0)
        , take(1), filter([](int) { return true; }))
        | subscribe(observer.ref());
}

TEST(Pipe, FreeFunction)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    auto custom_processor = [](auto&& observable)
    {
        (void)observable; // Can be ignored...
        return observable::from(42);
    };

    pipe(observable::range(1)
        , take(1), filter([](int) { return false; }) // Ignored...
        , custom_processor // because of this.
        , subscribe(observer.ref()));
}

TEST(Pipe, Compose_WithPipe_AsMemberFunction)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);

    observable::range(0).pipe(
        take(1)
        , filter([](int) { return true; }))
        .subscribe(observer.ref());
}
