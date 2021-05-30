#include "observables_factory.h"
#include "operators/operator_from.h"
#include "utils_observers.h"
#include "debug_copy_move.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;

TEST(From, SingleElement_InvokedOnce)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42)
        .subscribe(observer.ref());
}

TEST(From, AllRangePassedToOnNext)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(45)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(46)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(47)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42, 43, 44, 45, 46, 47)
        .subscribe(observer.ref());
}

TEST(From, CopyMoveTrack_SingleElement_Ideal)
{
    using debug::CopyMoveTrack;

    observable::from(CopyMoveTrack(42))
        .subscribe([](CopyMoveTrack&& v)
    {
        printf("[from %i] %i\n", v._id, v._user);
    });
}

TEST(From, CopyMoveTrack_SingleElement_ForkOnce)
{
    using debug::CopyMoveTrack;

    auto o = observable::from(CopyMoveTrack(42));
    printf("[fork move] start\n");
    {
        o.fork_move()
            .subscribe([](CopyMoveTrack&& v)
        {
            printf("[from %i] %i\n", v._id, v._user);
        });
    }
    printf("[fork move] end\n");
}
