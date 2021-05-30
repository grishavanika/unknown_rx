#include "operators/operator_filter.h"
#include "operators/operator_from.h"
#include "observable_interface.h"
#include "observables_factory.h"


#include <gtest/gtest.h>
#include "observer_mock.h"
#include "noop_archetypes.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Filter;

TEST(Filter, Filter_NooneWhenTrue)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);
    EXPECT_CALL(observer, on_next(3)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2, 3)
        .filter([](int) { return true; })
        .subscribe(observer.ref());
}

TEST(Filter, Filter_AllWhenFalse)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);

    EXPECT_CALL(observer, on_completed());
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2, 3)
        .filter([](int) { return false; })
        .subscribe(observer.ref());
}

TEST(Compile, Filter)
{
    auto filter = [](int) { return false; };
    auto o = make_operator(Filter(), Noop_Observable<int, void_>(), std::move(filter));
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, FilterPipe)
{
    using O = Observable_<Noop_Observable<int, void_>>;
    auto o = O() | filter([](int) { return false; });
    (void)o.fork().subscribe(Noop_Observer<int, void>());
    static_assert(ConceptObservable<decltype(o)>);
}
