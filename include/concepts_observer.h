#pragma once
#include "xrx_prologue.h"
#include "tag_invoke_with_extension.h"
#include "meta_utils.h"

#include <utility>
#include <type_traits>
#include <functional> // std::invoke

namespace xrx
{
    struct unsubscribe
    {
        const bool _stop;
        constexpr explicit unsubscribe(bool do_unsubscribe = true)
            : _stop(do_unsubscribe)
        {
        }
    };
} // namespace xrx

namespace xrx::detail
{
    struct on_next_fn
    {
        // on_next() with single argument T
        template<typename S, typename T>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve1_(S&& s, T&& v, priority_tag<2>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s), std::forward<T>(v))))
               requires tag_invocable<on_next_fn, S, T>
        {
            return tag_invoke(*this, std::forward<S>(s), std::forward<T>(v));
        }
        template<typename S, typename T>
        XRX_FORCEINLINE() constexpr auto resolve1_(S&& s, T&& v, priority_tag<1>) const
            noexcept(noexcept(std::forward<S>(s).on_next(std::forward<T>(v))))
                -> decltype(std::forward<S>(s).on_next(std::forward<T>(v)))
        {
            return std::forward<S>(s).on_next(std::forward<T>(v));
        }
        template<typename S, typename T>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve1_(S&& s, T&& v, priority_tag<0>) const
            noexcept(noexcept(std::invoke(std::forward<S>(s), std::forward<T>(v))))
                requires std::invocable<S, T>
        {
            return std::invoke(std::forward<S>(s), std::forward<T>(v));
        }
        template<typename S, typename T>
        XRX_FORCEINLINE() /*[[nodiscard]]*/ constexpr auto operator()(S&& s, T&& v) const
            noexcept(noexcept(resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>())))
                -> decltype(resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>()))
        {
            return resolve1_(std::forward<S>(s), std::forward<T>(v), priority_tag<2>());
        }
        // on_next() with no argument.
        template<typename S>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve0_(S&& s, priority_tag<2>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
                requires tag_invocable<on_next_fn, S>
        {
            return tag_invoke(*this, std::forward<S>(s));
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr auto resolve0_(S&& s, priority_tag<1>) const
            noexcept(noexcept(std::forward<S>(s).on_next()))
                -> decltype(std::forward<S>(s).on_next())
        {
            return std::forward<S>(s).on_next();
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve0_(S&& s, priority_tag<0>) const
            noexcept(noexcept(std::invoke(std::forward<S>(s))))
                requires std::invocable<S>
        {
            return std::invoke(std::forward<S>(s));
        }
        template<typename S>
        XRX_FORCEINLINE() /*[[nodiscard]]*/ constexpr auto operator()(S&& s) const
            noexcept(noexcept(resolve0_(std::forward<S>(s), priority_tag<2>())))
                -> decltype(resolve0_(std::forward<S>(s), priority_tag<2>()))
        {
            return resolve0_(std::forward<S>(s), priority_tag<2>());
        }
    };
    
    inline const on_next_fn on_next{};

    struct on_completed_fn
    {
        template<typename S>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve0_(S&& s, priority_tag<1>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
                requires tag_invocable<on_completed_fn, S>
        {
            return tag_invoke(*this, std::forward<S>(s));
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr auto resolve0_(S&& s, priority_tag<0>) const
            noexcept(noexcept(std::forward<S>(s).on_completed()))
                -> decltype(std::forward<S>(s).on_completed())
        {
            return std::forward<S>(s).on_completed();
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr auto operator()(S&& s) const
            noexcept(noexcept(resolve0_(std::forward<S>(s), priority_tag<1>())))
                -> decltype(resolve0_(std::forward<S>(s), priority_tag<1>()))
        {
            return resolve0_(std::forward<S>(s), priority_tag<1>());
        }
    };

    inline const on_completed_fn on_completed{};

    struct on_error_fn
    {
        template<typename S, typename E>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve1_(S&& s, E&& e, priority_tag<1>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s), std::forward<E>(e))))
                requires tag_invocable<on_error_fn, S, E>
        {
            return tag_invoke(*this, std::forward<S>(s), std::forward<E>(e));
        }
        template<typename S, typename E>
        XRX_FORCEINLINE() constexpr auto resolve1_(S&& s, E&& e, priority_tag<0>) const
            noexcept(noexcept(std::forward<S>(s).on_error(std::forward<E>(e))))
                -> decltype(std::forward<S>(s).on_error(std::forward<E>(e)))
        {
            return std::forward<S>(s).on_error(std::forward<E>(e));
        }
        template<typename S, typename E>
        XRX_FORCEINLINE() constexpr auto operator()(S&& s, E&& e) const
            noexcept(noexcept(resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>())))
                -> decltype(resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>()))
        {
            return resolve1_(std::forward<S>(s), std::forward<E>(e), priority_tag<1>());
        }
    };

    inline const on_error_fn on_error{};

    template<typename Observer, typename Value>
    concept ConceptWithOnNext = is_cpo_invocable_v<tag_t<on_next>, Observer, Value>;
    template<typename Observer>
    concept ConceptWithOnCompleted = is_cpo_invocable_v<tag_t<on_completed>, Observer>;
    template<typename Observer, typename Error>
    concept ConceptWithOnError = is_cpo_invocable_v<tag_t<on_error>, Observer, Error>;
} // namespace xrx::detail

namespace xrx
{
    // Simplest possible observer with only on_next() callback.
    // No error handling or on_complete() detection.
    template<typename Observer, typename Value>
    concept ConceptValueObserverOf
        = detail::ConceptWithOnNext<Observer, Value>;

    // Full/strict definition of observer:
    // - on_next(Value)
    // - on_error(Error)
    // - on_completed()
    template<typename Observer, typename Value, typename Error>
    concept ConceptObserverOf
        =    detail::ConceptWithOnNext<Observer, Value>
          && detail::ConceptWithOnCompleted<Observer>
          && detail::ConceptWithOnError<Observer, Error>;
} // namespace xrx

#include "xrx_epilogue.h"
