#include "operators/operator_tap_do.h"
#include "operators/operator_from.h"
#include "observable_interface.h"
#include "observables_factory.h"
#include "utils_observers.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;

TEST(Tap, SameEvents_BeforeObserver)
{
    ObserverMock listener;
    ObserverMock observer;
    Sequence s;
    EXPECT_CALL(listener, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);

    EXPECT_CALL(listener, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(listener, on_next(3)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);

    EXPECT_CALL(listener, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_completed()).InSequence(s);
    
    EXPECT_CALL(listener, on_error(_)).Times(0);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    int calls_count = 0;
    auto wrap = observable::from(1, 2, 3)
        .do_(observer::make(
            [&](int v)
        {
            ++calls_count;
            listener.on_next(int(v));
        }
            , [&]()
        {
            ++calls_count;
            listener.on_completed();
        }
            , [&](void_)
        {
            ++calls_count;
            listener.on_error(void_());
        }));
    ASSERT_EQ(0, calls_count);

    wrap.fork_move()
        .subscribe(observer.ref());
    ASSERT_EQ(3 + 1, calls_count);
}
