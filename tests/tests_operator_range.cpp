#include "observables_factory.h"
#include "operators/operator_range.h"
#include "utils_observers.h"
#include "noop_archetypes.h"
#include "cpo_make_operator.h"

#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;

TEST(Compile, Range_Constructor)
{
    using xrx::detail::operator_tag::Range;
    auto o = make_operator(Range(), 0, 0, 0, std::true_type());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, Range_Subscribe_OnlyOnNext)
{
    using xrx::detail::operator_tag::Range;
    auto o = make_operator(Range(), 0, 0, 1, std::false_type());
    o.fork().subscribe([](int) {});
}

TEST(Compile, Range_Subscribe_OnNextOnCompleted)
{
    using xrx::detail::operator_tag::Range;
    auto o = make_operator(Range(), 0, 0, 1, std::false_type());
    o.fork().subscribe(observer::make([](int) {}, [] {}));
}

TEST(Range, WhenBoundariesAreEqual_InvokedOnce)
{
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error(_)).Times(0);

        observable::range(0, 0)
            .subscribe(observer.ref());
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error(_)).Times(0);

        observable::range(0, 0, 1)
            .subscribe(observer.ref());
    }
    {
        ObserverMock observer;
        Sequence s;

        EXPECT_CALL(observer, on_next(0)).Times(1).InSequence(s);

        EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
        EXPECT_CALL(observer, on_error(_)).Times(0);

        observable::range(0, 0, -1)
            .subscribe(observer.ref());
    }
}

TEST(Range, BoundariesAreInclusive_DirectionRight)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(_)).Times(11).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::range(0, 10)
        .subscribe(observer.ref());
}

TEST(Range, BoundariesAreInclusive_DirectionLeft)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(_)).Times(11).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::range(10, 0, -1)
        .subscribe(observer.ref());
}
