#pragma once
// 
// Auto-generated single-header file:
// powershell -File tools/generate_single_header.ps1
// 

// Header: debug/assert_mutex.h.

// #pragma once
#include <shared_mutex>
#include <mutex>
#include <utility>
#include <cassert>

namespace xrx::debug
{
    namespace detail
    {
        struct AssertAction
        {
            void operator()() const
            {
                assert(false && "Contention on the lock detected.");
            }
        };
    } // namespace detail

    template<typename Action = detail::AssertAction>
    class AssertMutex
    {
    public:
        explicit AssertMutex() noexcept
            : _action()
        {
        }

        explicit AssertMutex(Action action) noexcept
            : _action(std::move(action))
        {
        }

        void lock()
        {
            if (not _mutex.try_lock())
            {
                _action();
            }
        }

        void unlock()
        {
            _mutex.unlock();
        }

    private:
        [[no_unique_address]] Action _action;
        std::shared_mutex _mutex;
    };

    template<typename Mutex>
    struct ScopeUnlock
    {
        ScopeUnlock(const ScopeUnlock&) = delete;
        ScopeUnlock& operator=(const ScopeUnlock&) = delete;
        ScopeUnlock(ScopeUnlock&&) = delete;
        ScopeUnlock& operator=(ScopeUnlock&&) = delete;

        std::unique_lock<Mutex>& _lock;

        explicit ScopeUnlock(std::unique_lock<Mutex>& lock)
            : _lock(lock)
        {
            _lock.unlock();
        }

        ~ScopeUnlock()
        {
            _lock.lock();
        }
    };
} // namespace xrx::debug


// Header: tag_invoke.hpp.

// #pragma once
// FROM Eric Niebler:
// https://gist.github.com/ericniebler/056f5459cf259da526d9ea2279c386bb/#file-tag_invoke-hpp
// Simple implementation of the tag_invoke customization point.
// 
#include <utility>
#include <type_traits>

namespace xrx { // #ZZZ: modification.
namespace _tag_invoke {
  void tag_invoke();

  struct _fn {
    template <typename CPO, typename... Args>
    constexpr auto operator()(CPO cpo, Args&&... args) const
        noexcept(noexcept(tag_invoke((CPO &&) cpo, (Args &&) args...)))
        -> decltype(tag_invoke((CPO &&) cpo, (Args &&) args...)) {
      return tag_invoke((CPO &&) cpo, (Args &&) args...);
    }
  };

  template <typename CPO, typename... Args>
  using tag_invoke_result_t = decltype(
      tag_invoke(std::declval<CPO>(), std::declval<Args>()...));

  using yes_type = char;
  using no_type = char(&)[2];

  template <typename CPO, typename... Args>
  auto try_tag_invoke(int) //
      noexcept(noexcept(tag_invoke(
          std::declval<CPO>(), std::declval<Args>()...)))
      -> decltype(static_cast<void>(tag_invoke(
          std::declval<CPO>(), std::declval<Args>()...)), yes_type{});

  template <typename CPO, typename... Args>
  no_type try_tag_invoke(...) noexcept(false);

  template <template <typename...> class T, typename... Args>
  struct defer {
    using type = T<Args...>;
  };

  struct empty {};
}  // namespace _tag_invoke

namespace _tag_invoke_cpo {
  inline constexpr _tag_invoke::_fn tag_invoke{};
}
using namespace _tag_invoke_cpo;

template <auto& CPO>
using tag_t = std::remove_cvref_t<decltype(CPO)>;

using _tag_invoke::tag_invoke_result_t;

template <typename CPO, typename... Args>
inline constexpr bool is_tag_invocable_v =
    (sizeof(_tag_invoke::try_tag_invoke<CPO, Args...>(0)) ==
     sizeof(_tag_invoke::yes_type));

template <typename CPO, typename... Args>
struct tag_invoke_result
  : std::conditional_t<
        is_tag_invocable_v<CPO, Args...>,
        _tag_invoke::defer<tag_invoke_result_t, CPO, Args...>,
        _tag_invoke::empty> 
{};

template <typename CPO, typename... Args>
using is_tag_invocable = std::bool_constant<is_tag_invocable_v<CPO, Args...>>;

template <typename CPO, typename... Args>
inline constexpr bool is_nothrow_tag_invocable_v =
    noexcept(_tag_invoke::try_tag_invoke<CPO, Args...>(0));

template <typename CPO, typename... Args>
using is_nothrow_tag_invocable =
    std::bool_constant<is_nothrow_tag_invocable_v<CPO, Args...>>;

template <typename CPO, typename... Args>
concept tag_invocable =
    (sizeof(_tag_invoke::try_tag_invoke<CPO, Args...>(0)) ==
     sizeof(_tag_invoke::yes_type));
} // namespace xrx
// #ZZZ: modification.

// Header: utils_containers.h.

// #pragma once
#include <type_traits>
#include <vector>
#include <limits>

#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename T>
    struct HandleVector
    {
        static_assert(std::is_default_constructible_v<T>
            , "T must be default constructible for erase() support.");
        static_assert(std::is_move_assignable_v<T>
            , "T must be move assignable for erase() support.");
        static_assert(std::is_move_constructible_v<T>
            , "T must be move constructible for erase() support.");

        struct Value
        {
            T _data{};
            std::uint32_t _version = 0;
        };

        struct Handle
        {
            std::uint32_t _version = 0;
            std::uint32_t _index = 0;

            explicit operator bool() const
            {
                return (_version > 0);
            }
        };

        std::uint32_t _version = 0;
        std::vector<std::size_t> _free_indexes;
        std::vector<Value> _values;

        template<typename U>
        Handle push_back(U&& v)
        {
            const std::uint32_t version = ++_version;
            if (_free_indexes.empty())
            {
                const std::size_t index = _values.size();
                assert(index <= std::size_t(std::numeric_limits<std::uint32_t>::max()));
                _values.push_back(Value(T(std::forward<U>(v)), version));
                return Handle(version, std::uint32_t(index));
            }
            const std::size_t index = _free_indexes.back();
            _free_indexes.pop_back();
            _values[index] = Value(T(std::forward<U>(v)), version);
            return Handle(version, std::uint32_t(index));
        }

        std::size_t size() const
        {
            assert(_values.size() >= _free_indexes.size());
            return (_values.size() - _free_indexes.size());
        }

        bool erase(Handle h)
        {
            assert(h._version > 0);
            if (h._index < _values.size())
            {
                auto it = std::begin(_values) + h._index;
                if (it->_version == h._version)
                {
                    if (_values.size() == 1) // last one
                    {
                        _values.clear();
                    }
                    else
                    {
                        _values[h._index] = Value(T(), 0);
                        _free_indexes.push_back(h._index);
                    }
                    return true;
                }
            }
            return false;
        }

        template<typename F>
        void for_each(F&& f)
            requires requires { f(std::declval<T&>()); }
        {
            // Using raw for with computing size() every loop for a case where
            // `f()` can erase element from our vector we iterate.
            for (std::size_t i = 0; i < _values.size(); ++i)
            {
                if (_values[i]._version > 0)
                {
                    f(_values[i]._data);
                }
            }
        }

        template<typename F>
        void for_each(F&& f)
            requires requires { f(std::declval<T&>(), Handle()); }
        {
            // Using raw for with computing size() every loop for a case where
            // `f()` can erase element from our vector we iterate.
            for (std::size_t i = 0; i < _values.size(); ++i)
            {
                if (_values[i]._version > 0)
                {
                    f(_values[i]._data
                        , Handle(_values[i]._version, std::uint32_t(i)));
                }
            }
        }

        T* get(Handle h)
        {
            if (h._index < _values.size())
            {
                if (_values[h._index]._version == h._version)
                {
                    return &_values[h._index]._data;
                }
            }
            return nullptr;
        }
    };
} // namespace xrx::detail

// Header: debug/assert_flag.h.

// #pragma once
#include <cassert>

// This is specifically made to be ugly.
// TO BE REMOVED.
// 
namespace xrx::debug
{
    namespace detail
    {
        struct AssertFlagAction
        {
            void operator()() const
            {
                assert(false && "Debug flag is unexpectedly set.");
            }
        };
    } // namespace detail

    template<typename Action = detail::AssertFlagAction>
    class AssertFlag
    {
    public:
        explicit AssertFlag() noexcept
            : _action()
            , _state(false)
        {
        }

        void check_not_set() const
        {
            if (_state)
            {
                _action();
            }
        }

        void raise()
        {
            _state = true;
        }
        
    private:
        [[no_unique_address]] Action _action;
        bool _state = false;
    };
} // namespace xrx::debug

// Header: utils_observable.h.

// #pragma once
#include <type_traits>

namespace xrx
{
    struct NoopUnsubscriber
    {
        using has_effect = std::false_type;
        constexpr bool detach() const noexcept { return false; }
    };
} // namespace xrx

// Header: meta_utils.h.

// #pragma once
#include <type_traits>

namespace xrx::detail
{
    template<unsigned I>
    struct priority_tag
        : priority_tag<I - 1> {};
    template<>
    struct priority_tag<0> {};

    template<typename>
    struct AlwaysFalse : std::false_type {};

    struct none_tag {};
} // namespace xrx::detail


// Header: tag_invoke_with_extension.h.

// #pragma once
// #include "tag_invoke.hpp"

#include <utility>

namespace xrx::detail
{
    template<typename CPO, typename... Args>
    concept ConceptCPOInvocable = requires(CPO& cpo, Args&&... args)
    {
        cpo(std::forward<Args>(args)...);
    };

    template<typename CPO, typename... Args>
    constexpr bool is_cpo_invocable_v = ConceptCPOInvocable<CPO, Args...>;
} // namespace xrx::detail

// Header: concepts_observable.h.

// #pragma once
// #include "meta_utils.h"
#include <type_traits>
#include <concepts>

namespace xrx::detail
{
    template<typename Handle_>
    concept ConceptHandle =
           std::is_copy_constructible_v<Handle_>
        && std::is_move_constructible_v<Handle_>;
    // #TODO: revisit is_copy/move_assignable.

    template<typename Unsubscriber_>
    concept ConceptUnsubscriber =
           ConceptHandle<Unsubscriber_>
        && requires(Unsubscriber_ unsubscriber)
    {
        // MSVC fails to compile ConceptObservable<> if this line
        // is enabled (because of subscribe() -> ConceptObservable<> check.
        // typename Unsubscriber_::has_effect;

        { unsubscriber.detach() } -> std::convertible_to<bool>;
    };

    template<typename Value, typename Error>
    struct ObserverArchetype
    {
        void on_next(Value);
        void on_error(Error);
        void on_completed();
    };

    template<typename Value>
    struct ObserverArchetype<Value, void>
    {
        void on_next(Value);
        void on_error();
        void on_completed();
    };

    template<typename Value>
    struct ObserverArchetype<Value, none_tag>
    {
        void on_next(Value);
        void on_error();
        void on_completed();
    };

    template<typename ObservableLike_>
    concept ConceptObservable =
           (not std::is_reference_v<ObservableLike_>)
        && ConceptHandle<ObservableLike_>
        && requires(ObservableLike_ observable)
    {
        typename ObservableLike_::value_type;
        typename ObservableLike_::error_type;
        typename ObservableLike_::Unsubscriber;

        { std::declval<ObservableLike_>().subscribe(ObserverArchetype<
              typename ObservableLike_::value_type
            , typename ObservableLike_::error_type>()) }
                -> ConceptUnsubscriber<>;
        // #TODO: should return Self - ConceptObservable<> ?
        { std::declval<ObservableLike_&>().fork() }
            -> ConceptHandle<>;
    };
} // namespace xrx::detail

// Header: cpo_make_operator.h.

// #pragma once
// #include "tag_invoke.hpp"

#include <utility>

namespace xrx::detail
{
    struct make_operator_fn
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
    };

    inline const make_operator_fn make_operator;
} // namespace xrx::detail

// Header: operator_tags.h.

// #pragma once
// #include "meta_utils.h"

namespace xrx::detail::operator_tag
{
    struct Publish
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .publish() operator implementation. "
                  "Missing `operators/operator_publish.h` include ?");
        };
    };

    struct SubscribeOn
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .subscribe_on() operator implementation. "
                  "Missing `operators/operator_subscribe_on.h` include ?");
        };
    };

    struct ObserveOn
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .observe_on() operator implementation. "
                  "Missing `operators/operator_observe_on.h` include ?");
        };
    };

    struct Filter
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .filter() operator implementation. "
                  "Missing `operators/operator_filter.h` include ?");
        };
    };

    struct Transform
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .transform() operator implementation. "
                  "Missing `operators/operator_transform.h` include ?");
        };
    };

    struct Interval
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .interval() operator implementation. "
                  "Missing `operators/operator_interval.h` include ?");
        };
    };

    template<typename Value, typename Error>
    struct Create
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .create() operator implementation. "
                  "Missing `operators/operator_create.h` include ?");
        };
    };

    struct Range
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .range() operator implementation. "
                  "Missing `operators/operator_range.h` include ?");
        };
    };

    struct Take
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .take() operator implementation. "
                  "Missing `operators/operator_take.h` include ?");
        };
    };

} // namespace xrx::detail::operator_tag

// Header: concepts_observer.h.

// #pragma once
// #include "tag_invoke_with_extension.h"
// #include "meta_utils.h"

#include <utility>
#include <type_traits>
#include <functional> // std::invoke

namespace xrx
{
    struct unsubscribe
    {
        const bool _do_unsubscribe;
        constexpr explicit unsubscribe(bool do_unsubscribe = true)
            : _do_unsubscribe(do_unsubscribe)
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
        [[nodiscard]] constexpr auto operator()(S&& s, T&& v) const
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
        [[nodiscard]] constexpr auto operator()(S&& s) const
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
    };

    inline const on_completed_fn on_completed{};

    struct on_error_fn
    {
        // on_error() with single argument.
        template<typename S, typename E>
        constexpr decltype(auto) resolve1_(S&& s, E&& e, priority_tag<1>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s), std::forward<E>(e))))
                requires tag_invocable<on_error_fn, S, E>
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
                requires tag_invocable<on_error_fn, S>
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
    };

    inline const on_error_fn on_error{};

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
    concept ConceptWithOnNext = OnNext_Invocable<Observer, Value>::value;
    template<typename Observer>
    concept ConceptWithOnCompleted = OnCompleted_Invocable<Observer>::value;
    template<typename Observer, typename Error>
    concept ConceptWithOnError = OnError_Invocable<Observer, Error>::value;
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

// Header: observables_factory.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
#include <utility>

namespace xrx::observable
{
    template<typename Duration, typename Scheduler>
    auto interval(Duration period, Scheduler scheduler)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Interval()
            , std::move(period), std::move(scheduler));
    }

    template<typename Value, typename Error = void, typename F>
    auto create(F on_subscribe)
    {
        using Tag_ = xrx::detail::operator_tag::Create<Value, Error>;
        return ::xrx::detail::make_operator(Tag_()
            , std::move(on_subscribe));
    }

    template<typename Integer>
    auto range(Integer start = Integer())
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , start, 1, 1);
    }
} // namespace xrx::observable

// Header: utils_observers.h.

// #pragma once
// #include "concepts_observer.h"
// #include "debug/assert_flag.h"

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
        requires requires { ::xrx::detail::on_next(std::forward<Observer>(observer), std::forward<Value>(value)); }
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
            return OnNextAction{._unsubscribe = state._do_unsubscribe};
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
        requires requires { ::xrx::detail::on_next(std::forward<Observer>(observer)); }
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
            return OnNextAction{._unsubscribe = state._do_unsubscribe};
        }
        else
        {
            static_assert(AlwaysFalse<Observer>()
                , "Unknown return type from ::on_next(). New tag to handle ?");
        }
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
} // namespace xrx::detail

namespace xrx::observer
{
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


// Header: any_observer.h.

// #pragma once
// #include "concepts_observer.h"
// #include "utils_observers.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <cassert>

// Type-erased version of any Observer.
namespace xrx
{
    template<typename Value, typename Error>
    struct AnyObserver;

    namespace detail
    {
        template<typename>
        struct IsAnyObserver : std::false_type {};
        template<typename Value, typename Error>
        struct IsAnyObserver<AnyObserver<Value, Error>> : std::true_type {};
    } // namespace detail

    template<typename Value, typename Error = void>
    struct AnyObserver
    {
#define X_ANY_OBSERVER_SUPPORTS_COPY() 0

        struct ObserverConcept
        {
            virtual ~ObserverConcept() = default;
#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const = 0;
#endif
            virtual ::xrx::detail::OnNextAction on_next(Value v) = 0;
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

#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const override
            {
                return std::make_unique<Observer>(_observer);
            }
#endif

            virtual ::xrx::detail::OnNextAction on_next(Value v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, std::forward<Value>(v));
            }

            virtual void on_error(Error e) override
            {
                return ::xrx::detail::on_error(std::move(_observer), std::forward<Error>(e));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed(std::move(_observer));
            }

            explicit Observer(ConcreateObserver o)
                : _observer(std::move(o))
            {
            }

            ConcreateObserver _observer;
        };

        // Should not be explicit to support implicit conversions
        // on the user side from any observable where AnyObserver can't be
        // directly constructed. Example:
        // observable::create([](AnyObserver<>) {})
        //     .subscribe(CustomObserver()).
        // (Here - conversions from CustomObserver to AnyObserver<> happens
        // when passing arguments to create() callback).
        /*explicit*/ AnyObserver() = default;

        template<typename ConcreateObserver>
            requires (!detail::IsAnyObserver<ConcreateObserver>::value)
        /*explicit*/ AnyObserver(ConcreateObserver o)
            : _observer(std::make_unique<Observer<ConcreateObserver>>(std::move(o)))
        {
        }

#if (X_ANY_OBSERVER_SUPPORTS_COPY())
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
#endif
        AnyObserver(AnyObserver&& rhs) noexcept
            : _observer(std::move(rhs._observer))
        {
        }
        AnyObserver& operator=(AnyObserver&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _observer = std::move(rhs._observer);
            }
            return *this;
        }

        explicit operator bool() const
        {
            return !!_observer;
        }

        std::unique_ptr<ObserverConcept> _observer;

        ::xrx::detail::OnNextAction on_next(Value v)
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
} // namespace xrx

#undef X_ANY_OBSERVER_SUPPORTS_COPY

// Header: observable_interface.h.

// #pragma once
// #include "concepts_observer.h"
// #include "concepts_observable.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"

#include <type_traits>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct Observable_
    {
        static_assert(ConceptObservable<SourceObservable>
            , "SourceObservable should satisfy Observable concept.");

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            return std::move(_source).subscribe(std::forward<Observer>(observer));
        }

        Observable_ fork() { return Observable_(_source.fork()); }

        template<typename Scheduler>
        auto subscribe_on(Scheduler&& scheduler) &&
        {
            return make_operator(detail::operator_tag::SubscribeOn()
                , std::move(*this), std::forward<Scheduler>(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(Scheduler&& scheduler) &&
        {
            return make_operator(detail::operator_tag::ObserveOn()
                , std::move(*this), std::forward<Scheduler>(scheduler));
        }

        auto publish() &&
        {
            return make_operator(detail::operator_tag::Publish()
                , std::move(*this));
        }

        template<typename F>
        auto filter(F&& f) &&
        {
            return make_operator(detail::operator_tag::Filter()
                , std::move(*this), std::forward<F>(f));
        }
        template<typename F>
        auto transform(F&& f) &&
        {
            return make_operator(detail::operator_tag::Transform()
                , std::move(*this), std::forward<F>(f));
        }
        auto take(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Take()
                , std::move(*this), std::size_t(count));
        }
    };
} // namespace xrx::detail

// Header: operators/operator_subscribe_on.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
#include <utility>
#include <variant>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cassert>

namespace xrx::observable
{
    namespace detail
    {
        template<typename SourceObservable, typename Scheduler>
        struct SubscribeStateShared_
            : public std::enable_shared_from_this<SubscribeStateShared_<SourceObservable, Scheduler>>
        {
            using TaskHandle = typename Scheduler::TaskHandle;
            using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

            struct SubscribeInProgress
            {
                std::int32_t _waiting_unsubsribers = 0;
                std::mutex _mutex;
                std::condition_variable _on_finish;
                std::optional<SourceUnsubscriber> _unsubscriber;
            };

            struct SubscribeEnded
            {
                SourceUnsubscriber _unsubscriber;
            };

            struct Subscribed
            {
                std::variant<std::monostate
                    , SubscribeInProgress
                    , SubscribeEnded> _state;
            };

            std::mutex _mutex;
            Scheduler _scheduler;
            bool _try_cancel_subscribe = false;
            std::variant<std::monostate
                , TaskHandle
                , Subscribed> _unsubscribers;

            explicit SubscribeStateShared_(Scheduler sheduler)
                : _mutex()
                , _scheduler(std::move(sheduler))
                , _try_cancel_subscribe(false)
                , _unsubscribers()
            {
            }

            static std::shared_ptr<SubscribeStateShared_> make(Scheduler scheduler)
            {
                return std::make_shared<SubscribeStateShared_>(std::move(scheduler));
            }

            template<typename Observer>
                // requires ConceptValueObserverOf<Observer, value_type>
            void subscribe_impl(SourceObservable source, Observer observer
                , std::true_type) // source unsubscriber does have unsubscribe effect; remember it.
            {
                assert(std::get_if<std::monostate>(&_unsubscribers)); // Not yet initialized.
                // The trick there (I hope correct one) is that we remember weak reference
                // to our state. If at the time of task_post() execution there will be
                // no strong reference, this would mean there is no *active* Unsubscriber.
                // If there is no Unsubscriber, there is no need to remember anything.
                auto shared_weak = this->weak_from_this();

                const TaskHandle task_handle = _scheduler.task_post(
                    [self = shared_weak, source_ = std::move(source), observer_ = std::move(observer)]() mutable
                {
                    auto shared = self.lock(); // remember & extend lifetime.
                    SubscribeInProgress* in_progress = nullptr;
                    if (shared)
                    {
                        const std::lock_guard lock(shared->_mutex);
                        if (shared->_try_cancel_subscribe)
                        {
                            // Unsubscribe was requested just before task schedule.
                            // Do nothing & return.
                            return;
                        }

                        Subscribed* subscribed = nullptr;
                        if (auto* not_initialized = std::get_if<std::monostate>(&shared->_unsubscribers); not_initialized)
                        {
                            // We are first; task_post() executed before subscribe_impl() end.
                            subscribed = &shared->_unsubscribers.template emplace<Subscribed>();
                        }
                        else if (auto* task_handle = std::get_if<TaskHandle>(&shared->_unsubscribers); task_handle)
                        {
                            // We are second; task_post() executed *after* subscribe_impl() end.
                            // Invalidate task handle reference. We can't cancel that anymore.
                            subscribed = &shared->_unsubscribers.template emplace<Subscribed>();
                        }
                        else if (auto* already_subscribing = std::get_if<Subscribed>(&shared->_unsubscribers))
                        {
                            assert(false && "No-one should be able to subscribe to single state twice.");
                        }
                        if (subscribed)
                        {
                            in_progress = &subscribed->_state.template emplace<SubscribeInProgress>();
                        }
                    }

                    SourceUnsubscriber unsubscriber = std::move(source_).subscribe(std::move(observer_));

                    if (in_progress)
                    {
                        assert(shared);
                        const std::lock_guard lock(shared->_mutex);

                        if (shared->_try_cancel_subscribe)
                        {
                            // Racy unsubscribe. Detach now, but also process & resume everyone waiting.
                            unsubscriber.detach();
                            unsubscriber = {};
                        }

                        if (in_progress->_waiting_unsubsribers > 0)
                        {
                            // We were too late. Can't destroy `SubscribeInProgress`.
                            const std::lock_guard lock2(in_progress->_mutex);
                            // Set the data everyone is waiting for.
                            in_progress->_unsubscriber.emplace(std::move(unsubscriber));
                            in_progress->_on_finish.notify_all();
                        }
                        else
                        {
                            // No-one is waiting yet. We can destroy temporary in-progress data
                            // and simply put final Unsubscriber.
                            Subscribed* subscribed = std::get_if<Subscribed>(&shared->_unsubscribers);
                            assert(subscribed && "No-one should change Subscribed state when it was already set");
                            subscribed->_state.template emplace<SubscribeEnded>(std::move(unsubscriber));
                        }
                    }
                });
                {
                    const std::lock_guard lock(_mutex);
                    if (auto* not_initialized = std::get_if<std::monostate>(&_unsubscribers); not_initialized)
                    {
                        _unsubscribers.template emplace<TaskHandle>(std::move(task_handle));
                    }
                    else if (auto* already_subscribing = std::get_if<Subscribed>(&_unsubscribers); already_subscribing)
                    {
                        // No need to remember `task_handle`. We already executed the task, nothing to cancel.
                        (void)task_handle;
                    }
                    else if (auto* task_handle = std::get_if<TaskHandle>(&_unsubscribers); task_handle)
                    {
                        assert(false && "No-one should be able to subscribe to single state twice.");
                    }
                }
            }

            bool detach_impl()
            {
                std::unique_lock lock(_mutex);
                _try_cancel_subscribe = true;

                if (auto* not_initialized = std::get_if<std::monostate>(&_unsubscribers); not_initialized)
                {
                    assert(false && "Unexpected. Calling detach() without valid subscription state");
                    return false;
                }
                else if (auto* task_handle = std::get_if<TaskHandle>(&_unsubscribers); task_handle)
                {
                    // We are not yet in subscribe_impl() task, but still
                    // task cancel can be racy on the Scheduler implementation side.
                    // May subscribe be started. Handled on the subscribe_impl() side with
                    // `_try_cancel_subscribe`. Return value can be incorrect in this case.
                    // (We will unsubscribe for sure, but task is already executed and can't be canceled).
                    return _scheduler.task_cancel(*task_handle);
                }

                Subscribed* subscribed = std::get_if<Subscribed>(&_unsubscribers);
                assert(subscribed);
                if (auto* not_initialized = std::get_if<std::monostate>(&subscribed->_state); not_initialized)
                {
                    assert(false && "Unexpected. Subscribed state with invalid invariant.");
                    return false;
                }
                else if (auto* in_progress = std::get_if<SubscribeInProgress>(&subscribed->_state); in_progress)
                {
                    ++in_progress->_waiting_unsubsribers;
                    lock.unlock(); // Unlock main data.

                    std::optional<SourceUnsubscriber> unsubscriber;
                    {
                        std::unique_lock wait_lock(in_progress->_mutex);
                        in_progress->_on_finish.wait(wait_lock, [in_progress]()
                        {
                            return !!in_progress->_unsubscriber;
                        });
                        // Don't move unsubscriber, there may be multiple unsubscribers waiting.
                        unsubscriber = in_progress->_unsubscriber;
                    }
                    if (unsubscriber)
                    {
                        return unsubscriber->detach();
                    }
                    return false;
                }
                else if (auto* ended = std::get_if<SubscribeEnded>(&subscribed->_state); ended)
                {
                    return ended->_unsubscriber.detach();
                }

                assert(false);
                return false;
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct SubscribeOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Handle = typename Scheduler::TaskHandle;
            using StateShared_ = SubscribeStateShared_<SourceObservable, Scheduler>;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                std::shared_ptr<StateShared_> _shared;

                bool detach()
                {
                    if (_shared)
                    {
                        return _shared->detach_impl();
                    }
                    return false;
                }
            };

            SourceObservable _source;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer&& observer) &&
            {
                auto shared = StateShared_::make(std::move(_scheduler));
#if (0)
                // #TODO: implement subscribe_impl(..., std::false_type);
                using remember_source = typename StateShared_::SourceUnsubscriber::has_effect;
#else
                using remember_source = std::true_type;
#endif
                shared->subscribe_impl(std::move(_source), std::forward<Observer>(observer)
                    , remember_source());
                return Unsubscriber(shared);
            }

            auto fork()
            {
                return SubscribeOnObservable_(_source.fork(), _scheduler);
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::SubscribeOn
        , SourceObservable source, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::SubscribeOnObservable_<SourceObservable, Scheduler>;
        return Observable_<Impl>(Impl(std::move(source), std::move(scheduler)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_range.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
#include <utility>
#include <type_traits>
#include <cstdint>

namespace xrx::detail
{
    template<typename Integer>
    struct RangeObservable
    {
        struct NoUnsubscription
        {
            using has_effect = std::false_type;
            constexpr bool detach() const noexcept { return false; }
        };

        using value_type   = Integer;
        using error_type   = none_tag;
        using Unsubscriber = NoUnsubscription;


        const Integer _first;
        const Integer _last;
        const std::intmax_t _step;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            Integer value = _first;
            while (true)
            {
                const OnNextAction action = ::xrx::detail::on_next_with_action(observer, Integer(value));
                if (action._unsubscribe)
                {
                    break;
                }
                value = Integer(std::intmax_t(value) + _step);
            }
            return Unsubscriber();
        }

        RangeObservable fork()
        {
            return *this;
        }
    };

    template<typename Integer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Range
        , Integer first, Integer last, std::intmax_t step)
    {
        using Impl = RangeObservable<Integer>;
        return Observable_<Impl>(Impl(first, last, step));
    }
} // namespace xrx::detail

// Header: operators/operator_observe_on.h.

// #pragma once
// #include "concepts_observer.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <atomic>

namespace xrx::observable
{
    namespace detail
    {
        template<typename Value, typename Error
            , typename Scheduler
            , typename DestinationObserver_>
        struct ObserveOnObserver_
        {
            // Needs to have shared state so destination Observer
            // can be only move_construcrible.
            // Also, on_next() and on_completed() calls need to target same Observer.
            // #TODO: don't do this if Observer is stateless.
            struct Shared_
            {
                Scheduler _sheduler;
                DestinationObserver_ _target;
                // #TODO: nicer memory orders; sequential is not needed.
                std::atomic_bool _unsubscribed = false;
            };
            std::shared_ptr<Shared_> _shared;
            
            static std::shared_ptr<Shared_> make_state(Scheduler scheduler, DestinationObserver_ target)
            {
                return std::make_shared<Shared_>(std::move(scheduler), std::move(target));
            }

            ::xrx::unsubscribe on_next(Value v)
            {
                if (_shared->_unsubscribed)
                {
                    return ::xrx::unsubscribe(true);
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, v_ = std::forward<Value>(v)]() mutable
                {
                    if (self->_unsubscribed)
                    {
                        return;
                    }
                    const auto action = ::xrx::detail::on_next_with_action(
                        self->_target.on_next(std::forward<Value>(v_)));
                    if (action._unsubscribe)
                    {
                        self->_unsubscribed = true;
                    }
                });
                (void)handle;
            }
            // #TODO: support void Error.
            void on_error(Error e)
            {
                if (_shared->_unsubscribed)
                {
                    return;
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, e_ = std::forward<Error>(e)]() mutable
                {
                    if (not self->_unsubscribed)
                    {
                        self->_target.on_error(std::forward<Error>(e_));
                    }
                });
                (void)handle;
            }
            void on_completed()
            {
                if (_shared->_unsubscribed)
                {
                    return;
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self]()
                {
                    if (not self->_unsubscribed)
                    {
                        self->_target.on_completed();
                    }
                });
                (void)handle;
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct ObserveOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                SourceUnsubscriber _unsubscriber;
                // #TODO: does shared_ptr needed to be there.
                // Looks like weak should work in this case.
                std::shared_ptr<std::atomic_bool> _unsubscribed;

                bool detach()
                {
                    if (_unsubscribed)
                    {
                        *_unsubscribed = true;
                        return _unsubscriber.detach();
                    }
                    return false;
                }
            };

            SourceObservable _source;
            // #TODO: define concept for `StreamScheduler`, see stream_scheduler().
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                using ObserverImpl_ = ObserveOnObserver_<value_type, error_type, Scheduler, Observer>;
                auto shared = ObserverImpl_::make_state(std::move(_scheduler), std::move(observer));
                auto handle = std::move(_source).subscribe(ObserverImpl_(shared));
                Unsubscriber unsubscriber;
                unsubscriber._unsubscribed = std::shared_ptr<std::atomic_bool>(shared, &shared->_unsubscribed);
                unsubscriber._unsubscriber = handle;
                return unsubscriber;
            }

            auto fork() && { return ObserveOnObservable_(std::move(_source), std::move(_scheduler)); }
            auto fork() &  { return ObserveOnObservable_(_source.fork(), _scheduler); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::ObserveOn
        , SourceObservable source, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::ObserveOnObservable_<SourceObservable, Scheduler>;
        // #TODO: add overload that directly accepts `StreamScheduler` so
        // client can pass more narrow interface.
        auto stream = std::move(scheduler).stream_scheduler();
        return Observable_<Impl>(Impl(std::move(source), std::move(stream)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_create.h.

// #pragma once
// #include "concepts_observable.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
#include <utility>
#include <type_traits>

namespace xrx::observable
{
    namespace detail
    {
        // #TODO: allow any unsubscriber, just with `bool detach()`.
        // Add default `using has_effect = std::true_type` implementation in that case.
        // #TODO: in case `ExternalUnsubscriber` is full ConceptUnsubscriber<>
        // just return it; do not create unnecesary wrapper.
        template<typename ExternalUnsubscriber>
            // requires ConceptUnsubscriber<ExternalUnsubscriber>
        struct CreateUnsubscriber : ExternalUnsubscriber
        {
            explicit CreateUnsubscriber(ExternalUnsubscriber subscriber)
                : ExternalUnsubscriber(std::move(subscriber))
            {
            }

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                return CreateUnsubscriber(std::forward<F>(on_subscribe)
                    (std::forward<Observer>(observer)));
            }
        };

        template<>
        struct CreateUnsubscriber<void>
        {
            using has_effect = std::false_type;
            bool detach() { return false; }

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                (void)std::forward<F>(on_subscribe)(std::forward<Observer>(observer));
                return CreateUnsubscriber();
            }
        };

        template<typename Value, typename Error, typename F>
        struct CreateObservable_
        {
            F _on_subscribe;

            using value_type = Value;
            using error_type = Error;
            
            using ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>;
            using CreateReturn = decltype(std::move(_on_subscribe)(ObserverArchetype()));
            static_assert(std::is_same_v<void, CreateReturn> or
                ::xrx::detail::ConceptUnsubscriber<CreateReturn>
                , "Return value of create() callback should be Unsubscriber-like.");

            using Unsubscriber = CreateUnsubscriber<CreateReturn>;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                return Unsubscriber::invoke_(std::move(_on_subscribe), std::move(observer));
            }

            auto fork() && { return CreateObservable_(std::move(_on_subscribe)); }
            auto fork() &  { return CreateObservable_(_on_subscribe); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Value, typename Error, typename F
        , typename ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::Create<Value, Error>
        , F on_subscribe)
            requires requires(ObserverArchetype observer)
        {
            on_subscribe(observer);
        }
    {
        using Impl = ::xrx::observable::detail::CreateObservable_<Value, Error, F>;
        return Observable_<Impl>(Impl(std::move(on_subscribe)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_filter.h.

// #pragma once
// #include "concepts_observer.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
// #include "debug/assert_flag.h"
#include <utility>
#include <concepts>

namespace xrx::detail
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
        struct FilterObserver : Observer
        {
            explicit FilterObserver(Observer&& o, Filter&& f)
                : Observer(std::move(o))
                , _filter(std::move(f))
                , _disconnected() {}
            Filter _filter;
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            OnNextAction on_next(Value&& v)
            {
                _disconnected.check_not_set();
                if (not _filter(v))
                {
                    return OnNextAction();
                }
                return ::xrx::detail::ensure_action_state(
                    ::xrx::detail::on_next_with_action(observer(), std::forward<Value>(v))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using FilterObserver = FilterObserver<Observer_>;
            return std::move(_source).subscribe(FilterObserver(std::forward<Observer>(observer), std::move(_filter)));
        }

        FilterObservable fork()
        {
            return FilterObservable(_source.fork(), _filter);
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , SourceObservable source, F filter)
            requires std::predicate<F, typename SourceObservable::value_type>
    {
        using Impl = FilterObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(std::move(source), std::move(filter)));
    }
} // namespace xrx::detail

// Header: operators/operator_interval.h.

// #pragma once
// #include "meta_utils.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
#include <utility>
#include <chrono>
#include <cstdint>

namespace xrx::observable
{
    namespace detail
    {
        template<typename Duration, typename Scheduler>
        struct IntervalObservable_
        {
            using value_type = std::uint64_t;
            using error_type = ::xrx::detail::none_tag;
            using clock = typename Scheduler::clock;
            using clock_duration = typename Scheduler::clock_duration;
            using clock_point = typename Scheduler::clock_point;
            using Handle = typename Scheduler::ActionHandle;

            struct Unsubscriber
            {
                using has_effect = std::true_type;

                Handle _handle;
                Scheduler _scheduler;

                bool detach()
                {
                    return _scheduler.tick_cancel(_handle);
                }
            };

            Duration _period;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                const clock_duration period(_period);
                // To get zero tick at first. #TODO: revisit this logic.
                const clock_point start_from = (clock::now() - period);
                
                struct State
                {
                    Observer _observer;
                    value_type _ticks = 0;
                };

                const Handle handle = _scheduler.tick_every(start_from, period
                    , [](State& state) mutable
                {
                    const value_type now = state._ticks++;
                    const auto action = ::xrx::detail::on_next_with_action(state._observer, now);
                    return action._unsubscribe;
                }
                    , State(std::move(observer), value_type(0)));

                return Unsubscriber(handle, std::move(_scheduler));
            }

            auto fork()
            {
                return IntervalObservable_(_period, _scheduler);
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Duration, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::Interval
        , Duration period, Scheduler scheduler)
    {
        using Impl = ::xrx::observable::detail::IntervalObservable_<Duration, Scheduler>;
        return Observable_<Impl>(Impl(std::move(period), std::move(scheduler)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_take.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct TakeObservable
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        template<typename Observer>
        struct TakeObserver_ : Observer
        {
            explicit TakeObserver_(Observer&& o, std::size_t count)
                : Observer(std::move(o))
                , _max(count)
                , _taken(0) {}
            std::size_t _max;
            std::size_t _taken;
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            OnNextAction on_next(Value&& v)
            {
                _disconnected.check_not_set();
                if (++_taken > _max)
                {
                    ::xrx::detail::on_completed(observer());
                    _disconnected.raise();
                    return OnNextAction{._unsubscribe = true};
                }
                return xrx::detail::ensure_action_state(
                    ::xrx::detail::on_next_with_action(observer(), std::forward<Value>(v))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer observer) &&
        {
            using TakeObserver_ = TakeObserver_<Observer>;
            return std::move(_source).subscribe(TakeObserver_(std::move(observer), _count));
        }

        TakeObservable fork() && { return TakeObservable(std::move(_source), _count); };
        TakeObservable fork() &  { return TakeObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Take
        , SourceObservable source, std::size_t count)
    {
        using Impl = TakeObservable<SourceObservable>;
        return Observable_<Impl>(Impl(std::move(source), count));
    }
} // namespace xrx::detail

// Header: operators/operator_transform.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
// #include "debug/assert_flag.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, typename Transform>
    struct TransformObservable
    {
        SourceObservable _source;
        Transform _transform;

        using value_type   = decltype(_transform(std::declval<typename SourceObservable::value_type>()));
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;


        template<typename Observer>
        struct TransformObserver : Observer
        {
            explicit TransformObserver(Observer&& o, Transform&& f)
                : Observer(std::move(o))
                , _transform(std::move(f))
                , _disconnected() {}
            Transform _transform;
            [[no_unique_address]] debug::AssertFlag<> _disconnected;
            Observer& observer() { return *this; }

            template<typename Value>
            auto on_next(Value&& v)
            {
                _disconnected.check_not_set();
                return xrx::detail::ensure_action_state(
                    xrx::detail::on_next_with_action(observer()
                        , _transform(std::forward<Value>(v)))
                    , _disconnected);
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using TransformObserver = TransformObserver<Observer_>;
            return std::move(_source).subscribe(TransformObserver(
                std::forward<Observer>(observer), std::move(_transform)));
        }

        TransformObservable fork()
        {
            return TransformObservable(_source.fork(), _transform);
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , SourceObservable source, F transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        using Impl = TransformObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(std::move(source), std::move(transform)));
    }
} // namespace xrx::detail

// Header: subject.h.

// #pragma once
// #include "any_observer.h"
// #include "utils_containers.h"
// #include "observable_interface.h"
// #include "debug/assert_mutex.h"

#include <memory>
#include <cassert>

namespace xrx
{
    template<typename Value, typename Error>
    struct Subject_
    {
        using Subscriptions = detail::HandleVector<AnyObserver<Value, Error>>;
        using Handle = typename Subscriptions::Handle;

        struct SharedImpl_
        {
            [[no_unique_address]] debug::AssertMutex<> _assert_mutex;
            Subscriptions _subscriptions;
        };

        using value_type = Value;
        using error_type = Error;

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            std::weak_ptr<SharedImpl_> _shared_weak;
            Handle _handle;

            bool detach()
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
                    auto _ = std::lock_guard(shared->_assert_mutex);
                    return shared->_subscriptions.erase(_handle);
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
            using value_type   = Subject_::value_type;
            using error_type   = Subject_::error_type;
            using Unsubscriber = Subject_::Unsubscriber;

            std::weak_ptr<SharedImpl_> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            Unsubscriber subscribe(Observer&& observer)
            {
                if (auto shared = _shared_weak.lock(); shared)
                {
                    AnyObserver<value_type, error_type> erased(std::forward<Observer>(observer));
                    auto _ = std::lock_guard(shared->_assert_mutex);
                    Unsubscriber unsubscriber;
                    unsubscriber._shared_weak = _shared_weak;
                    unsubscriber._handle = shared->_subscriptions.push_back(std::move(erased));
                    return unsubscriber;
                }
                return Unsubscriber();
            }

            auto fork() { return SourceObservable(_shared_weak); }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        Unsubscriber subscribe(Observer&& observer) const
        {
            return as_observable().subscribe(std::forward<Observer>(observer));
        }

        detail::Observable_<SourceObservable> as_observable() const
        {
            return Observable_<SourceObservable>(SourceObservable(_shared));
        }

        Subject_ as_observer() const
        {
            return Subject_(_shared);
        }

        void on_next(Value v)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer, Handle handle)
                {
                    bool do_unsubscribe = false;
                    {
                        auto _ = debug::ScopeUnlock(lock);
                        const auto action = observer.on_next(v);
                        do_unsubscribe = action._unsubscribe;
                    }
                    if (do_unsubscribe)
                    {
                        // Notice: we have mutex lock.
                        _shared->_subscriptions.erase(handle);
                    }
                });
            }
            // else: already completed
        }

        void on_error(Error e)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                    auto _ = debug::ScopeUnlock(lock);
                    observer.on_error(e);
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }

        void on_completed()
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                        auto _ = debug::ScopeUnlock(lock);
                        observer.on_completed();
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }
    };
} // namespace xrx

// Header: operators/operator_publish.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "observable_interface.h"
// #include "utils_containers.h"
// #include "debug/assert_mutex.h"
// #include "any_observer.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <optional>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct ConnectObservableState_
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using AnyObserver_ = AnyObserver<value_type, error_type>;
        using Subscriptions = detail::HandleVector<AnyObserver_>;
        using Handle = typename Subscriptions::Handle;
        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

        struct SharedImpl_;

        struct Observer_
        {
            std::shared_ptr<SharedImpl_> _shared;
            bool _do_refcount = false;

            void on_next(value_type v);
            void on_error(error_type e);
            void on_completed();
        };

        struct Unsubscriber
        {
            using has_effect = std::true_type;

            // #XXX: note strong reference, not weak one.
            // #TODO: add a test that makes sure we can't use weak there.
            std::shared_ptr<SharedImpl_> _shared;
            Handle _handle;

            bool detach(bool do_refcount = false);
        };

        struct RefCountUnsubscriber
        {
            Unsubscriber _unsubscriber;
            explicit RefCountUnsubscriber(Unsubscriber unsubscriber)
                : _unsubscriber(std::move(unsubscriber))
            {
            }

            bool detach()
            {
                const bool do_refcount = true;
                return _unsubscriber.detach(do_refcount);
            }
        };

        struct RefCountObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using Unsubscriber = typename ConnectObservableState_::Unsubscriber;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
            auto subscribe(Observer&& observer) &&;

            auto fork()
            {
                return RefCountObservable_(_shared);
            }
        };

        struct SharedImpl_ : public std::enable_shared_from_this<SharedImpl_>
        {
            using Base = std::enable_shared_from_this<SharedImpl_>;

            [[no_unique_address]] debug::AssertMutex<> _assert_mutex;
            SourceObservable _source;
            Subscriptions _subscriptions;
            std::optional<SourceUnsubscriber> _connected;

            explicit SharedImpl_(SourceObservable source)
                : Base()
                , _source(std::move(source))
                , _subscriptions()
                , _connected(std::nullopt)
            {
            }

            SourceUnsubscriber connect(bool do_refcount)
            {
                auto _ = std::lock_guard(_assert_mutex);
                if (not _connected)
                {
                    // #XXX: circular dependency ?
                    // `_source` that remembers Observer that takes strong reference to this.
                    auto unsubscriber = _source.fork().subscribe(
                        Observer_(this->shared_from_this(), do_refcount));
                    _connected = unsubscriber;
                    return unsubscriber;
                }
                return SourceUnsubscriber();
            }

            template<typename Observer>
            Unsubscriber subscribe(Observer&& observer, bool do_refcount = false)
            {
                std::size_t count_before = 0;
                AnyObserver_ erased_observer(std::forward<Observer>(observer));
                Unsubscriber unsubscriber;
                unsubscriber._shared = Base::shared_from_this();
                {
                    auto _ = std::lock_guard(_assert_mutex);
                    count_before = _subscriptions.size();
                    unsubscriber._handle = _subscriptions.push_back(std::move(erased_observer));
                }
                if (do_refcount && (count_before == 0))
                {
                    connect(do_refcount);
                }
                return unsubscriber;
            }

            bool unsubscribe(Handle handle, bool do_refcount)
            {
                auto _ = std::lock_guard(_assert_mutex);
                const std::size_t count_before = _subscriptions.size();
                const bool ok = _subscriptions.erase(handle);
                const std::size_t count_now = _subscriptions.size();
                if (do_refcount && (count_now == 0) && (count_before > 0))
                {
                    if (_connected)
                    {
                        _connected->detach();
                        _connected.reset();
                    }
                }
                return ok;
            }

            void on_next_impl(value_type v, bool do_refcount)
            {
                auto lock = std::unique_lock(_assert_mutex);
                _subscriptions.for_each([&](AnyObserver_& observer, Handle handle)
                {
                    auto _ = debug::ScopeUnlock(lock);
                    const auto action = observer.on_next(v);
                    if (action._unsubscribe)
                    {
                        unsubscribe(handle, do_refcount);
                    }
                });
                // #XXX: should we return there for caller's on_next() ?
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit ConnectObservableState_(SourceObservable source)
            : _shared(std::make_shared<SharedImpl_>(std::move(source)))
        {
        }
    };

    template<typename SourceObservable>
    struct ConnectObservableSource_
    {
        using State_ = ConnectObservableState_<SourceObservable>;
        using SharedImpl_ = typename State_::SharedImpl_;
        using Unsubscriber = typename State_::Unsubscriber;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;

        std::weak_ptr<SharedImpl_> _shared_weak;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer)
        {
            if (auto shared = _shared_weak.lock(); shared)
            {
                return shared->subscribe(std::forward<Observer>(observer));
            }
            return Unsubscriber();
        }

        ConnectObservableSource_ fork() { return ConnectObservableSource_(_shared_weak); }
    };

    template<typename SourceObservable
        , typename SharedState_ = ConnectObservableState_<SourceObservable>
        , typename Source_      = ConnectObservableSource_<SourceObservable>>
    struct ConnectObservable_
        : private SharedState_
        , public  Observable_<Source_>
    {
        using ObservableImpl_ = Observable_<Source_>;
        using value_type = typename ObservableImpl_::value_type;
        using error_type = typename ObservableImpl_::error_type;
        using SharedImpl_ = typename SharedState_::SharedImpl_;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SharedState_& state() { return static_cast<SharedState_&>(*this); }

        explicit ConnectObservable_(SourceObservable source)
            : SharedState_(std::move(source))
            , ObservableImpl_(Source_(state()._shared))
        {
        }

        Unsubscriber connect()
        {
            const bool do_refcount = false;
            return state()._shared->connect(do_refcount);
        }

        auto ref_count()
        {
            using RefCountObservable_ = typename SharedState_::RefCountObservable_;
            return Observable_<RefCountObservable_>(RefCountObservable_(state()._shared));
        }
    };

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_next(value_type v)
    {
        _shared->on_next_impl(std::forward<value_type>(v), _do_refcount);
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_error(error_type e)
    {
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto _ = debug::ScopeUnlock(lock);
            observer.on_error(e);
        });
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_completed()
    {
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto _ = debug::ScopeUnlock(lock);
            observer.on_completed();
        });
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Unsubscriber::detach(bool do_refcount /*= false*/)
    {
        return _shared->unsubscribe(_handle, do_refcount);
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_
        ::subscribe(Observer&& observer) &&
    {
        const bool do_refcount = true;
        return RefCountUnsubscriber(_shared->subscribe(
            std::forward<Observer>(observer), do_refcount));
    }

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Publish
        , SourceObservable source)
    {
        return ConnectObservable_<SourceObservable>(std::move(source));
    }
} // namespace xrx::detail
