#include "concepts_observer.h"

using namespace xrx;
using namespace detail;

namespace tests::on_next_
{
    struct WithOnly_OnNext
    {
        constexpr int on_next(int v) const { return v; }
    };
    static_assert(on_next(WithOnly_OnNext(), 1) == 1);

    struct WithOnly_FunctionCall
    {
        constexpr int operator()(int v) const { return v; }
    };
    static_assert(on_next(WithOnly_FunctionCall(), 2) == 2);

    struct WithOnly_TagInvoke
    {
        friend constexpr int tag_invoke(tag_t<::on_next>
            , WithOnly_TagInvoke, int v)
        { return v; }
    };
    static_assert(on_next(WithOnly_TagInvoke(), 3) == 3);

    struct WithCombination_1
    {
        constexpr int on_next(int v) const  { return v; }
        constexpr int operator()(int) const { return -1; }
    };
    static_assert(on_next(WithCombination_1(), 1) == 1);

    struct WithCombination_2
    {
        friend constexpr int tag_invoke(tag_t<::on_next>
            , WithCombination_2, int v)
        { return v; }
        constexpr int on_next(int) const { return -1; }
    };
    static_assert(on_next(WithCombination_2(), 2) == 2);

    struct WithCombination_3
    {
        friend constexpr int tag_invoke(tag_t<::on_next>
            , WithCombination_3, int v)
        { return v; }
        constexpr int operator()(int) const { return -1; }
    };
    static_assert(on_next(WithCombination_3(), 3) == 3);

    struct WithAll
    {
        friend constexpr int tag_invoke(tag_t<::on_next>
            , WithAll, int v)
        { return v; }
        constexpr int on_next(int) const { return -1; }
        constexpr int operator()(int) const { return -1; }
    };
    static_assert(on_next(WithAll(), 4) == 4);

    static_assert(is_cpo_invocable_v<tag_t<on_next>, WithAll, int>);
    static_assert(not is_cpo_invocable_v<tag_t<on_next>, void, int>);
} // namespace tests::on_next_

namespace tests::on_completed_
{
    struct With_Completed
    {
        constexpr int on_completed() const { return 1; }
    };
    static_assert(on_completed(With_Completed()) == 1);

    struct WithOnly_TagInvoke
    {
        friend constexpr int tag_invoke(tag_t<::on_completed>
            , WithOnly_TagInvoke)
        { return 2; }
    };
    static_assert(on_completed(WithOnly_TagInvoke()) == 2);

    struct WithAll
    {
        friend constexpr int tag_invoke(tag_t<::on_completed>
            , WithAll)
        { return 3; }
        constexpr int on_completed() const { return -1; }
    };
    static_assert(on_completed(WithAll()) == 3);
} // namespace tests::on_completed_

namespace tests::on_error_
{
    struct WithOnly_OnError
    {
        constexpr int on_error(int v) const { return v; }
    };
    static_assert(on_error(WithOnly_OnError(), 1) == 1);

    struct WithOnly_FunctionCall
    {
        constexpr int operator()(int v) const { return v; }
    };
    static_assert(not is_cpo_invocable_v<tag_t<on_error>, WithOnly_FunctionCall, int>);

    struct WithOnly_TagInvoke
    {
        friend constexpr int tag_invoke(tag_t<::on_error>
            , WithOnly_TagInvoke, int v)
        { return v; }
    };
    static_assert(on_error(WithOnly_TagInvoke(), 3) == 3);

    struct WithCombination
    {
        friend constexpr int tag_invoke(tag_t<::on_error>
            , WithCombination, int v)
        { return v; }
        constexpr int on_error(int) const { return -1; }
    };
    static_assert(on_error(WithCombination(), 1) == 1);
} // namespace tests::on_error_

namespace tests::concepts_
{
    struct Callback
    {
        void operator()(int) const;
        void operator()(void_) const;
    };
    static_assert(ConceptValueObserverOf<Callback, int>);
    static_assert(ConceptValueObserverOf<Callback, void_>);
    static_assert(not ConceptValueObserverOf<Callback, char*>);
    static_assert(not ConceptObserverOf<Callback, int/*value*/, int/*error*/>);

    struct Custom
    {
        void on_next(int) const;
        void operator()(void_) const;
    };
    static_assert(ConceptValueObserverOf<Custom, int>);
    static_assert(ConceptValueObserverOf<Custom, void_>);
    static_assert(not ConceptValueObserverOf<Custom, char*>);
    static_assert(not ConceptObserverOf<Custom, int/*value*/, int/*error*/>);

    struct StrictCustom
    {
        void on_next(int) const;
        void on_error(int) const;
        void on_completed() const;
    };
    static_assert(ConceptObserverOf<StrictCustom, int/*value*/, int/*error*/>);
    static_assert(ConceptValueObserverOf<StrictCustom, int>);
} // namespace tests::concepts_
