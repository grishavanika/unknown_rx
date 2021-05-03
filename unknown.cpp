#include "tag_invoke.hpp"

#include <concepts>
#include <functional>

#define ZZZ_TEST() 1

template<unsigned I>
struct priority_tag
    : priority_tag<I - 1> {};
template<>
struct priority_tag<0> {};

inline const struct on_next_fn
{
    // on_next() with single argument T
    template<typename S, typename T>
    constexpr decltype(auto) resolve1_(S&& s, T&& v, priority_tag<2>) const
        noexcept(noexcept(tag_invoke(*this, std::forward<S>(s), std::forward<T>(v))))
           requires tag_invocable<on_next_fn, S, T>
    {
        return tag_invoke(*this, std::forward<S>(s), std::forward<T>(v));
    }
    template<typename S, typename T>
    constexpr auto resolve1_(S&& s, T&& v, priority_tag<1>) const
        noexcept(noexcept(std::forward<S>(s).on_next(std::forward<T>(v))))
            -> decltype(std::forward<S>(s).on_next(std::forward<T>(v)))
    {
        return std::forward<S>(s).on_next(std::forward<T>(v));
    }
    template<typename S, typename T>
    constexpr decltype(auto) resolve1_(S&& s, T&& v, priority_tag<0>) const
        noexcept(noexcept(std::invoke(std::forward<S>(s), std::forward<T>(v))))
            requires std::invocable<S, T>
    {
        return std::invoke(std::forward<S>(s), std::forward<T>(v));
    }
    template<typename S, typename T>
    constexpr auto operator()(S&& s, T&& v) const
        noexcept(noexcept(resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>())))
            -> decltype(resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>()))
    {
        return resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>());
    }
    // on_next() with no argument.
    template<typename S>
    constexpr decltype(auto) resolve0_(S&& s, priority_tag<2>) const
        noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
            requires tag_invocable<on_next_fn, S>
    {
        return tag_invoke(*this, std::forward<S>(s));
    }
    template<typename S>
    constexpr auto resolve0_(S&& s, priority_tag<1>) const
        noexcept(noexcept(std::forward<S>(s).on_next()))
            -> decltype(std::forward<S>(s).on_next())
    {
        return std::forward<S>(s).on_next();
    }
    template<typename S>
    constexpr decltype(auto) resolve0_(S&& s, priority_tag<0>) const
        noexcept(noexcept(std::invoke(std::forward<S>(s))))
            requires std::invocable<S>
    {
        return std::invoke(std::forward<S>(s));
    }
    template<typename S>
    constexpr auto operator()(S&& s) const
        noexcept(noexcept(resolve0_(std::forward<S>(s), priority_tag<2>())))
            -> decltype(resolve0_(std::forward<S>(s), priority_tag<2>()))
    {
        return resolve0_(std::forward<S>(s), priority_tag<2>());
    }
}
on_next{};

#if (ZZZ_TEST())
namespace debug_tests::on_next_
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

    static_assert(is_tag_invocable_v<tag_t<on_next>, WithAll, int>);
    static_assert(not is_tag_invocable_v<tag_t<on_next>, void, int>);
} // namespace debug_tests
#endif

inline const struct on_completed_fn
{
    template<typename S>
    constexpr decltype(auto) resolve0_(S&& s, priority_tag<1>) const
        noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
            requires tag_invocable<on_completed_fn, S>
    {
        return tag_invoke(*this, std::forward<S>(s));
    }
    template<typename S>
    constexpr auto resolve0_(S&& s, priority_tag<0>) const
        noexcept(noexcept(std::forward<S>(s).on_completed()))
            -> decltype(std::forward<S>(s).on_completed())
    {
        return std::forward<S>(s).on_completed();
    }
    template<typename S>
    constexpr auto operator()(S&& s) const
        noexcept(noexcept(resolve0_(std::forward<S>(s), priority_tag<1>())))
            -> decltype(resolve0_(std::forward<S>(s), priority_tag<1>()))
    {
        return resolve0_(std::forward<S>(s), priority_tag<1>());
    }
}
on_completed{};

#if (ZZZ_TEST())
namespace debug_tests::on_completed_
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
} // namespace debug_tests
#endif

inline const struct on_error
{
    // on_error() with single argument.
    template<typename S, typename E>
    constexpr decltype(auto) resolve1_(S&& s, E&& e, priority_tag<1>) const
        noexcept(noexcept(tag_invoke(*this, std::forward<S>(s), std::forward<E>(e))))
            requires tag_invocable<on_error, S, E>
    {
        return tag_invoke(*this, std::forward<S>(s), std::forward<E>(e));
    }
    template<typename S, typename E>
    constexpr auto resolve1_(S&& s, E&& e, priority_tag<0>) const
        noexcept(noexcept(std::forward<S>(s).on_error(std::forward<E>(e))))
            -> decltype(std::forward<S>(s).on_error(std::forward<E>(e)))
    {
        return std::forward<S>(s).on_error(std::forward<E>(e));
    }
    template<typename S, typename E>
    constexpr auto operator()(S&& s, E&& e) const
        noexcept(noexcept(resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>())))
            -> decltype(resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>()))
    {
        return resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>());
    }
    // on_error() with no argument.
    template<typename S>
    constexpr decltype(auto) resolve0_(S&& s, priority_tag<1>) const
        noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
            requires tag_invocable<on_error, S>
    {
        return tag_invoke(*this, std::forward<S>(s));
    }
    template<typename S>
    constexpr auto resolve0_(S&& s, priority_tag<0>) const
        noexcept(noexcept(std::forward<S>(s).on_error()))
            -> decltype(std::forward<S>(s).on_error())
    {
        return std::forward<S>(s).on_error();
    }
    template<typename S>
    constexpr auto operator()(S&& s) const
        noexcept(noexcept(resolve0_(std::forward<S>(s), priority_tag<1>())))
            -> decltype(resolve0_(std::forward<S>(s), priority_tag<1>()))
    {
        return resolve0_(std::forward<S>(s), priority_tag<1>());
    }
}
on_error{};

#if (ZZZ_TEST())
namespace debug_tests::on_error_
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
    static_assert(not is_tag_invocable_v<tag_t<on_error>, WithOnly_FunctionCall, int>);

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
        constexpr int on_error(int v) const { return -1; }
    };
    static_assert(on_error(WithCombination(), 1) == 1);

    // void case.
    struct Void_WithOnly_OnError
    {
        constexpr int on_error() const { return 2; }
    };
    static_assert(on_error(Void_WithOnly_OnError()) == 2);

    struct Void_WithOnly_FunctionCall
    {
        constexpr int operator()() const { return -1; }
    };
    static_assert(not is_tag_invocable_v<tag_t<on_error>, Void_WithOnly_FunctionCall>);

    struct Void_WithOnly_TagInvoke
    {
        friend constexpr int tag_invoke(tag_t<::on_error>
            , Void_WithOnly_TagInvoke)
        { return 2; }
    };
    static_assert(on_error(Void_WithOnly_TagInvoke()) == 2);

    struct Void_WithCombination
    {
        friend constexpr int tag_invoke(tag_t<::on_error>
            , Void_WithCombination)
        { return 2; }
        constexpr int on_error(int v) const { return -1; }
    };
    static_assert(on_error(Void_WithCombination()) == 2);
} // namespace debug_tests
#endif

int main()
{

}
