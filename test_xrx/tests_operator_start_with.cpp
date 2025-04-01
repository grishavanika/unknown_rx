#include "observable_interface.h"
#include "observables_factory.h"
#include "operators/operator_start_with.h"
#include "operators/operator_from.h"
#include "utils_observers.h"
#include "debug_copy_move.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;

TEST(StartsWith, SingleElement)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(42)
        .starts_with(1)
        .subscribe(observer.ref());
}
