#include "operators/operator_observe_on.h"
#include "subject.h"
#include "noop_archetypes.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;
using xrx::detail::operator_tag::ObserveOn;

struct NotRealTaskScheduler
{
    std::function<void ()>* _task = nullptr;
    int* _posts_count = nullptr;
    template<typename F>
    int task_post(F task)
    {
        *_task = [f = std::move(task)]() mutable
        {
            f();
        };
        ++(*_posts_count);
        return 777;
    }

    bool task_cancel(int handle)
    {
        return (handle == 777);
    }

    auto stream_scheduler() { return *this; }
};

TEST(Compile, ObserveOn)
{
    auto o = make_operator(ObserveOn(), Noop_Observable<int, int>(), Noop_Scheduler());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(ObserveOn, SourceEmit_RequestsToScheduleTask)
{
    Subject_<int> subject;
    std::function<void ()> task;
    int posts_count = 0;
    NotRealTaskScheduler scheduler(&task, &posts_count);

    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42)).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    subject.as_observable()
        .observe_on(std::move(scheduler))
        .subscribe(observer.ref());

    ASSERT_EQ(0, posts_count);
    ASSERT_FALSE(task);

    subject.on_next(42);
    ASSERT_EQ(1, posts_count);
    ASSERT_TRUE(task);
    task();

    task = nullptr;
    subject.on_next(43);
    ASSERT_EQ(2, posts_count);
    ASSERT_TRUE(task);
    task();

    task = nullptr;
    subject.on_completed();
    ASSERT_EQ(3, posts_count);
    ASSERT_TRUE(task);
    task();
}
