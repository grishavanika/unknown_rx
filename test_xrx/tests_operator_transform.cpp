#include "operators/operator_transform.h"
#include "operators/operator_from.h"
#include "observables_factory.h"
#include "subject.h"
#include "noop_archetypes.h"
#include "observer_mock.h"
#include "debug_copy_move.h"
#include <gtest/gtest.h>
using namespace testing;
using namespace xrx;
using namespace xrx::detail;
using namespace tests;
using xrx::detail::operator_tag::Transform;

TEST(Compile, Transform)
{
    auto transform = [](int v) { return v; };
    auto o = make_operator(Transform(), Noop_Observable<int, void_>(), std::move(transform));
    static_assert(ConceptObservable<decltype(o)>);
}


TEST(Compile, TransformPipe)
{
    using O = Observable_<Noop_Observable<int, void_>>;
    auto o = O() | transform([](int) { return false; });
    static_assert(ConceptObservable<decltype(o)>);
    (void)o.fork().subscribe(Noop_Observer<bool, void>());
}

TEST(Transform, IdentityTransform)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2)
        .transform([_ = debug::CopyMoveTrack(42)](int v) { return v; })
        .subscribe(observer.ref());
}

TEST(Transform, IdentityTransform_CopyMoveTrack)
{
    ObserverMock observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(1)).InSequence(s);
    EXPECT_CALL(observer, on_next(2)).InSequence(s);

    EXPECT_CALL(observer, on_completed()).InSequence(s);
    EXPECT_CALL(observer, on_error(_)).Times(0);

    observable::from(1, 2)
        .transform([_ = debug::CopyMoveTrack(42)](int v)
    {
        printf("[transform] %i\n", v);
        return v;
    })
        .subscribe(observer.ref());
}

TEST(Transform, StopOnError)
{
    Subject_<int, int> subject;
    auto o = subject.as_observable();

    ObserverMock_Error observer;
    Sequence s;

    EXPECT_CALL(observer, on_next(42 + 1)).InSequence(s);

    EXPECT_CALL(observer, on_error(-1)).Times(1).InSequence(s);
    EXPECT_CALL(observer, on_completed()).Times(0);

    o.fork_move()
        .transform([](int v) { return v + 1; })
        .subscribe(observer.ref());

    subject.on_next(42);
    subject.on_error(-1);
}
