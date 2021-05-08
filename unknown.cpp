#include "tag_invoke.hpp"
#include <memory>
#include <algorithm>

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

#include <cassert>

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
        : std::bool_constant<
            is_cpo_invocable_v<tag_t<on_next>, Observer, Value>>
    {
    };
    template<typename Observer>
    struct OnNext_Invocable<Observer, void>
        : std::bool_constant<
            is_cpo_invocable_v<tag_t<on_next>, Observer>>
    {
    };

    template<typename Observer>
    struct OnCompleted_Invocable
        : std::bool_constant<
            is_cpo_invocable_v<tag_t<on_completed>, Observer>>
    {
    };

    template<typename Observer, typename Value>
    struct OnError_Invocable
        : std::bool_constant<
            is_cpo_invocable_v<tag_t<on_error>, Observer, Value>>
    {
    };
    template<typename Observer>
    struct OnError_Invocable<Observer, void>
        : std::bool_constant<
            is_cpo_invocable_v<tag_t<on_error>, Observer>>
    {
    };

    template<typename Observer, typename Value>
    concept WithOnNext = OnNext_Invocable<Observer, Value>::value;
    template<typename Observer>
    concept WithOnCompleted = OnCompleted_Invocable<Observer>::value;
    template<typename Observer, typename Error>
    concept WithOnError = OnError_Invocable<Observer, Error>::value;

} // namespace detail

// Simplest possible observer with only on_next() callback.
// No error handling or on_complete() detection.
template<typename Observer, typename Value>
concept ValueObserverOf
    = detail::OnNext_Invocable<Observer, Value>::value;

// Full/strict definition of observer:
// - on_next(Value)
// - on_error(Error)
// - on_completed()
template<typename Observer, typename Value, typename Error>
concept StrictObserverOf
    =    detail::WithOnNext<Observer, Value>
      && detail::WithOnCompleted<Observer>
      && detail::WithOnError<Observer, Error>;

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
    struct none_tag {};

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
        constexpr decltype(auto) on_next(Value&& value)
            requires detail::WithOnNext<OnNext, Value>
        {
            return ::on_next(_on_next, std::forward<Value>(value));
        }

        constexpr decltype(auto) on_next()
            requires detail::WithOnNext<OnNext, void>
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
            requires requires { std::move(_on_error)(std::forward<Error>(error)); }
        {
            return std::move(_on_error)(std::forward<Error>(error));
        }

        constexpr decltype(auto) on_error()
            requires requires { std::move(_on_error)(); }
        {
            return std::move(_on_error)();
        }
    };

    template<typename Observer>
    struct StrictObserver
    {
        [[no_unique_address]] Observer _observer;

        template<typename Value>
        constexpr decltype(auto) on_next(Value&& value)
        {
            if constexpr (detail::WithOnNext<Observer, Value>)
            {
                return ::on_next(_observer, std::forward<Value>(value));
            }
        }

        constexpr decltype(auto) on_next()
        {
            if constexpr (detail::WithOnNext<Observer, void>)
            {
                return ::on_next(_observer);
            }
        }

        constexpr auto on_completed()
        {
            if constexpr (detail::WithOnCompleted<Observer>)
            {
                return ::on_completed(std::move(_observer));
            }
        }

        template<typename Error>
        constexpr auto on_error(Error&& error)
        {
            if constexpr (detail::WithOnError<Observer, Error>)
            {
                return ::on_error(std::move(_observer), std::forward<Error>(error));
            }
        }

        constexpr decltype(auto) on_error()
        {
            if constexpr (detail::WithOnError<Observer, void>)
            {
                return ::on_error(std::move(_observer));
            }
        }
    };

    template<typename Observer>
    auto make_strict(Observer&& o)
    {
        using Observer_ = std::remove_cvref_t<Observer>;
        return StrictObserver<Observer_>{std::forward<Observer>(o)};
    }

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

namespace detail::operator_tag
{
    template<typename>
    struct AlwaysFalse : std::false_type {};

    struct Filter
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .filter() operator implementation. "
                  "Missing include ?");
        };
    };
} // namespace detail

inline const struct make_operator_fn
{
    template<typename Tag, typename... Args>
    constexpr decltype(auto) operator()(Tag&& tag, Args&&... args) const
        noexcept(noexcept(tag_invoke(*this, std::forward<Tag>(tag), std::forward<Args>(args)...)))
            requires tag_invocable<make_operator_fn, Tag, Args...>
    {
        return tag_invoke(*this, std::forward<Tag>(tag), std::forward<Args>(args)...);
    }

    template<typename Tag, typename... Args>
    constexpr decltype(auto) operator()(Tag, Args&&...) const noexcept
        requires (not tag_invocable<make_operator_fn, Tag, Args...>)
    {
        using NotFound = typename Tag::template NotFound<int/*any placeholder type*/>;
        return NotFound();
    }
} make_operator;

struct InitialSourceObservable_
{
    using value_type = int;
    using error_type = detail::none_tag;

    struct Unsubscriber
    {
        using has_effect = std::false_type;
        bool detach() { return false; }
    };

    template<typename Observer>
    Unsubscriber subscribe(Observer&& observer) &&
    {
        auto strict = detail::make_strict(std::forward<Observer>(observer));
        strict.on_next(1);
        strict.on_next(2);
        strict.on_next(3);
        strict.on_next(4);
        strict.on_completed();
        return Unsubscriber();
    }
};

#if (0)
template<typename T>
static auto copy(T&& v) -> std::remove_cv_t<T>
{
    using Self = std::remove_cv_t<T>;
    return Self(std::forward<T>(v));
}

template<typename T>
static auto own(T&& v)
    requires (not std::is_lvalue_reference_v<T>)
{
    using Self = std::remove_cv_t<T>;
    return Self(std::move(v));
}
#endif

template<typename SourceObservable>
struct Observable_
{
    using value_type   = typename SourceObservable::value_type;
    using error_type   = typename SourceObservable::error_type;
    using Unsubscriber = typename SourceObservable::Unsubscriber;

    SourceObservable _source;

    template<typename Observer>
        requires ValueObserverOf<Observer, value_type>
    decltype(auto) subscribe(Observer&& observer) &&
    {
        return std::move(_source).subscribe(std::forward<Observer>(observer));
    }

    template<typename F>
    auto filter(F&& f) &&
    {
        return make_operator(detail::operator_tag::Filter()
            , std::move(*this)
            , std::forward<F>(f));
    }
};

namespace detail
{
    template<typename SourceObservable, typename Filter>
    struct FilterObservable
    {
        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;
        Filter _filter;

        template<typename Observer>
        struct ProxyObserver : Observer
        {
            explicit ProxyObserver(Observer&& o, Filter&& f)
                : Observer(std::move(o))
                , _filter(std::move(f)) {}
            Filter _filter;
            Observer& observer() { return *this; }

            template<typename Value>
            void on_next(Value&& v)
            {
                if (_filter(v))
                {
                    (void)observer().on_next(std::forward<Value>(v));
                }
            }
        };

        template<typename Observer>
            requires ValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            auto strict = make_strict(std::forward<Observer>(observer));
            using Observer_ = decltype(strict);
            using Proxy     = ProxyObserver<Observer_>;
            return std::move(_source).subscribe(Proxy(std::move(strict), std::move(_filter)));
        }
    };
} // namespace detail

template<typename SourceObservable, typename F>
auto tag_invoke(tag_t<make_operator>, detail::operator_tag::Filter
    , SourceObservable source, F filter)
        requires std::predicate<F, typename SourceObservable::value_type>
{
    using Impl = detail::FilterObservable<SourceObservable, F>;
    return Observable_<Impl>(Impl(std::move(source), std::move(filter)));
}

template<typename Value, typename Error>
struct AnyObserver;

namespace detail
{
    template<typename>
    struct IsAnyObserver : std::false_type {};
    template<typename Value, typename Error>
    struct IsAnyObserver<AnyObserver<Value, Error>> : std::true_type {};
} // namespace detail

template<typename Value, typename Error>
struct AnyObserver
{
    struct ObserverConcept
    {
        virtual ~ObserverConcept() = default;
        virtual std::unique_ptr<ObserverConcept> copy_() const = 0;
        virtual void on_next(Value v) = 0;
        virtual void on_error(Error e) = 0;
        virtual void on_completed() = 0;
    };

    template<typename ConcreateObserver>
    struct Observer : ObserverConcept
    {
        Observer(const Observer&) = delete;
        Observer(Observer&&) = delete;
        Observer& operator=(const Observer&) = delete;
        Observer& operator=(Observer&&) = delete;

        virtual std::unique_ptr<ObserverConcept> copy_() const override
        {
            return std::make_unique<Observer>(_observer);
        }

        virtual void on_next(Value v) override
        {
            return ::on_next(_observer, std::forward<Value>(v));
        }

        virtual void on_error(Error e) override
        {
            return ::on_error(std::move(_observer), std::forward<Error>(e));
        }

        virtual void on_completed()
        {
            return ::on_completed(std::move(_observer));
        }

        explicit Observer(ConcreateObserver o)
            : _observer(std::move(o))
        {
        }

        ConcreateObserver _observer;
    };

    template<typename ConcreateObserver>
        requires (!detail::IsAnyObserver<ConcreateObserver>::value)
    explicit AnyObserver(ConcreateObserver o)
        : _observer(std::make_unique<Observer<ConcreateObserver>>(std::move(o)))
    {
    }

    AnyObserver(const AnyObserver& rhs)
        : _observer((assert(rhs._observer), rhs._observer->copy_()))
    {
    }
    AnyObserver& operator=(const AnyObserver& rhs)
    {
        if (this != &rhs)
        {
            assert(rhs._observer);
            _observer = rhs._observer->copy_();
        }
        return *this;
    }
    AnyObserver(AnyObserver& rhs) noexcept
        : _observer(std::move(rhs._observer))
    {
    }
    AnyObserver& operator=(AnyObserver& rhs) noexcept
    {
        if (this != &rhs)
        {
            _observer = std::move(rhs._observer);
        }
        return *this;
    }

    std::unique_ptr<ObserverConcept> _observer;

    void on_next(Value v)
    {
        assert(_observer);
        return _observer->on_next(std::forward<Value>(v));
    }

    void on_error(Error e)
    {
        assert(_observer);
        return _observer->on_error(std::forward<Error>(e));
    }

    void on_completed()
    {
        assert(_observer);
        return _observer->on_completed();
    }
};

template<typename Value, typename Error>
struct Subject_
{
    struct Subscription
    {
        AnyObserver<Value, Error> _observer;
        std::uint32_t _index = 0;
        std::uint32_t _version = 0;
    };

    struct SharedImpl_
    {
        std::uint32_t _version = 0;
        std::vector<Subscription> _subscriptions;
    };

    struct Unsubsriber
    {
        using has_effect = std::true_type;

        std::weak_ptr<SharedImpl_> _shared_weak;
        std::uint32_t _version = 0;
        std::uint32_t _index = 0;

        bool detach()
        {
            auto shared = _shared_weak.lock();
            if (not shared)
            {
                return false;
            }
            auto& subscriptions = shared->_subscriptions;
            if (_index < subscriptions.size())
            {
                auto it = subscriptions.begin() + _index;
                if (it->_version == _version)
                {
                    subscriptions.erase(it);
                    return true;
                }
            }
            else
            {
                auto it = std::find_if(std::begin(subscriptions), std::end(subscriptions)
                    , [&](const Subscription& s)
                {
                    return ((s._index == _index) && (s._version == _version));
                });
                if (it != std::end(subscriptions))
                {
                    subscriptions.erase(it);
                    return true;
                }
            }
            return false;
        }
    };

    std::shared_ptr<SharedImpl_> _shared;

    explicit Subject_()
        : _shared(std::make_shared<SharedImpl_>())
    {
    }

    struct SourceObservable
    {
        using value_type   = Value;
        using error_type   = Error;
        using Unsubscriber = Subject_::Unsubsriber;

        std::shared_ptr<SharedImpl_> _shared;

        template<typename Observer>
            requires ValueObserverOf<Observer, Value>
        Unsubsriber subscribe(Observer&& observer)
        {
            using O_ = AnyObserver<Value, Error>;
            assert(_shared);

            auto strict = detail::make_strict(std::forward<Observer>(observer));
            const std::uint32_t index = std::uint32_t(_shared->_subscriptions.size());
            const std::uint32_t version = ++_shared->_version;

            Subscription subscription{O_(std::move(strict))};
            subscription._version = version;
            subscription._index = index;
            Unsubsriber unsubscriber;
            unsubscriber._shared_weak = _shared;
            unsubscriber._version = subscription._version;
            unsubscriber._index = subscription._index;

            _shared->_subscriptions.push_back(std::move(subscription));
            return unsubscriber;
        }
    };

    Observable_<SourceObservable> as_observable() const
    {
        return Observable_<SourceObservable>(SourceObservable(_shared));
    }

    Subject_ as_observer() const
    {
        return Subject_(_shared);
    }

    void on_next(Value v)
    {
        assert(_shared);
        // Using raw for with computing size() every loop for a case where
        // un-subscription can happen in the callback.
        for (std::size_t i = 0; i < _shared->_subscriptions.size(); ++i)
        {
            _shared->_subscriptions[i]._observer.on_next(v);
        }
    }

    void on_error(Error e)
    {
        assert(_shared);
        for (std::size_t i = 0; i < _shared->_subscriptions.size(); ++i)
        {
            _shared->_subscriptions[i]._observer.on_error(e);
        }
    }

    void on_completed()
    {
        assert(_shared);
        for (std::size_t i = 0; i < _shared->_subscriptions.size(); ++i)
        {
            _shared->_subscriptions[i]._observer.on_completed();
        }
    }
};

#include <cstdio>

int main()
{
    InitialSourceObservable_ initial;
    using O = Observable_<InitialSourceObservable_>;
    O observable(std::move(initial));

    {
        auto unsubscriber = O(observable).subscribe([](int value)
        {
            std::printf("Observer: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    {
        auto unsubscriber = O(observable)
            .filter([](int)   { return true; })
            .filter([](int v) { return ((v % 2) == 0); })
            .filter([](int)   { return true; })
            .subscribe([](int value)
        {
            std::printf("Filtered even numbers: %i.\n", value);
        });
        using Unsubscriber = typename decltype(unsubscriber)::Unsubscriber;
        static_assert(std::is_same_v<Unsubscriber, InitialSourceObservable_::Unsubscriber>);
        static_assert(not Unsubscriber::has_effect());
    }

    Subject_<int, int> subject;
    using ExpectedUnsubscriber = typename decltype(subject)::Unsubsriber;

    auto unsubscriber = subject.as_observable()
        .filter([](int)   { return true; })
        .filter([](int v) { return ((v % 2) == 0); })
        .filter([](int)   { return true; })
        .subscribe(observer::make([](int value)
    {
        std::printf("[Subject1] Filtered even numbers: %i.\n", value);
    }
        , []()
    {
        std::printf("[Subject1] Completed.\n");
    }));
    static_assert(std::is_same_v<decltype(unsubscriber), ExpectedUnsubscriber>);
    static_assert(ExpectedUnsubscriber::has_effect());

    subject.as_observable()
        .filter([](int v) { return (v == 3); })
        .subscribe([](int value)
    {
        std::printf("[Subject2] Exactly 3: %i.\n", value);
    });

    subject.on_next(1);
    subject.on_next(2);

    unsubscriber.detach();

    subject.on_next(3);
    subject.on_next(4);
    subject.on_completed();
}
