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
namespace debug_tests
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
} // debug_tests
#endif

int main()
{

}
