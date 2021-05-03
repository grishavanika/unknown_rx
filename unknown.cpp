#include "tag_invoke.hpp"

namespace detail
{
    template<typename CPO, typename... Args>
    concept CPOInvocable = requires(CPO& cpo, Args&&... args)
    {
        cpo(std::forward<Args>(args)...);
    };
} // namespace detail

template<typename CPO, typename... Args>
constexpr bool is_cpo_invocable_v = detail::CPOInvocable<CPO, Args...>;

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

    static_assert(is_cpo_invocable_v<tag_t<on_next>, WithAll, int>);
    static_assert(not is_cpo_invocable_v<tag_t<on_next>, void, int>);
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
    static_assert(not is_cpo_invocable_v<tag_t<on_error>, Void_WithOnly_FunctionCall>);

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

namespace detail
{
    template<typename Observer, typename Value>
    struct OnNext_Invocable
    {
        using Evaluate = std::bool_constant<
            is_cpo_invocable_v<tag_t<on_next>, Observer, Value>>;
    };
    template<typename Observer>
    struct OnNext_Invocable<Observer, void>
    {
        using Evaluate = std::bool_constant<
            is_cpo_invocable_v<tag_t<on_next>, Observer>>;
    };

    template<typename Observer, typename Value>
    struct OnError_Invocable
    {
        using Evaluate = std::bool_constant<
            is_cpo_invocable_v<tag_t<on_error>, Observer, Value>>;
    };
    template<typename Observer>
    struct OnError_Invocable<Observer, void>
    {
        using Evaluate = std::bool_constant<
            is_cpo_invocable_v<tag_t<on_error>, Observer>>;
    };
} // namespace detail

// Simplest possible observer with only on_next() callback.
// No error handling or on_complete() detection.
template<typename Observer, typename Value>
concept ValueObserverOf
    = detail::OnNext_Invocable<Observer, Value>::Evaluate::value;

// Full/strict definition of observer:
// - on_next(Value)
// - on_error(Error)
// - on_completed()
template<typename Observer, typename Value, typename Error>
concept StrictObserverOf
    =    detail::OnNext_Invocable<Observer, Value>::Evaluate::value
      && is_cpo_invocable_v<tag_t<on_completed>, Observer>
      && detail::OnError_Invocable<Observer, Error>::Evaluate::value;

#if (ZZZ_TEST())
namespace debug_tests::concepts_
{
    struct Callback
    {
        void operator()(int) const;
        void operator()() const;
    };
    static_assert(ValueObserverOf<Callback, int>);
    static_assert(ValueObserverOf<Callback, void>);
    static_assert(not ValueObserverOf<Callback, char*>);
    static_assert(not StrictObserverOf<Callback, int/*value*/, int/*error*/>);

    struct Custom
    {
        void on_next(int) const;
        void operator()() const;
    };
    static_assert(ValueObserverOf<Custom, int>);
    static_assert(ValueObserverOf<Custom, void>);
    static_assert(not ValueObserverOf<Custom, char*>);
    static_assert(not StrictObserverOf<Custom, int/*value*/, int/*error*/>);

    struct StrictCustom
    {
        void on_next(int) const;
        void on_error() const;
        void on_error(int) const;
        void on_completed() const;
    };
    static_assert(StrictObserverOf<StrictCustom, int/*value*/, void/*error*/>);
    static_assert(StrictObserverOf<StrictCustom, int/*value*/, int/*error*/>);
    static_assert(ValueObserverOf<StrictCustom, int>);
} // namespace debug_tests
#endif

namespace detail
{
    struct OnNext_Noop
    {
        template<typename Value>
        constexpr void on_next(Value&&) const noexcept
        {
        }
        void on_next() const noexcept
        {
        }
    };

    struct OnCompleted_Noop
    {
        constexpr void on_completed() const noexcept
        {
        }
    };

    struct OnError_Noop
    {
        template<typename Error>
        constexpr void on_error(Error&&) const noexcept
        {
        }
        constexpr void on_error() const noexcept
        {
        }
    };

    template<typename OnNext, typename OnCompleted, typename OnError>
    struct LambdaObserver
    {
        [[no_unique_address]] OnNext _on_next;
        [[no_unique_address]] OnCompleted _on_completed;
        [[no_unique_address]] OnError _on_error;

        template<typename Value>
        constexpr auto on_next(Value&& value) const
            -> decltype(::on_next(_on_next, std::forward<Value>(value)))
        {
            return ::on_next(_on_next, std::forward<Value>(value));
        }

        constexpr decltype(auto) on_next() const
            requires requires { ::on_next(_on_next); }
        {
            return ::on_next(_on_next);
        }

        constexpr auto on_completed()
            requires requires { std::move(_on_completed)(); }
        {
            return std::move(_on_completed)();
        }

        template<typename Error>
        constexpr auto on_error(Error&& error)
            -> decltype(std::move(_on_error)(std::forward<Error>(error)))
        {
            return std::move(_on_error)(std::forward<Error>(error));
        }

        constexpr decltype(auto) on_error()
            requires requires { std::move(_on_error)(); }
        {
            return std::move(_on_error)();
        }
    };
} // namespace detail

namespace observer
{
    template<typename OnNext>
    auto make(OnNext&& on_next)
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = detail::LambdaObserver<F_, detail::OnCompleted_Noop, detail::OnError_Noop>;
        return Observer(std::forward<OnNext>(on_next), {}, {});
    }

    template<typename Value, typename OnNext>
    ValueObserverOf<Value> auto
        make_of(OnNext&& on_next)
            requires is_cpo_invocable_v<tag_t<::on_next>, OnNext, Value>
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = detail::LambdaObserver<F_, detail::OnCompleted_Noop, detail::OnError_Noop>;
        return Observer(std::forward<OnNext>(on_next), {}, {});
    }

    template<typename OnNext, typename OnCompleted, typename OnError = detail::OnError_Noop>
    auto make(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error = {})
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(std::forward<OnNext>(on_next)
            , std::forward<OnCompleted>(on_completed)
            , std::forward<OnError>(on_error));
    }

    template<typename Value, typename Error
        , typename OnNext, typename OnCompleted, typename OnError>
    StrictObserverOf<Value, Error> auto
        make_of(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error)
            requires is_cpo_invocable_v<tag_t<::on_next>, OnNext, Value> &&
                     is_cpo_invocable_v<tag_t<::on_error>, OnError, Error>
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(std::forward<OnNext>(on_next)
            , std::forward<OnCompleted>(on_completed)
            , std::forward<OnError>(on_error));
    }
} // namespace observer

template<typename KnownObserver>
    requires ValueObserverOf<KnownObserver, int>
struct StaticObservable
{
    [[no_unique_address]] KnownObserver _known_observer;

    template<typename Observer>
        requires ValueObserverOf<Observer, int>
    void subscribe(Observer&& observer)
    {
        auto do_next = observer::make([&](int new_value)
        {
            on_next(_known_observer, new_value);
            on_next(observer, new_value);
        });

        do_next.on_next(1);
        do_next.on_next(2);
        do_next.on_next(3);
        do_next.on_next(4);

        if constexpr (is_cpo_invocable_v<tag_t<on_completed>, KnownObserver>)
        {
            on_completed(_known_observer);
        }
        if constexpr (is_cpo_invocable_v<tag_t<on_completed>, Observer>)
        {
            on_completed(std::forward<Observer>(observer));
        }
    }
};

#include <cstdio>

int main()
{
    auto o1 = observer::make([](int value)
    {
        std::printf("Known Observer: %i.\n", value);
    }
        , []()
    {
        std::printf("Known Observer: Completed.\n");
    });

    StaticObservable<decltype(o1)> observable(std::move(o1));
    observable.subscribe([](int value)
    {
        std::printf("Observer: %i.\n", value);
    });
}
