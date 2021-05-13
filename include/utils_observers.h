#pragma once
#include "concepts_observer.h"
#include "debug/assert_flag.h"

#include <type_traits>
#include <utility>

namespace xrx::detail
{
    struct [[nodiscard]] OnNextAction
    {
        bool _unsubscribe = false;
    };

    template<typename Action>
    OnNextAction ensure_action_state(OnNextAction action, debug::AssertFlag<Action>& unsubscribed)
    {
        if (action._unsubscribe)
        {
            unsubscribed.raise();
        }
        return action;
    }

    template<typename Observer, typename Value>
    OnNextAction on_next_with_action(Observer&& observer, Value&& value)
        requires ConceptWithOnNext<Observer, Value>
    {
        using Return_ = decltype(::xrx::detail::on_next(std::forward<Observer>(observer), std::forward<Value>(value)));
        static_assert(not std::is_reference_v<Return_>
            , "Return by value only allowed fron on_next() callback. To simplify implementation below.");
        if constexpr (std::is_same_v<Return_, void>)
        {
            (void)::xrx::detail::on_next(std::forward<Observer>(observer), std::forward<Value>(value));
            return OnNextAction();
        }
        else if constexpr (std::is_same_v<Return_, OnNextAction>)
        {
            return ::xrx::detail::on_next(std::forward<Observer>(observer), std::forward<Value>(value));
        }
        else if constexpr (std::is_same_v<Return_, ::xrx::unsubscribe>)
        {
            const ::xrx::unsubscribe state = ::xrx::detail::on_next(std::forward<Observer>(observer), std::forward<Value>(value));
            return OnNextAction{ ._unsubscribe = state._do_unsubscribe };
        }
        else
        {
            static_assert(AlwaysFalse<Observer>()
                , "Unknown return type from ::on_next(Value). New tag to handle ?");
        }
    }

    // #TODO: merge those 2 functions.
    template<typename Observer, typename Value>
    OnNextAction on_next_with_action(Observer&& observer)
        requires ConceptWithOnNext<Observer, void>
    {
        using Return_ = decltype(::xrx::detail::on_next(std::forward<Observer>(observer)));
        static_assert(not std::is_reference_v<Return_>
            , "Return by value only allowed fron on_next() callback. To simplify implementation below.");
        if constexpr (std::is_same_v<Return_, void>)
        {
            (void)::xrx::detail::on_next(std::forward<Observer>(observer));
            return OnNextAction();
        }
        else if constexpr (std::is_same_v<Return_, OnNextAction>)
        {
            return ::xrx::detail::on_next(std::forward<Observer>(observer));
        }
        else if constexpr (std::is_same_v<Return_, ::xrx::unsubscribe>)
        {
            const ::xrx::unsubscribe state = ::xrx::detail::on_next(std::forward<Observer>(observer));
            return OnNextAction{ ._unsubscribe = state._do_unsubscribe };
        }
        else
        {
            static_assert(AlwaysFalse<Observer>()
                , "Unknown return type from ::on_next(). New tag to handle ?");
        }
    }

    template<typename Observer, typename Error>
    void on_error_optional(Observer&& observer, Error&& error)
        requires ConceptWithOnError<Observer, Error>
    {
        return ::xrx::detail::on_error(std::forward<Observer>(observer), std::forward<Error>(error));
    }
    template<typename Observer, typename Error>
    void on_error_optional(Observer&&, Error&&)
        requires (not ConceptWithOnError<Observer, Error>)
    {
    }

    template<typename Observer>
    void on_error_optional(Observer&& observer)
        requires ConceptWithOnError<Observer, void>
    {
        return ::xrx::detail::on_error(std::forward<Observer>(observer));
    }
    template<typename Observer>
    void on_error_optional(Observer&&)
        requires (not ConceptWithOnError<Observer, void>)
    {
    }

    template<typename Observer>
    auto on_completed_optional(Observer&& o)
        requires ConceptWithOnCompleted<Observer>
    {
        return ::xrx::detail::on_completed(std::forward<Observer>(o));
    }
    
    template<typename Observer>
    auto on_completed_optional(Observer&&)
        requires (not ConceptWithOnCompleted<Observer>)
    {
    }

    struct OnNext_Noop
    {
        template<typename Value>
        constexpr void on_next(Value&&) const noexcept
        {
        }
        constexpr void on_next() const noexcept
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
            requires ConceptWithOnNext<OnNext, Value>
        {
            return ::xrx::detail::on_next(_on_next, std::forward<Value>(value));
        }

        constexpr decltype(auto) on_next()
            requires ConceptWithOnNext<OnNext, void>
        {
            return ::xrx::detail::on_next(_on_next);
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
    struct ObserverRef
    {
        Observer* _ref = nullptr;

        template<typename Value>
        auto on_next(Value&& v) { return _ref->on_next(XRX_FWD(v)); }
        
        template<typename Error>
            requires ConceptWithOnError<Observer, Error>
        auto on_error(Error&& e)
        { return _ref->on_error(XRX_FWD(e)); }
        
        auto on_error()
            requires ConceptWithOnError<Observer, void>
        { return _ref->on_error(); }
        
        auto on_completed() { return _ref->on_completed(); }
    };
} // namespace xrx::detail

namespace xrx::observer
{
    template<typename Observer_>
    auto ref(Observer_& observer)
    {
        return ::xrx::detail::ObserverRef<Observer_>(&observer);
    }

    template<typename OnNext>
    auto make(OnNext&& on_next)
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = ::xrx::detail::LambdaObserver<F_, ::xrx::detail::OnCompleted_Noop, ::xrx::detail::OnError_Noop>;
        return Observer(std::forward<OnNext>(on_next), {}, {});
    }

    template<typename Value, typename OnNext>
    ConceptValueObserverOf<Value> auto
        make_of(OnNext&& on_next)
            requires ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_next>, OnNext, Value>
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = ::xrx::detail::LambdaObserver<F_, ::xrx::detail::OnCompleted_Noop, ::xrx::detail::OnError_Noop>;
        return Observer(std::forward<OnNext>(on_next), {}, {});
    }

    template<typename OnNext, typename OnCompleted, typename OnError = ::xrx::detail::OnError_Noop>
    auto make(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error = {})
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = ::xrx::detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(std::forward<OnNext>(on_next)
            , std::forward<OnCompleted>(on_completed)
            , std::forward<OnError>(on_error));
    }

    template<typename Value, typename Error
        , typename OnNext, typename OnCompleted, typename OnError>
    ConceptObserverOf<Value, Error> auto
        make_of(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error)
            requires ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_next>, OnNext, Value> &&
                     ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_error>, OnError, Error>
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = ::xrx::detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(std::forward<OnNext>(on_next)
            , std::forward<OnCompleted>(on_completed)
            , std::forward<OnError>(on_error));
    }
} // namespace xrx::observer

