#include "operators/operator_create.h"
#include "observables_factory.h"
#include "noop_archetypes.h"
#include "subject.h"
#include <gtest/gtest.h>
#include "observer_mock.h"
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Create;

TEST(Create, OnlyComplete)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);
    EXPECT_CALL(observer, on_completed());
    EXPECT_CALL(observer, on_error()).Times(0);

    auto o = observable::create<int>([](auto observer)
    {
        observer.on_completed();
    });
    static_assert(not decltype(o)::is_async());

    o.fork_move().subscribe(observer.ref());
}

TEST(Create, OnlyError)
{
    ObserverMock observer;

    EXPECT_CALL(observer, on_next(_)).Times(0);
    EXPECT_CALL(observer, on_completed()).Times(0);
    EXPECT_CALL(observer, on_error()).Times(1);

    auto o = observable::create<int>([](auto observer)
    {
        observer.on_error();
    });
    static_assert(not decltype(o)::is_async());

    o.fork_move().subscribe(observer.ref());
}

TEST(Create, SimpleSyncSequence)
{
    ObserverMock observer;
    Sequence s;
    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    auto o = observable::create<int>([](auto observer)
    {
        (void)on_next(observer, 42);
        (void)on_next(observer, 43);
        (void)on_next(observer, 44);
        on_completed(observer);
    });
    static_assert(not decltype(o)::is_async());

    o.fork_move().subscribe(observer.ref());
}

TEST(Create, SimpleAsyncSequence)
{
    ObserverMock observer;
    Sequence s;
    EXPECT_CALL(observer, on_next(42)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(43)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_next(44)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_completed()).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_error()).Times(0);

    Subject_<int> subject;

    auto o = observable::create<int>([&subject](auto observer)
    {
        return subject.as_observable().subscribe(XRX_MOV(observer));
    });
    static_assert(decltype(o)::is_async() == true); // !

    o.fork_move().subscribe(observer.ref());

    subject.on_next(42);
    subject.on_next(43);
    subject.on_next(44);
    subject.on_completed();
}

TEST(Compile, Create)
{
    auto on_subscribe = [](auto) {};
    auto o = make_operator(Create<int, int>(), XRX_MOV(on_subscribe));
    static_assert(ConceptObservable<decltype(o)>);
}
