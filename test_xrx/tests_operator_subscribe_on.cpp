#include "operators/operator_subscribe_on.h"
#include "operators/operator_create.h"
#include "observables_factory.h"
#include "noop_archetypes.h"
#include "simple_scheduler_event_loop.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;

TEST(Compile, SubscribeOn)
{
    using xrx::detail::operator_tag::SubscribeOn;
    auto o = make_operator(SubscribeOn(), Noop_Observable<int, int>(), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(SubscribeOn, SubscribeExecutedOnScheduler)
{
    debug::EventLoop event_loop;
    auto source = observable::create<int>([](auto observer)
    {
        on_completed_optional(observer);
    });

    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);
    EXPECT_CALL(observer, on_completed()).Times(1);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    auto on_scheduler = source.fork_move()
        .subscribe_on(event_loop.scheduler());
    ASSERT_EQ(0, event_loop.work_count());

    on_scheduler.fork_move()
        .subscribe(observer.ref());
    ASSERT_EQ(1, event_loop.work_count());
    ASSERT_EQ(std::size_t(1), event_loop.poll_one());
}
