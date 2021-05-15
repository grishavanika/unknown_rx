#include "operators/operator_interval.h"
#include "observables_factory.h"
#include "noop_archetypes.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;
using xrx::detail::operator_tag::Interval;

struct NotRealScheduler
{
    using clock = std::chrono::steady_clock;
    using clock_duration = clock::duration;
    using clock_point = clock::time_point;
    using ActionHandle = int;

    std::function<void ()>* _request = nullptr;

    template<typename F, typename State>
    int tick_every(clock_point, clock_duration, F f, State state)
    {
        assert(_request);
        *_request = [f_ = XRX_MOV(f), state_ = XRX_MOV(state)]() mutable
        {
            f_(state_);
        };
        return 1;
    }

    bool tick_cancel(int handle)
    {
        return (handle == 1);
    }
};

TEST(Compile, Interval)
{
    auto o = make_operator(Interval(), std::chrono::seconds(1), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Interval, WhenSubscribed_RequestsScheduler)
{
    std::function<void ()> request;
    NotRealScheduler fake(&request);
    auto tick = observable::interval(std::chrono::seconds(1), XRX_MOV(fake));
    ASSERT_FALSE(request);

    ObserverMockAny<std::uint64_t> observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(0)).InSequence(s);
    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(0);

    auto fake_handle = tick.fork_move().subscribe(observer.ref());
    ASSERT_TRUE(request);
    request(); // 0
    request(); // 1
    request(); // 2
    ASSERT_TRUE(fake_handle.detach());
}
