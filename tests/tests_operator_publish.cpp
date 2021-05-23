#include "operators/operator_publish.h"
#include "operators/operator_create.h"
#include "observables_factory.h"
#include "noop_archetypes.h"
#include "observable_interface.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace tests;
using namespace xrx;
using namespace xrx::detail;
using xrx::detail::operator_tag::Publish;

TEST(Publish, SourceSubscribe_CalledOnceWhenPublished)
{
    int source_subscribe_count = 0;
    auto source = observable::create<int>([&](auto observer)
    {
        ++source_subscribe_count;
        (void)on_next(observer, 42);
        (void)on_next(observer, 43);
        (void)on_completed_optional(observer);
    });

    auto connected = source.fork_move().publish();
    
    ObserverMock o1;
    Sequence s1;
    ObserverMock o2;
    Sequence s2;
    ObserverMock o3_unsubscribe;

    {
        EXPECT_CALL(o1, on_next(42)).Times(1).InSequence(s1);
        EXPECT_CALL(o1, on_next(43)).Times(1).InSequence(s1);
        EXPECT_CALL(o1, on_completed()).Times(1).InSequence(s1);
        EXPECT_CALL(o1, on_error()).Times(0);
    }
    {
        EXPECT_CALL(o2, on_next(42)).Times(1).InSequence(s2);
        EXPECT_CALL(o2, on_next(43)).Times(1).InSequence(s2);
        EXPECT_CALL(o2, on_completed()).Times(1).InSequence(s2);
        EXPECT_CALL(o2, on_error()).Times(0);
    }
    {
        EXPECT_CALL(o3_unsubscribe, on_next(_)).Times(0);
        EXPECT_CALL(o3_unsubscribe, on_completed()).Times(0);
        EXPECT_CALL(o3_unsubscribe, on_error()).Times(0);
    }

    connected.fork().subscribe(o1.ref());
    // No source subscribe() yet.
    ASSERT_EQ(0, source_subscribe_count);
    // No source subscribe() again.
    connected.fork().subscribe(o2.ref());
    ASSERT_EQ(0, source_subscribe_count);
    
    {
        auto unsubscribe = connected.fork().subscribe(o3_unsubscribe.ref());
        ASSERT_EQ(0, source_subscribe_count);
        unsubscribe.detach();
    }
    // Fetch source, populate to children.
    connected.connect();
    ASSERT_EQ(1, source_subscribe_count);
}

TEST(Compile, Publish)
{
    auto o = make_operator(Publish(), Noop_Observable<int, int>());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(Compile, PublishPipe)
{
    using O = Observable_<Noop_Observable<int, int>>;
    auto o = O() | publish();
    (void)o.fork().subscribe(Noop_Observer<int, int>());
    static_assert(ConceptObservable<decltype(o)>);
}

TEST(PublishX, Connect_Is_Noop_IfAlreadyConnected)
{
    int source_subscribe_count = 0;
    auto source = observable::create<int>([&](auto observer)
    {
        ++source_subscribe_count;
        (void)on_completed_optional(observer);
    });

    auto connected = source.fork_move().publish();
    {
        auto unconnect1 = connected.connect();
        ASSERT_TRUE(unconnect1);
        // 1st time subscribe.
        ASSERT_EQ(1, source_subscribe_count);
        auto unconnect2 = connected.connect();
        ASSERT_FALSE(unconnect2);
        (void)unconnect1.detach();
    }

    {
        auto unconnect1 = connected.connect();
        ASSERT_TRUE(unconnect1);
        // 2nd time subscribe.
        ASSERT_EQ(2, source_subscribe_count);
        auto unconnect2 = connected.connect();
        ASSERT_FALSE(unconnect2);
        (void)unconnect1.detach();
    }
}

TEST(PublishX, RefCount_ConnectsOnFirstSubscribe)
{
    int source_subscribe_count = 0;
    auto source = observable::create<int>([&](auto observer)
    {
        ++source_subscribe_count;
        (void)on_next(observer, 42);
        (void)on_next(observer, 43);
        (void)on_completed_optional(observer);
    });

    ObserverMock o1;
    Sequence s1;
    EXPECT_CALL(o1, on_next(42)).Times(1).InSequence(s1);
    EXPECT_CALL(o1, on_next(43)).Times(1).InSequence(s1);
    EXPECT_CALL(o1, on_completed()).Times(1).InSequence(s1);

    EXPECT_CALL(o1, on_next(42)).Times(1).InSequence(s1);
    EXPECT_CALL(o1, on_next(43)).Times(1).InSequence(s1);
    EXPECT_CALL(o1, on_completed()).Times(1).InSequence(s1);
    EXPECT_CALL(o1, on_error()).Times(0);

    EXPECT_CALL(o1, on_error()).Times(0);

    auto shared = source.fork_move()
        .publish()
        .ref_count();

    { // subscribe-subscribe-detach-detach -> connect/disconnect.
        auto unsubscriber1 = shared.fork().subscribe(o1.ref());
        auto unsubscriber2 = shared.fork().subscribe(o1.ref());
        ASSERT_EQ(1, source_subscribe_count);
        unsubscriber1.detach();
        unsubscriber2.detach();
    }

    { // subscribe-detach -> connect/disconnect 2nd time.
        auto unsubscriber1 = shared.fork().subscribe(o1.ref());
        ASSERT_EQ(2, source_subscribe_count);
        unsubscriber1.detach();
    }
}
