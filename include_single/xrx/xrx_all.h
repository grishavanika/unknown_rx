#pragma once
// 
// Auto-generated single-header file:
// powershell -File tools/generate_single_header.ps1
// 

// Header: utils_observable.h.

// #pragma once
#include <type_traits>

namespace xrx::detail
{
    struct NoopUnsubscriber
    {
        using has_effect = std::false_type;
        constexpr bool detach() const noexcept { return false; }
    };
} // namespace xrx::detail

// Header: utils_fast_fwd.h.

// #pragma once
#include <utility>

// #XXX: this needs to be without include-guard
// and in something like XRX_prelude.h header
// so it can be undefined in XRX_conclusion.h.
// #XXX: do with static_cast<>. Get read of <utility> include.
// https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html
// https://github.com/SuperV1234/vrm_core/issues/1
#define XRX_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define XRX_MOV(...) ::std::move(__VA_ARGS__)

// Just an annotation that T&& should be rvalue reference
// (because of constraints from other places).
// XRX_MOV() can be used.
#define XRX_RVALUE(...) __VA_ARGS__

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


// Header: utils_defines.h.

// #pragma once

#if defined(_MSC_VER)
  #define XRX_FORCEINLINE() __forceinline
  #define XRX_FORCEINLINE_LAMBDA()
#else
  // #XXX: temporary disabled, need to see why GCC says:
  // warning: 'always_inline' function might not be inlinable [-Wattributes]
  // on simple functions. Something done in a wrong way.
  // 
  // #define XRX_FORCEINLINE() __attribute__((always_inline))
  // #define XRX_FORCEINLINE_LAMBDA() __attribute__((always_inline))
  #define XRX_FORCEINLINE() 
  #define XRX_FORCEINLINE_LAMBDA() 
#endif

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

    struct From
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .from() operator implementation. "
                  "Missing `operators/operator_from.h` include ?");
        };
    };

    struct Repeat
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .repeat() operator implementation. "
                  "Missing `operators/operator_repeat.h` include ?");
        };
    };

    struct Concat
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .concat() operator implementation. "
                  "Missing `operators/operator_concat.h` include ?");
        };
    };

    struct FlatMap
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .flat_map() operator implementation. "
                  "Missing `operators/operator_flat_map.h` include ?");
        };
    };

    struct Window
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .window() operator implementation. "
                  "Missing `operators/operator_window.h` include ?");
        };
    };

    struct Reduce
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .reduce() operator implementation. "
                  "Missing `operators/operator_reduce.h` include ?");
        };
    };

} // namespace xrx::detail::operator_tag

// Header: utils_wrappers.h.

// #pragma once
// #include "utils_defines.h"
#include <type_traits>

namespace xrx::detail
{
    // Hint that V is reference and may be moved from
    // to MaybeRef<>.
#define XRX_MOV_IF_ASYNC(V) V

    template<typename T, bool MustBeValue>
    struct MaybeRef;

    template<typename T>
    struct MaybeRef<T, true/*value*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        [[no_unique_address]] T _value;
        XRX_FORCEINLINE() T& get() { return _value; }

        XRX_FORCEINLINE() MaybeRef(T& v)
            : _value(XRX_MOV(v))
        {
        }
    };

    template<typename T>
    struct MaybeRef<T, false/*reference*/>
    {
        static_assert(not std::is_reference_v<T>);
        static_assert(not std::is_const_v<T>);

        T* _value = nullptr;
        XRX_FORCEINLINE() T& get() { return *_value; }

        XRX_FORCEINLINE() MaybeRef(T& v)
            : _value(&v)
        {
        }
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

// Header: concepts_observable.h.

// #pragma once
// #include "meta_utils.h"
#include <type_traits>
#include <concepts>

namespace xrx::detail
{
    template<typename Observable_>
        requires requires { typename Observable_::is_async; }
    constexpr auto detect_async() { return typename Observable_::is_async(); }
    template<typename Observable_>
        requires (not requires { typename Observable_::is_async; })
    constexpr auto detect_async() { return std::true_type(); }

    template<typename Observable_>
    using IsAsyncObservable = decltype(detect_async<Observable_>());

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
        // #TODO: value_type and error_type should not be references.
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

// Header: debug/assert_mutex.h.

// #pragma once
// #include "utils_fast_FWD.h"
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
            , _mutex()
        {
        }

        explicit AssertMutex(Action action) noexcept
            : _action(XRX_MOV(action))
            , _mutex()
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


// Header: utils_containers.h.

// #pragma once
#include <type_traits>
#include <vector>
#include <limits>
#include <algorithm>

#include <cstdint>
#include <cassert>
#include <cstring>

// #include "utils_fast_FWD.h"
// #include "utils_defines.h"

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

    template<typename T, std::size_t Size = 1>
    struct SmallVector
    {
        using Element = std::aligned_storage_t<sizeof(T), alignof(T)>;
        Element _static[Size];
        T* _dynamic;
        std::size_t _size;
        std::size_t _capacity;

        XRX_FORCEINLINE() std::size_t size() const { return _size; }
        XRX_FORCEINLINE() std::size_t dynamic_capacity() const { return _capacity; }

        XRX_FORCEINLINE() explicit SmallVector(std::size_t max_capacity = 0) noexcept // #XXX: lie.
            // : _static()
            : _dynamic(nullptr)
            , _size(0)
            , _capacity(0)
        {
            if (max_capacity > Size)
            {
                _capacity = (max_capacity - Size);
                _dynamic = new T[_capacity];
            }
        }

        XRX_FORCEINLINE() ~SmallVector() noexcept
        {
            if constexpr (not std::is_trivial_v<T>)
            {
                for (std::size_t i = 0, count = (std::min)(Size, _size); i < count; ++i)
                {
                    void* memory = &_static[i];
                    static_cast<T*>(memory)->~T();
                }
            }
            if (_dynamic)
            {
                delete[] _dynamic;
            }
        }

        XRX_FORCEINLINE() SmallVector(const SmallVector& rhs) noexcept
            // : _static()
            : _dynamic(nullptr)
            , _size(rhs._size)
            , _capacity(rhs._capacity)
        {
            if constexpr (std::is_trivial_v<T>)
            {
                std::memcpy(&_static, rhs._static, (std::min)(Size, _size) * sizeof(T));
            }
            else
            {
                for (std::size_t i = 0, count = (std::min)(Size, _size); i < count; ++i)
                {
                    void* memory = &_static[i];
                    (void)new(memory) T(rhs._static[i]);
                }
            }
            if (rhs._capacity > 0)
            {
                _dynamic = new T[_capacity];
                if (rhs._size > Size)
                {
                    assert(rhs._dynamic);
                    std::copy(rhs._dynamic, rhs._dynamic + rhs._size - Size, _dynamic);
                }
            }
        }

        XRX_FORCEINLINE() SmallVector(SmallVector&& rhs) noexcept
            // : _static()
            : _dynamic(nullptr)
            , _size(rhs._size)
            , _capacity(rhs._capacity)
        {
            if constexpr (std::is_trivial_v<T>)
            {
                std::memcpy(&_static, rhs._static, (std::min)(Size, _size) * sizeof(T));
            }
            else
            {
                for (std::size_t i = 0, count = (std::min)(Size, _size); i < count; ++i)
                {
                    void* memory = &_static[i];
                    (void)new(memory) T(rhs._static[i]);
                }
            }
            _dynamic = rhs._dynamic;
            rhs._dynamic = nullptr;
            if constexpr (not std::is_trivial_v<T>)
            {
                for (std::size_t i = 0, count = (std::min)(Size, rhs._size); i < count; ++i)
                {
                    void* memory = &rhs._static[i];
                    static_cast<T*>(memory)->~T();
                }
            }
            rhs._size = 0;
            rhs._capacity = 0;
        }

        SmallVector& operator=(const SmallVector& rhs) = delete;

        XRX_FORCEINLINE() SmallVector& operator=(SmallVector&& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }
            this->~SmallVector();
            new(static_cast<void*>(this)) SmallVector(XRX_MOV(rhs));
            return *this;
        }

        template<typename U>
        XRX_FORCEINLINE() void push_back(U&& v)
        {
            if (_size < Size)
            {
                void* memory = &_static[_size];
                (void)new(memory) T(XRX_FWD(v));
                ++_size;
            }
            else if (_size < (_capacity + Size))
            {
                assert(_dynamic);
                _dynamic[_size - Size] = XRX_FWD(v);
                ++_size;
            }
            else
            {
                assert(false && "SmallVector<> overflow.");
            }
        }

        template<typename F>
        XRX_FORCEINLINE() bool for_each(F&& f)
        {
            for (std::size_t i = 0, count = (std::min)(Size, _size); i < count; ++i)
            {
                void* memory = &_static[i];
                if (not f(*static_cast<T*>(memory)))
                {
                    return false;
                }
            }
            if (_size >= Size)
            {
                for (std::size_t i = 0, count = (_size - Size); i < count; ++i)
                {
                    if (not f(_dynamic[i]))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    };
} // namespace xrx::detail

// Header: concepts_observer.h.

// #pragma once
// #include "tag_invoke_with_extension.h"
// #include "meta_utils.h"
// #include "utils_defines.h"

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
        // on_error() with single argument.
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
        // on_error() with no argument.
        template<typename S>
        XRX_FORCEINLINE() constexpr decltype(auto) resolve0_(S&& s, priority_tag<1>) const
            noexcept(noexcept(tag_invoke(*this, std::forward<S>(s))))
                requires tag_invocable<on_error_fn, S>
        {
            return tag_invoke(*this, std::forward<S>(s));
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr auto resolve0_(S&& s, priority_tag<0>) const
            noexcept(noexcept(std::forward<S>(s).on_error()))
                -> decltype(std::forward<S>(s).on_error())
        {
            return std::forward<S>(s).on_error();
        }
        template<typename S>
        XRX_FORCEINLINE() constexpr auto operator()(S&& s) const
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

// Header: simple_scheduler_event_loop.h.

// #pragma once
// #include "utils_containers.h"
// #include "utils_fast_FWD.h"
// #include "debug/assert_mutex.h"

#include <functional>
#include <memory>
#include <chrono>
#include <mutex>
#include <thread>
#include <concepts>
#include <cassert>

namespace xrx::debug
{
    // Some debug code to play around `Scheduler` concept.
    // Not a final interface/API.
    // 
    struct EventLoop
    {
        using clock = std::chrono::steady_clock;
        using clock_duration = clock::duration;
        using clock_point = clock::time_point;

        struct IActionCallback
        {
            clock_point _start_from;
            clock_point _last_tick;
            clock_duration _period;

            virtual ~IActionCallback() = default;
            virtual bool invoke() = 0;
        };

        template<typename F, typename State>
        struct ActionCallback_ : IActionCallback
        {
            F _f;
            State _state;

            explicit ActionCallback_(XRX_RVALUE(F&&) f, XRX_RVALUE(State&&) state)
                : _f(XRX_MOV(f))
                , _state(XRX_MOV(state))
            {
            }

            virtual bool invoke() override
            {
                return _f(_state);
            }
        };
        // Need to be able to copy when executing - to release a lock, hence shared_ptr<>.
        // Can be avoided if request to remove/cancel is delayed to next frame ?
        // This way we can use std::function<> only, probably.
        using TickAction = std::shared_ptr<IActionCallback>;

        template<typename F, typename State>
        static std::unique_ptr<IActionCallback> make_action(
            clock_point start_from, clock_duration period
            , XRX_RVALUE(F&&) f, XRX_RVALUE(State&&) state)
        {
            static_assert(not std::is_lvalue_reference_v<F>);
            static_assert(not std::is_lvalue_reference_v<State>);
            using Action_ = ActionCallback_<std::remove_reference_t<F>, std::remove_reference_t<State>>;
            auto action = std::make_unique<Action_>(XRX_MOV(f), XRX_MOV(state));
            action->_start_from = start_from;
            action->_last_tick = {};
            action->_period = period;
            return action;
        }

        using TickActions = ::xrx::detail::HandleVector<TickAction>;
        using ActionHandle = typename TickActions::Handle;
        TickActions _tick_actions;
        mutable AssertMutex<> _assert_mutex_tick;

        using TaskCallback = std::function<void ()>;
        using Tasks = ::xrx::detail::HandleVector<TaskCallback>;
        using TaskHandle = typename Tasks::Handle;
        Tasks _tasks;
        mutable AssertMutex<> _assert_mutex_tasks;

        template<typename F, typename State>
            requires std::convertible_to<std::invoke_result_t<F, State&>, bool>
        ActionHandle tick_every(clock_point start_from, clock_duration period, F&& f, State&& state)
        {
            auto guard = std::lock_guard(_assert_mutex_tick);
            const ActionHandle handle = _tick_actions.push_back(make_action(
                start_from
                , clock_duration(period)
                , XRX_FWD(f)
                , XRX_FWD(state)));
            return handle;
        }

        bool tick_cancel(ActionHandle handle)
        {
            auto guard = std::lock_guard(_assert_mutex_tick);
            return _tick_actions.erase(handle);
        }

        template<typename F>
        TaskHandle task_post(F&& task)
        {
            const TaskHandle handle = _tasks.push_back(TaskCallback(XRX_FWD(task)));
            return handle;
        }

        bool task_cancel(TaskHandle handle)
        {
            return _tasks.erase(handle);
        }

        std::size_t poll_one_task()
        {
            TaskCallback task;
            {
                auto guard = std::lock_guard(_assert_mutex_tasks);
                if (_tasks.size() == 0)
                {
                    return 0;
                }
                for (std::size_t i = 0; i < _tasks._values.size(); ++i)
                {
                    const TaskHandle handle(_tasks._values[i]._version, std::uint32_t(i));
                    if (handle)
                    {
                        TaskCallback* callback = _tasks.get(handle);
                        assert(callback);
                        task = XRX_MOV(*callback);
                        _tasks.erase(handle);
                        break; // find & execute first task.
                    }
                }
            }
            if (task)
            {
                XRX_MOV(task)();
                return 1;
            }
            return 0;
        }

        std::size_t poll_one()
        {
            ActionHandle to_execute;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                clock_duration smallest = clock_duration::max();
                _tick_actions.for_each([&](TickAction& action, ActionHandle handle)
                {
                    const clock_duration point = (action->_last_tick + action->_period).time_since_epoch();
                    if (point < smallest)
                    {
                        smallest = point;
                        to_execute = handle;
                    }
                });
            }
            if (not to_execute)
            {
                return poll_one_task();
            }

            clock_point desired_point;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                TickAction* action = _tick_actions.get(to_execute);
                if (not action)
                {
                    // Someone just removed it.
                    return 0;
                }
                desired_point = ((*action)->_last_tick + (*action)->_period);
            }
        
            // #TODO: resume this one when new item arrives
            // (with probably closer-to-execute time point).
            std::this_thread::sleep_until(desired_point); // no-op if time in the past.

            TickAction invoke_action;
            bool do_remove = false;
            {
                auto guard = std::lock_guard(_assert_mutex_tick);
                if (TickAction* action = _tick_actions.get(to_execute))
                {
                    invoke_action = *action; // copy shared.
                }
            }
            if (invoke_action)
            {
                // Notice: we don't have lock.
                do_remove = invoke_action->invoke();
            }
            if (do_remove)
            {
                const bool removed = tick_cancel(to_execute);
                assert(removed); (void)removed;
                return 1;
            }
            auto guard = std::lock_guard(_assert_mutex_tick);
            // Action can removed from the inside of callback.
            if (TickAction* action_modified = _tick_actions.get(to_execute))
            {
                (*action_modified)->_last_tick = clock::now();
            }
            return 1;
        }

        std::size_t work_count() const
        {
            // std::scoped_lock<> requires .try_lock() ?
            auto ticks_lock = std::lock_guard(_assert_mutex_tick);
            auto tasks_lock = std::lock_guard(_assert_mutex_tasks);
            return (_tick_actions.size() + _tasks.size());
        }

        struct Scheduler
        {
            using clock = EventLoop::clock;
            using clock_duration = EventLoop::clock_duration;
            using clock_point = EventLoop::clock_point;
            using ActionHandle = EventLoop::ActionHandle;
            using TaskHandle = EventLoop::TaskHandle;

            EventLoop* _event_loop = nullptr;

            template<typename F, typename State>
            ActionHandle tick_every(clock_point start_from, clock_duration period, F&& f, State&& state)
            {
                assert(_event_loop);
                return _event_loop->tick_every(
                    XRX_MOV(start_from)
                    , XRX_MOV(period)
                    , XRX_FWD(f)
                    , XRX_FWD(state));
            }

            bool tick_cancel(ActionHandle handle)
            {
                assert(_event_loop);
                return _event_loop->tick_cancel(handle);
            }

            template<typename F>
            TaskHandle task_post(F&& task)
            {
                assert(_event_loop);
                return _event_loop->task_post(XRX_FWD(task));
            }

            bool task_cancel(TaskHandle handle)
            {
                assert(_event_loop);
                return _event_loop->task_cancel(handle);
            }

            // Scheduler for which every task is ordered.
            // #TODO: looks like too big restriction.
            // on_next() can go in whatever order, but on_complete and on_error
            // should be last one.
            Scheduler stream_scheduler() { return *this; }
        };

        Scheduler scheduler()
        {
            return Scheduler{this};
        }
    };
} // namespace xrx::debug



// Header: observables_factory.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_fast_FWD.h"
#include <utility>

namespace xrx::observable
{
    template<typename Duration, typename Scheduler>
    auto interval(Duration period, XRX_RVALUE(Scheduler&&) scheduler)
    {
        static_assert(not std::is_lvalue_reference_v<Scheduler>);
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Interval()
            , XRX_MOV(period), XRX_MOV(scheduler));
    }

    template<typename Value, typename Error = void, typename F>
    auto create(XRX_RVALUE(F&&) on_subscribe)
    {
        static_assert(not std::is_same_v<Value, void>);
        static_assert(not std::is_lvalue_reference_v<F>);
        using Tag_ = xrx::detail::operator_tag::Create<Value, Error>;
        return ::xrx::detail::make_operator(Tag_()
            , XRX_MOV(on_subscribe));
    }

    template<typename Integer>
    auto range(Integer first)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, first/*last*/, 1/*step*/, std::true_type()/*endless*/);
    }
    template<typename Integer>
    auto range(Integer first, Integer last)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, last/*last*/, 1/*step*/, std::false_type()/*endless*/);
    }
    template<typename Integer>
    auto range(Integer first, Integer last, std::intmax_t step)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Range()
            , first, last/*last*/, step/*step*/, std::false_type()/*endless*/);
    }

    template<typename V, typename... Vs>
    auto from(V&& v0, Vs&&... vs)
    {
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::From()
            , XRX_FWD(v0), XRX_FWD(vs)...);
    }

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto concat(XRX_RVALUE(Observable1&&) observable1, XRX_RVALUE(Observable2&&) observable2, XRX_RVALUE(ObservablesRest&&)... observables)
    {
        static_assert(not std::is_lvalue_reference_v<Observable1>);
        static_assert(not std::is_lvalue_reference_v<Observable2>);
        static_assert(((not std::is_lvalue_reference_v<ObservablesRest>) && ...));
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Concat()
            , XRX_MOV(observable1), XRX_MOV(observable2), XRX_MOV(observables)...);
    }
} // namespace xrx::observable

// Header: utils_observers.h.

// #pragma once
// #include "concepts_observer.h"
// #include "utils_fast_FWD.h"
// #include "utils_defines.h"

#include <type_traits>
#include <utility>

namespace xrx::detail
{
    struct [[nodiscard]] OnNextAction
    {
        bool _stop = false;
    };

    // #pragma GCC diagnostic ignored "-Wattributes"
    // #XXX: check why GCC can't possibly inline one-liner.
    // 
    template<typename Observer, typename Value>
    XRX_FORCEINLINE() OnNextAction on_next_with_action(Observer&& observer, Value&& value)
        requires ConceptWithOnNext<Observer, Value>
    {
        using Return_ = decltype(::xrx::detail::on_next(XRX_FWD(observer), XRX_FWD(value)));
        static_assert(not std::is_reference_v<Return_>
            , "Return by value only allowed fron on_next() callback. To simplify implementation below.");
        if constexpr (std::is_same_v<Return_, void>)
        {
            (void)::xrx::detail::on_next(XRX_FWD(observer), XRX_FWD(value));
            return OnNextAction();
        }
        else if constexpr (std::is_same_v<Return_, OnNextAction>)
        {
            return ::xrx::detail::on_next(XRX_FWD(observer), XRX_FWD(value));
        }
        else if constexpr (std::is_same_v<Return_, ::xrx::unsubscribe>)
        {
            const ::xrx::unsubscribe state = ::xrx::detail::on_next(XRX_FWD(observer), XRX_FWD(value));
            return OnNextAction{._stop = state._do_unsubscribe};
        }
        else
        {
            static_assert(AlwaysFalse<Observer>()
                , "Unknown return type from ::on_next(Value). New tag to handle ?");
        }
    }

    // #TODO: merge those 2 functions.
    template<typename Observer, typename Value>
    XRX_FORCEINLINE() OnNextAction on_next_with_action(Observer&& observer)
        requires ConceptWithOnNext<Observer, void>
    {
        using Return_ = decltype(::xrx::detail::on_next(XRX_FWD(observer)));
        static_assert(not std::is_reference_v<Return_>
            , "Return by value only allowed fron on_next() callback. To simplify implementation below.");
        if constexpr (std::is_same_v<Return_, void>)
        {
            (void)::xrx::detail::on_next(XRX_FWD(observer));
            return OnNextAction();
        }
        else if constexpr (std::is_same_v<Return_, OnNextAction>)
        {
            return ::xrx::detail::on_next(XRX_FWD(observer));
        }
        else if constexpr (std::is_same_v<Return_, ::xrx::unsubscribe>)
        {
            const ::xrx::unsubscribe state = ::xrx::detail::on_next(XRX_FWD(observer));
            return OnNextAction{ ._stop = state._do_unsubscribe };
        }
        else
        {
            static_assert(AlwaysFalse<Observer>()
                , "Unknown return type from ::on_next(). New tag to handle ?");
        }
    }

    template<typename Observer, typename Error>
    XRX_FORCEINLINE() void on_error_optional(Observer&& observer, Error&& error)
        requires ConceptWithOnError<Observer, Error>
    {
        return ::xrx::detail::on_error(XRX_FWD(observer), XRX_FWD(error));
    }
    template<typename Observer, typename Error>
    XRX_FORCEINLINE() void on_error_optional(Observer&&, Error&&)
        requires (not ConceptWithOnError<Observer, Error>)
    {
    }

    template<typename Observer>
    XRX_FORCEINLINE() void on_error_optional(Observer&& observer)
        requires ConceptWithOnError<Observer, void>
    {
        return ::xrx::detail::on_error(XRX_FWD(observer));
    }
    template<typename Observer>
    XRX_FORCEINLINE() void on_error_optional(Observer&&)
        requires (not ConceptWithOnError<Observer, void>)
    {
    }

    template<typename Observer>
    XRX_FORCEINLINE() auto on_completed_optional(Observer&& o)
        requires ConceptWithOnCompleted<Observer>
    {
        return ::xrx::detail::on_completed(XRX_FWD(o));
    }
    
    template<typename Observer>
    XRX_FORCEINLINE() auto on_completed_optional(Observer&&)
        requires (not ConceptWithOnCompleted<Observer>)
    {
    }

    struct OnNext_Noop
    {
        template<typename Value>
        XRX_FORCEINLINE() constexpr void on_next(Value&&) const noexcept
        {
        }
        XRX_FORCEINLINE() constexpr void on_next() const noexcept
        {
        }
    };

    struct OnCompleted_Noop
    {
        XRX_FORCEINLINE() constexpr void on_completed() const noexcept
        {
        }
    };

    struct OnError_Noop
    {
        template<typename Error>
        XRX_FORCEINLINE() constexpr void on_error(Error&&) const noexcept
        {
        }
        XRX_FORCEINLINE() constexpr void on_error() const noexcept
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
        XRX_FORCEINLINE() constexpr decltype(auto) on_next(Value&& value)
            requires ConceptWithOnNext<OnNext, Value>
        {
            return ::xrx::detail::on_next(_on_next, XRX_FWD(value));
        }

        XRX_FORCEINLINE() constexpr decltype(auto) on_next()
            requires ConceptWithOnNext<OnNext, void>
        {
            return ::xrx::detail::on_next(_on_next);
        }

        XRX_FORCEINLINE() constexpr auto on_completed()
            requires requires { XRX_MOV(_on_completed)(); }
        {
            return XRX_MOV(_on_completed)();
        }

        template<typename Error>
        XRX_FORCEINLINE() constexpr auto on_error(Error&& error)
            requires requires { XRX_MOV(_on_error)(XRX_FWD(error)); }
        {
            return XRX_MOV(_on_error)(XRX_FWD(error));
        }

        XRX_FORCEINLINE() constexpr decltype(auto) on_error()
            requires requires { XRX_MOV(_on_error)(); }
        {
            return XRX_MOV(_on_error)();
        }
    };

    template<typename Observer>
    struct ObserverRef
    {
        Observer* _ref = nullptr;

        template<typename Value>
        XRX_FORCEINLINE() auto on_next(Value&& v) { return _ref->on_next(XRX_FWD(v)); }
        
        template<typename Error>
            requires ConceptWithOnError<Observer, Error>
        XRX_FORCEINLINE() auto on_error(Error&& e)
        { return _ref->on_error(XRX_FWD(e)); }
        
        XRX_FORCEINLINE() auto on_error()
            requires ConceptWithOnError<Observer, void>
        { return _ref->on_error(); }
        
        auto on_completed() { return _ref->on_completed(); }
    };
} // namespace xrx::detail

namespace xrx::observer
{
    template<typename Observer_>
    XRX_FORCEINLINE() auto ref(Observer_& observer)
    {
        return ::xrx::detail::ObserverRef<Observer_>(&observer);
    }

    template<typename OnNext>
    XRX_FORCEINLINE() auto make(OnNext&& on_next)
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = ::xrx::detail::LambdaObserver<F_, ::xrx::detail::OnCompleted_Noop, ::xrx::detail::OnError_Noop>;
        return Observer(XRX_FWD(on_next), {}, {});
    }

    template<typename Value, typename OnNext>
    XRX_FORCEINLINE()
    ConceptValueObserverOf<Value> auto
        make_of(OnNext&& on_next)
            requires ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_next>, OnNext, Value>
    {
        using F_       = std::remove_cvref_t<OnNext>;
        using Observer = ::xrx::detail::LambdaObserver<F_, ::xrx::detail::OnCompleted_Noop, ::xrx::detail::OnError_Noop>;
        return Observer(XRX_FWD(on_next), {}, {});
    }

    template<typename OnNext, typename OnCompleted, typename OnError = ::xrx::detail::OnError_Noop>
    XRX_FORCEINLINE() auto make(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error = {})
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = ::xrx::detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(XRX_FWD(on_next)
            , XRX_FWD(on_completed)
            , XRX_FWD(on_error));
    }

    template<typename Value, typename Error
        , typename OnNext, typename OnCompleted, typename OnError>
    XRX_FORCEINLINE()
    ConceptObserverOf<Value, Error> auto
        make_of(OnNext&& on_next, OnCompleted&& on_completed, OnError&& on_error)
            requires ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_next>, OnNext, Value> &&
                     ::xrx::detail::is_cpo_invocable_v<tag_t<::xrx::detail::on_error>, OnError, Error>
    {
        using OnNext_      = std::remove_cvref_t<OnNext>;
        using OnCompleted_ = std::remove_cvref_t<OnCompleted>;
        using OnError_     = std::remove_cvref_t<OnError>;
        using Observer     = ::xrx::detail::LambdaObserver<OnNext_, OnCompleted_, OnError_>;
        return Observer(XRX_FWD(on_next)
            , XRX_FWD(on_completed)
            , XRX_FWD(on_error));
    }
} // namespace xrx::observer


// Header: utils_ref_tracking_observer.h.

// #pragma once

// #include "utils_defines.h"
// #include "utils_fast_FWD.h"
// #include "utils_observers.h"
#include <cassert>

namespace xrx::detail
{
    struct RefObserverState
    {
        bool _completed = false;
        bool _with_error = false;
        bool _unsubscribed = false;

        bool is_finalized() const
        {
            const bool terminated = (_completed || _with_error);
            return (terminated || _unsubscribed);
        }
    };

    template<typename Observer_>
    struct RefTrackingObserver_
    {
        Observer_* _observer = nullptr;
        RefObserverState* _state = nullptr;

        template<typename Value>
        XRX_FORCEINLINE() OnNextAction on_next(Value&& v)
        {
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);
            const auto action = on_next_with_action(*_observer, XRX_FWD(v));
            _state->_unsubscribed = action._stop;
            return action;
        }

        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);

            on_completed_optional(XRX_MOV(*_observer));
            _state->_completed = true;
        }

        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            assert(not _state->_with_error);

            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_observer));
            }
            else
            {
                on_error_optional(XRX_MOV(*_observer), XRX_MOV(e...));
            }
            _state->_with_error = true;
        }
    };
}


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
        static_assert(not std::is_reference_v<Value>);
        static_assert(not std::is_reference_v<Error>);
#define X_ANY_OBSERVER_SUPPORTS_COPY() 0

        struct ObserverConcept
        {
            virtual ~ObserverConcept() = default;
#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const = 0;
#endif
            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) = 0;
            virtual void on_error(XRX_RVALUE(Error&&) e) = 0;
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

            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
            }

            virtual void on_error(XRX_RVALUE(Error&&) e) override
            {
                return ::xrx::detail::on_error_optional(XRX_MOV(_observer), XRX_MOV(e));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
            }

            explicit Observer(XRX_RVALUE(ConcreateObserver&&) o)
                : _observer(XRX_MOV(o))
            {
                static_assert(not std::is_lvalue_reference_v<ConcreateObserver>);
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
        /*explicit*/ AnyObserver(XRX_RVALUE(ConcreateObserver&&) o)
            : _observer(std::make_unique<Observer<ConcreateObserver>>(XRX_MOV(o)))
        {
            static_assert(not std::is_lvalue_reference_v<ConcreateObserver>);
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
            : _observer(XRX_MOV(rhs._observer))
        {
        }
        AnyObserver& operator=(AnyObserver&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _observer = XRX_MOV(rhs._observer);
            }
            return *this;
        }

        explicit operator bool() const
        {
            return !!_observer;
        }

        std::unique_ptr<ObserverConcept> _observer;

        ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v)
        {
            assert(_observer);
            return _observer->on_next(XRX_MOV(v));
        }

        void on_error(XRX_RVALUE(Error&&) e)
        {
            assert(_observer);
            return _observer->on_error(XRX_MOV(e));
        }

        void on_completed()
        {
            assert(_observer);
            return _observer->on_completed();
        }
    };

    template<typename Value>
    struct AnyObserver<Value, void>
    {
        struct ObserverConcept
        {
            virtual ~ObserverConcept() = default;
#if (X_ANY_OBSERVER_SUPPORTS_COPY())
            virtual std::unique_ptr<ObserverConcept> copy_() const = 0;
#endif
            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) = 0;
            virtual void on_error() = 0;
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

            virtual ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v) override
            {
                return ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
            }

            virtual void on_error() override
            {
                return ::xrx::detail::on_error_optional(XRX_MOV(_observer));
            }

            virtual void on_completed()
            {
                return ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
            }

            explicit Observer(XRX_RVALUE(ConcreateObserver&&) o)
                : _observer(XRX_MOV(o))
            {
            }

            ConcreateObserver _observer;
        };

        /*explicit*/ AnyObserver() = default;

        template<typename ConcreateObserver>
            requires (!detail::IsAnyObserver<ConcreateObserver>::value)
        /*explicit*/ AnyObserver(XRX_RVALUE(ConcreateObserver&&) o)
            : _observer(std::make_unique<Observer<ConcreateObserver>>(XRX_MOV(o)))
        {
            static_assert(not std::is_lvalue_reference_v<ConcreateObserver>);
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
            : _observer(XRX_MOV(rhs._observer))
        {
        }
        AnyObserver& operator=(AnyObserver&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _observer = XRX_MOV(rhs._observer);
            }
            return *this;
        }

        explicit operator bool() const
        {
            return !!_observer;
        }

        std::unique_ptr<ObserverConcept> _observer;

        ::xrx::detail::OnNextAction on_next(XRX_RVALUE(Value&&) v)
        {
            assert(_observer);
            return _observer->on_next(XRX_MOV(v));
        }

        void on_error()
        {
            assert(_observer);
            return _observer->on_error();
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
// #include "utils_fast_FWD.h"

#include <type_traits>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct Observable_
    {
        static_assert(not std::is_reference_v<SourceObservable>);
        static_assert(ConceptObservable<SourceObservable>
            , "SourceObservable should satisfy Observable concept.");

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using Unsubscriber = typename SourceObservable::Unsubscriber;
        using is_async = IsAsyncObservable<SourceObservable>;

        static_assert(not std::is_reference_v<value_type>
            , "Observable<> does not support emitting values of reference type.");
        static_assert(not std::is_reference_v<error_type>
            , "Observable<> does not support emitting error of reference type.");

        SourceObservable _source;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            // #XXX: we can require nothing if is_async == false, I guess.
            static_assert(std::is_move_constructible_v<Observer>);
            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        Observable_&& fork() &&      { return XRX_MOV(*this); }
        Observable_   fork() &       { return Observable_(_source.fork()); }
        Observable_&& fork_move() && { return XRX_MOV(*this); }
        Observable_   fork_move() &  { return XRX_MOV(*this); }

        template<typename Scheduler>
        auto subscribe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            static_assert(not std::is_lvalue_reference_v<Scheduler>);
            return make_operator(detail::operator_tag::SubscribeOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            static_assert(not std::is_lvalue_reference_v<Scheduler>);
            return make_operator(detail::operator_tag::ObserveOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        auto publish() &&
        {
            return make_operator(detail::operator_tag::Publish()
                , XRX_MOV(*this));
        }

        template<typename F>
        auto filter(F&& f) &&
        {
            return make_operator(detail::operator_tag::Filter()
                , XRX_MOV(*this), XRX_FWD(f));
        }
        template<typename F>
        auto transform(F&& f) &&
        {
            return make_operator(detail::operator_tag::Transform()
                , XRX_MOV(*this), XRX_FWD(f));
        }
        auto take(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Take()
                , XRX_MOV(*this), std::size_t(count));
        }
        auto repeat(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , XRX_MOV(*this), std::size_t(count), std::false_type()/*not infinity*/);
        }
        auto repeat() &&
        {
            return make_operator(detail::operator_tag::Repeat()
                , XRX_MOV(*this), std::size_t(0), std::true_type()/*infinity*/);
        }
        template<typename Produce>
        auto flat_map(Produce&& produce) &&
        {
            return make_operator(detail::operator_tag::FlatMap()
                , XRX_MOV(*this), XRX_FWD(produce));
        }
        template<typename Produce, typename Map>
        auto flat_map(Produce&& produce, Map&& map) &&
        {
            return make_operator(detail::operator_tag::FlatMap()
                , XRX_MOV(*this), XRX_FWD(produce), XRX_FWD(map));
        }
        auto window(std::size_t count) &&
        {
            return make_operator(detail::operator_tag::Window()
                , XRX_MOV(*this), std::size_t(count));
        }
        template<typename Value, typename Op>
        auto reduce(Value&& initial, Op&& op) &&
        {
            return make_operator(detail::operator_tag::Reduce()
                , XRX_MOV(*this), XRX_FWD(initial), XRX_FWD(op));
        }
    };
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename Observer>
        struct RememberSubscribe
        {
            Observer _observer;

            template<typename SourceObservable>
            auto pipe_(SourceObservable source) &&
                requires requires { XRX_MOV(source).subscribe(XRX_MOV(_observer)); }
            {
                return XRX_MOV(source).subscribe(XRX_MOV(_observer));
            }
        };
    } // namespace detail

    template<typename Observer>
    inline auto subscribe(Observer observer)
    {
        return detail::RememberSubscribe<Observer>(XRX_MOV(observer));
    }
} // namespace xrx

template<typename SourceObservable, typename PipeConnect>
auto operator|(::xrx::detail::Observable_<SourceObservable>&& source_rvalue, PipeConnect connect)
    requires requires { XRX_MOV(connect).pipe_(XRX_MOV(source_rvalue)); }
{
    return XRX_MOV(connect).pipe_(XRX_MOV(source_rvalue));
}

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

            explicit SubscribeStateShared_(XRX_RVALUE(Scheduler&&) sheduler)
                : _mutex()
                , _scheduler(XRX_MOV(sheduler))
                , _try_cancel_subscribe(false)
                , _unsubscribers()
            {
            }

            static std::shared_ptr<SubscribeStateShared_> make(XRX_RVALUE(Scheduler&&) scheduler)
            {
                return std::make_shared<SubscribeStateShared_>(XRX_MOV(scheduler));
            }

            template<typename Observer>
                // requires ConceptValueObserverOf<Observer, value_type>
            void subscribe_impl(XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Observer&&) observer
                , std::true_type) // source unsubscriber does have unsubscribe effect; remember it.
            {
                assert(std::get_if<std::monostate>(&_unsubscribers)); // Not yet initialized.
                // The trick there (I hope correct one) is that we remember weak reference
                // to our state. If at the time of task_post() execution there will be
                // no strong reference, this would mean there is no *active* Unsubscriber.
                // If there is no Unsubscriber, there is no need to remember anything.
                auto shared_weak = this->weak_from_this();

                const TaskHandle task_handle = _scheduler.task_post(
                    [self = shared_weak, source_ = XRX_MOV(source), observer_ = XRX_MOV(observer)]() mutable
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

                    // #XXX: we should pass our own observer.
                    // And do check if unsubscribe happened while this
                    // .subscribe() is in progress.
                    SourceUnsubscriber unsubscriber = XRX_MOV(source_).subscribe(XRX_MOV(observer_));

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
                            in_progress->_unsubscriber.emplace(XRX_MOV(unsubscriber));
                            in_progress->_on_finish.notify_all();
                        }
                        else
                        {
                            // No-one is waiting yet. We can destroy temporary in-progress data
                            // and simply put final Unsubscriber.
                            Subscribed* subscribed = std::get_if<Subscribed>(&shared->_unsubscribers);
                            assert(subscribed && "No-one should change Subscribed state when it was already set");
                            subscribed->_state.template emplace<SubscribeEnded>(XRX_MOV(unsubscriber));
                        }
                    }
                });
                {
                    const std::lock_guard lock(_mutex);
                    if (auto* not_initialized = std::get_if<std::monostate>(&_unsubscribers); not_initialized)
                    {
                        _unsubscribers.template emplace<TaskHandle>(XRX_MOV(task_handle));
                    }
                    else if (auto* already_subscribing = std::get_if<Subscribed>(&_unsubscribers); already_subscribing)
                    {
                        // No need to remember `task_handle`. We already executed the task, nothing to cancel.
                        (void)task_handle;
                    }
                    else if (std::get_if<TaskHandle>(&_unsubscribers))
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
            using is_async = std::true_type;
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
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                static_assert(not std::is_lvalue_reference_v<Observer>);
                auto shared = StateShared_::make(XRX_MOV(_scheduler));
#if (0)
                // #TODO: implement subscribe_impl(..., std::false_type);
                using remember_source = typename StateShared_::SourceUnsubscriber::has_effect;
#else
                using remember_source = std::true_type;
#endif
                shared->subscribe_impl(XRX_MOV(_source), XRX_MOV(observer)
                    , remember_source());
                return Unsubscriber(shared);
            }

            auto fork() && { return SubscribeOnObservable_(XRX_MOV(_source), XRX_MOV(_scheduler)); }
            auto fork() &  { return SubscribeOnObservable_(_source.fork(), _scheduler); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>, xrx::detail::operator_tag::SubscribeOn
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Scheduler&&) scheduler)
    {
        static_assert(not std::is_lvalue_reference_v<SourceObservable>);
        static_assert(not std::is_lvalue_reference_v<Scheduler>);
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Scheduler_ = std::remove_reference_t<Scheduler>;
        using Impl = ::xrx::observable::detail::SubscribeOnObservable_<SourceObservable_, Scheduler_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(scheduler)));
    }
} // namespace xrx::detail::operator_tag

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
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                static_assert(not std::is_lvalue_reference_v<Observer>);
                using Observer_ = std::remove_reference_t<Observer>;

                const clock_duration period(_period);
                // To get zero tick at first. #TODO: revisit this logic.
                const clock_point start_from = (clock::now() - period);
                
                struct State
                {
                    Observer_ _observer;
                    value_type _ticks = 0;
                };

                const Handle handle = _scheduler.tick_every(start_from, period
                    , [](State& state) mutable
                {
                    const value_type now = state._ticks++;
                    const auto action = ::xrx::detail::on_next_with_action(state._observer, value_type(now));
                    return action._stop;
                }
                    , State(XRX_MOV(observer), value_type(0)));

                return Unsubscriber(handle, XRX_MOV(_scheduler));
            }

            auto fork() && { return IntervalObservable_(XRX_MOV(_period), XRX_MOV(_scheduler)); }
            auto fork() &  { return IntervalObservable_(_period, _scheduler); }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Duration, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::Interval
        , Duration period, XRX_RVALUE(Scheduler&&) scheduler)
    {
        using Scheduler_ = std::remove_reference_t<Scheduler>;
        using Impl = ::xrx::observable::detail::IntervalObservable_<Duration, Scheduler_>;
        return Observable_<Impl>(Impl(XRX_MOV(period), XRX_MOV(scheduler)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_take.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "concepts_observable.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
#include <type_traits>
#include <utility>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable>
    struct TakeObservable
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        template<typename Observer>
        struct TakeObserver_
        {
            Observer _observer;
            std::size_t _max = 0;
            std::size_t _taken = 0;

            // #TODO: on_error(void) support.
            OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                assert(_max > 0);
                assert(_taken < _max);
                const auto action = ::xrx::detail::on_next_with_action(_observer, XRX_MOV(v));
                if (++_taken >= _max)
                {
                    ::xrx::detail::on_completed_optional(XRX_MOV(_observer));
                    return OnNextAction{._stop = true};
                }
                return action;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                (void)::xrx::detail::on_completed_optional(XRX_MOV(observer));
                return Unsubscriber();
            }
            using TakeObserver_ = TakeObserver_<Observer>;
            return XRX_MOV(_source).subscribe(TakeObserver_(XRX_MOV(observer), _count));
        }

        TakeObservable fork() && { return TakeObservable(XRX_MOV(_source), _count); };
        TakeObservable fork() &  { return TakeObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Take
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using Impl = TakeObservable<SourceObservable>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        struct RememberTake
        {
            std::size_t _count = 0;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Take
                    , SourceObservable, std::size_t>
            {
                return make_operator(operator_tag::Take(), XRX_MOV(source), _count);
            }
        };
    } // namespace detail

    inline auto take(std::size_t count)
    {
        return detail::RememberTake(count);
    }
} // namespace xrx

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
            // Shared state, so when cancel happens we can
            // terminate task that should-be-canceled,
            // but was already scheduled.
            // #TODO: can this be done without shared state
            // or, perhaps, without creating a task per each item ?
            // Maybe we can Scheduler to schedule whole source Observable ?
            // (This should minimize amount of state/overhead per item).
            struct Shared_
            {
                Scheduler _sheduler;
                DestinationObserver_ _target;
                // #TODO: nicer memory orders; sequential is not needed.
                std::atomic_bool _unsubscribed = false;
            };
            std::shared_ptr<Shared_> _shared;
            
            static std::shared_ptr<Shared_> make_state(
                XRX_RVALUE(Scheduler&&) scheduler
                , XRX_RVALUE(DestinationObserver_&&) target)
            {
                return std::make_shared<Shared_>(XRX_MOV(scheduler), XRX_MOV(target));
            }

            ::xrx::unsubscribe on_next(XRX_RVALUE(Value&&) v)
            {
                if (_shared->_unsubscribed)
                {
                    return ::xrx::unsubscribe(true);
                }
                auto self = _shared;
                const auto handle = self->_sheduler.task_post(
                    [self, v_ = XRX_MOV(v)]() mutable
                {
                    if (self->_unsubscribed)
                    {
                        return;
                    }
                    const auto action = ::xrx::detail::on_next_with_action(
                        self->_target, XRX_MOV(v_));
                    if (action._stop)
                    {
                        self->_unsubscribed = true;
                    }
                });
                (void)handle;
                return ::xrx::unsubscribe(false);
            }

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr (::xrx::detail::ConceptWithOnError<DestinationObserver_, Error>)
                {
                    if (_shared->_unsubscribed)
                    {
                        return;
                    }
                    auto self = _shared;

                    if constexpr ((sizeof...(e)) == 0)
                    {
                        const auto handle = self->_sheduler.task_post(
                            [self]() mutable
                        {
                            if (not self->_unsubscribed)
                            {
                                (void)::xrx::detail::on_error(XRX_MOV(self->_target));
                            }
                        });
                        (void)handle;
                    }
                    else
                    {
                        const auto handle = self->_sheduler.task_post(
                            [self, es = std::make_tuple(XRX_MOV(e)...)]() mutable
                        {
                            if (not self->_unsubscribed)
                            {
                                std::apply([&](auto&&... error) // error is rvalue.
                                {
                                    (void)::xrx::detail::on_error(
                                        XRX_MOV(self->_target), XRX_FWD(error)...);
                                }
                                    , XRX_MOV(es));
                            }
                        });
                        (void)handle;
                    }
                }
                else
                {
                    // Observer does not actually supports errors,
                    // just don't schedule empty task.
                }
            }
            void on_completed()
            {
                if constexpr (::xrx::detail::ConceptWithOnCompleted<DestinationObserver_>)
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
                            (void)xrx::detail::on_completed(self->_target);
                        }
                    });
                    (void)handle;
                }
            }
        };

        template<typename SourceObservable, typename Scheduler>
        struct ObserveOnObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            // Even if `SourceObservable` is NOT Async, every item 
            // from it will be re-scheduled thru some abstract interface.
            // At the end of our .subscribe() we can't know at compile time
            // if all scheduled items where processed.
            using is_async = std::true_type;

            SourceObservable _source;
            // #TODO: define concept for `StreamScheduler`, see stream_scheduler().
            Scheduler _scheduler;

            auto fork() && { return ObserveOnObservable_(XRX_MOV(_source), XRX_MOV(_scheduler)); }
            auto fork() &  { return ObserveOnObservable_(_source.fork(), _scheduler); }

            struct Unsubscriber
            {
                using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
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
                    }
                    return _unsubscriber.detach();
                }
            };

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                using ObserverImpl_ = ObserveOnObserver_<value_type, error_type, Scheduler, Observer>;
                auto shared = ObserverImpl_::make_state(XRX_MOV(_scheduler), XRX_MOV(observer));
                
                // #XXX: here, in theory, if `_source` Observable is Sync.
                // AND we know amount of items it can emit (not infinity),
                // we can cache all of them and then scheduler only _few_
                // tasks, each of which will emit several items per task.
                // (Instead of scheduling task per item).
                auto handle = XRX_MOV(_source).subscribe(ObserverImpl_(shared));

                Unsubscriber unsubscriber;
                unsubscriber._unsubscriber = handle;
                // Share only stop flag with unsubscriber.
                unsubscriber._unsubscribed = {shared, &shared->_unsubscribed};
                return unsubscriber;
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename SourceObservable, typename Scheduler>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , xrx::detail::operator_tag::ObserveOn
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Scheduler&&) scheduler)
    {
        static_assert(not std::is_lvalue_reference_v<SourceObservable>);
        static_assert(not std::is_lvalue_reference_v<Scheduler>);
        using Source_ = std::remove_reference_t<SourceObservable>;
        using Scheduler_ = std::remove_reference_t<Scheduler>;
        using Impl = ::xrx::observable::detail::ObserveOnObservable_<Source_, Scheduler_>;
        // #TODO: add overload that directly accepts `StreamScheduler` so
        // client can pass more narrow interface.
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(scheduler).stream_scheduler()));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_range.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
#include <utility>
#include <type_traits>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Integer, bool Endless>
    struct RangeObservable
    {
        using value_type   = Integer;
        using error_type   = none_tag;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        explicit RangeObservable(Integer first, Integer last, std::intmax_t step)
            : _first(first)
            , _last(last)
            , _step(step)
        {
            assert(((step >= 0) && (last  >= first))
                || ((step <  0) && (first >= last)));
        }

        const Integer _first;
        const Integer _last;
        const std::intmax_t _step;

        static bool compare_(Integer, Integer, std::intmax_t, std::true_type/*endless*/)
        {
            return true;
        }
        static bool compare_(Integer current, Integer last, std::intmax_t step, std::false_type/*endless*/)
        {
            if (step >= 0)
            {
                return (current <= last);
            }
            return (current >= last);
        }
        static Integer do_step_(Integer current, std::intmax_t step)
        {
            if (step >= 0)
            {
                return Integer(current + Integer(step));
            }
            return Integer(std::intmax_t(current) + step);
        }

        template<typename Observer>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            constexpr std::bool_constant<Endless> _edless;
            Integer current = _first;
            while (compare_(current, _last, _step, _edless))
            {
                const OnNextAction action = on_next_with_action(observer, Integer(current));
                if (action._stop)
                {
                    return Unsubscriber();
                }
                current = do_step_(current, _step);
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return Unsubscriber();
        }

        RangeObservable fork()
        {
            return *this;
        }
    };

    template<typename Integer, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Range
        , Integer first, Integer last, std::intmax_t step, std::bool_constant<Endless>)
    {
        using Impl = RangeObservable<Integer, Endless>;
        return Observable_<Impl>(Impl(first, last, step));
    }
} // namespace xrx::detail

// Header: operators/operator_transform.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_wrappers.h"
// #include "utils_defines.h"
// #include "observable_interface.h"
#include <type_traits>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, typename Transform>
    struct TransformObservable
    {
        SourceObservable _source;
        Transform _transform;

        using source_type  = typename SourceObservable::value_type;
        using value_type   = decltype(_transform(std::declval<source_type>()));
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        static_assert(not std::is_reference_v<value_type>);

        TransformObservable fork() && { return TransformObservable(XRX_MOV(_source), XRX_MOV(_transform)); }
        TransformObservable fork()  & { return TransformObservable(_source.fork(), _transform); }

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct TransformObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Transform> _transform;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(source_type&&) v)
            {
                return on_next_with_action(_observer.get()
                    , _transform.get()(XRX_MOV(v)));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e...));
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            using TransformObserver = TransformObserver<Observer_>;
            return XRX_MOV(_source).subscribe(TransformObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_transform)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using F_ = std::remove_reference_t<F>;
        using Impl = TransformObservable<SourceObservable_, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(transform)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberTransform
        {
            F _transform;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Transform
                    , SourceObservable, F>
            {
                return make_operator(operator_tag::Transform()
                    , XRX_MOV(source), XRX_MOV(_transform));
            }
        };
    } // namespace detail

    template<typename F>
    auto transform(XRX_RVALUE(F&&) transform_)
    {
        using F_ = std::remove_reference_t<F>;
        return detail::RememberTransform<F_>(XRX_MOV(transform_));
    }
} // namespace xrx

// Header: operators/operator_repeat.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <optional>
#include <vector>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable, bool Endless, bool IsSourceAsync>
    struct RepeatObservable
    {
        static_assert((not IsSourceAsync)
            , "Async Observable .repeat() is not implemented yet.");

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using Unsubscriber = NoopUnsubscriber;
        using is_async = std::bool_constant<IsSourceAsync>;

        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
        static_assert(IsSourceAsync
            or (not SourceUnsubscriber::has_effect::value)
            , "If Observable is Sync, its unsubscriber should have no effect.");

        SourceObservable _source;
        std::size_t _max_repeats;

        struct SyncState_
        {
            using Values = std::vector<value_type>;

            std::optional<Values> _values;
            bool _unsubscribed = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct SyncObserver_
        {
            SyncState_* _state = nullptr;
            Observer* _observer = nullptr;
            std::size_t _max_repeats = 0;

            OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                if (_state->_unsubscribed)
                {
                    assert(false && "Call to on_next() even when unsubscribe() was requested.");
                    return OnNextAction{._stop = true};
                }
                const auto action = on_next_with_action(*_observer, XRX_MOV(v));
                if (action._stop)
                {
                    _state->_unsubscribed = true;
                    return action;
                }
                // Remember values if only need to be re-emitted.
                if (_max_repeats > 1)
                {
                    if (!_state->_values)
                    {
                        _state->_values.emplace();
                    }
                    _state->_values->push_back(XRX_FWD(v));
                }
                return OnNextAction();
            }

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if (not _state->_unsubscribed)
                {
                    // Trigger an error there to avoid a need to remember it.
                    if constexpr ((sizeof...(e)) == 0)
                    {
                        on_error_optional(XRX_MOV(*_observer));
                    }
                    else
                    {
                        on_error_optional(XRX_MOV(*_observer), XRX_MOV(e)...);
                    }
                    _state->_unsubscribed = true;
                }
                else
                {
                    assert(false && "Call to on_error() even when unsubscribe() was requested.");
                }
            }

            void on_completed()
            {
                if (not _state->_unsubscribed)
                {
                    _state->_completed = true;
                }
                else
                {
                    assert(false && "Call to on_completed() even when unsubscribe() was requested.");
                }
            }
        };

        // Synchronous version.
        template<typename Observer>
        NoopUnsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            if (_max_repeats == 0)
            {
                (void)on_completed_optional(XRX_MOV(observer));
                return NoopUnsubscriber();
            }
            using ObserverImpl = SyncObserver_<std::remove_reference_t<Observer>>;
            SyncState_ state;
            // Ignore unsubscriber since it should haven no effect since we are synchronous.
            (void)XRX_MOV(_source).subscribe(ObserverImpl(&state, &observer, _max_repeats));
            if (state._unsubscribed)
            {
                // Either unsubscribed or error.
                return NoopUnsubscriber();
            }
            assert(state._completed && "Sync. Observable is not completed after .subscribe() end.");
            if (state._values)
            {
                // Emit all values (N - 1) times by copying from what we remembered.
                // Last emit iteration will move all the values.
                for (std::size_t i = 1; i < _max_repeats - 1; ++i)
                {
                    for (const value_type& v : *state._values)
                    {
                        const auto action = on_next_with_action(observer
                            , value_type(v)); // copy.
                        if (action._stop)
                        {
                            return NoopUnsubscriber();
                        }
                    }
                }

                // Move values, we don't need them anymore.
                for (value_type& v : *state._values)
                {
                    const auto action = on_next_with_action(observer, XRX_MOV(v));
                    if (action._stop)
                    {
                        return NoopUnsubscriber();
                    }
                }
            }
            on_completed_optional(XRX_MOV(observer));
            return NoopUnsubscriber();
        }

        RepeatObservable fork() && { return RepeatObservable(XRX_MOV(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Repeat
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count, std::bool_constant<Endless>)
            requires ConceptObservable<SourceObservable>
    {
        using IsAsync_ = IsAsyncObservable<SourceObservable>;
        using Impl = RepeatObservable<SourceObservable, Endless, IsAsync_::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<bool Endless>
        struct RememberRepeat
        {
            std::size_t _count = 0;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Repeat
                    , SourceObservable, std::size_t, std::bool_constant<Endless>>
            {
                return make_operator(operator_tag::Repeat()
                    , XRX_MOV(source), _count, std::bool_constant<Endless>());
            }
        };
    } // namespace detail

    inline auto repeat()
    {
        return detail::RememberRepeat<true/*endless*/>(0);
    }

    inline auto repeat(std::size_t count)
    {
        return detail::RememberRepeat<false/*not endless*/>(count);
    }
} // namespace xrx

// Header: operators/operator_from.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Tuple>
    struct FromObservable
    {
        using value_type = typename std::tuple_element<0, Tuple>::type;
        using error_type = none_tag;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        Tuple _tuple;

        template<typename Observer>
        Unsubscriber subscribe(Observer&& observer) &&
        {
            auto invoke_ = [](auto& observer, auto&& value)
            {
                const auto action = on_next_with_action(observer, XRX_FWD(value));
                return (not action._stop);
            };

            const bool all_processed = std::apply([&](auto&&... vs)
            {
                return (invoke_(observer, XRX_FWD(vs)) && ...);
            }
                , XRX_MOV(_tuple));
            if (all_processed)
            {
                (void)on_completed_optional(XRX_FWD(observer));
            }
            return Unsubscriber();
        }

        FromObservable fork() && { return FromObservable(XRX_MOV(_tuple)); }
        FromObservable fork() &  { return FromObservable(_tuple); }
    };

    template<typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::From
        , V&& v0, Vs&&... vs)
    {
        static_assert((not std::is_lvalue_reference_v<V>)
                  && ((not std::is_lvalue_reference_v<Vs>) && ...)
            , ".from(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<std::remove_reference_t<V>, std::remove_reference_t<Vs>...>;
        using Impl = FromObservable<Tuple>;
        return Observable_<Impl>(Impl(Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail

// Header: operators/operator_concat.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Tuple, bool IsAsync>
    struct ConcatObservable;

    template<typename Tuple>
    struct ConcatObservable<Tuple, false/*Sync*/>
    {
        static_assert(std::tuple_size_v<Tuple> >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_tuple)); }
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        struct State
        {
            bool _unsubscribed = false;
            bool _end_with_error = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct OneObserver
        {
            Observer* _destination = nullptr;
            State* _state = nullptr;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(value_type&&) value)
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                const auto action = on_next_with_action(*_destination, XRX_MOV(value));
                _state->_unsubscribed = action._stop;
                return action;
            }
            XRX_FORCEINLINE() void on_completed()
            {
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                assert(not _state->_unsubscribed);
                _state->_completed = true; // on_completed(): nothing to do, move to next observable.
            }
            template<typename... VoidOrError>
            XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(*_destination));
                }
                else
                {
                    on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
                }
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
        NoopUnsubscriber subscribe(Observer&& observer) &&
        {
            using Observer_ = std::remove_reference_t<Observer>;
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                State state;
                auto unsubscribe = XRX_FWD(observable).subscribe(
                    OneObserver<Observer_>(&observer, &state));
                static_assert(not decltype(unsubscribe)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (state._unsubscribed || state._end_with_error);
                assert((state._completed || stop)
                    && "Sync Observable should be ended after .subscribe() return.");
                return (not stop);
            };

            const bool all_processed = std::apply([&](auto&&... observables)
            {
                return (invoke_(observer, XRX_FWD(observables)) && ...);
            }
                , XRX_MOV(_tuple));
            if (all_processed)
            {
                on_completed_optional(observer);
            }
            return NoopUnsubscriber();
        }
    };

    template<typename Observable1, typename Observable2, bool>
    struct HaveSameStreamTypes
    {
        static constexpr bool value =
               std::is_same_v<typename Observable1::value_type
                            , typename Observable2::value_type>
            && std::is_same_v<typename Observable1::error_type
                            , typename Observable2::error_type>;
    };

    template<typename Observable1, typename Observable2>
    struct HaveSameStreamTypes<Observable1, Observable2
        , false> // not Observables.
            : std::false_type
    {
    };

    // AreConcatCompatible<> is a combination of different workarounds
    // for MSVC and GCC. We just need to check if value_type (+error_type) are same
    // for both types, if they are Observables.
    template<typename Observable1, typename Observable2>
    struct AreConcatCompatible
    {
        static constexpr bool are_observables =
               ConceptObservable<Observable1>
            && ConceptObservable<Observable2>;
        static constexpr bool value =
            HaveSameStreamTypes<
                Observable1
              , Observable2
              , are_observables>::value;
    };

    template<typename Observable1, typename Observable2, typename... ObservablesRest>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Concat
        , XRX_RVALUE(Observable1&&) observable1
        , XRX_RVALUE(Observable2&&) observable2
        , XRX_RVALUE(ObservablesRest&&)... observables)
            requires  (AreConcatCompatible<Observable1, Observable2>::value
                   && (AreConcatCompatible<Observable1, ObservablesRest>::value && ...))
    {
        constexpr bool IsAnyAsync = false
            or ( IsAsyncObservable<Observable1>())
            or ( IsAsyncObservable<Observable2>())
            or ((IsAsyncObservable<ObservablesRest>()) or ...);

        using Tuple = std::tuple<
            std::remove_reference_t<Observable1>
            , std::remove_reference_t<Observable2>
            , std::remove_reference_t<ObservablesRest>...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(
            XRX_MOV(observable1)
            , XRX_MOV(observable2)
            , XRX_MOV(observables)...)));
    }
} // namespace xrx::detail

// Header: operators/operator_window.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "concepts_observable.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_containers.h"
// #include "utils_defines.h"
#include <type_traits>
#include <algorithm>
#include <vector>
#include <utility>

namespace xrx::detail
{
    template<typename SourceObservable, bool IsAsync>
    struct WindowProducerObservable; // Not yet implemented.

    template<typename Value>
    struct WindowSyncObservable
    {
        using value_type = Value;
        using error_type = none_tag;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        using Storage = SmallVector<value_type, 1>;
        Storage _values;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        NoopUnsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            const bool all = _values.for_each([&](const value_type& v) XRX_FORCEINLINE_LAMBDA()
            {
                const auto action = on_next_with_action(observer, value_type(v)); // copy.
                if (action._stop)
                {
                    return false;
                }
                return true;
            });
            if (all)
            {
                (void)on_completed_optional(XRX_MOV(observer));
            }
            return NoopUnsubscriber();
        }

        WindowSyncObservable fork() && { return WindowSyncObservable(XRX_MOV(*this)); }
        WindowSyncObservable fork() &  { return WindowSyncObservable(*this); }
    };

    template<typename SourceObservable>
    struct WindowProducerObservable<SourceObservable, false/*synchronous*/>
    {
        SourceObservable _source;
        std::size_t _count = 0;

        using source_value = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using ObservableValue_  = WindowSyncObservable<source_value>;
        using value_type   = Observable_<ObservableValue_>;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        using Storage = ObservableValue_::Storage;

        struct State_
        {
            bool _unsubscribed = false;
            bool _end_with_error = false;
            bool _completed = false;
        };

        template<typename Observer>
        struct ObserverImpl_
        {
            State_* _state = nullptr;
            Observer* _observer = nullptr;
            std::size_t _count = 0;
            Storage _values;

            auto on_next(XRX_RVALUE(source_value&&) value)
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);
                if (_values._size < _count)
                {
                    if (_values._size == 0)
                    {
                        _values = Storage(_count);
                    }
                    _values.push_back(XRX_MOV(value));
                }
                assert(_values._size <= _count);
                if (_values._size == _count)
                {
                    const auto action = on_next_with_action(*_observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    if (action._stop)
                    {
                        _state->_unsubscribed = true;
                        return ::xrx::unsubscribe(true);
                    }
                }
                return ::xrx::unsubscribe(false);
            }

            void on_completed()
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);

                bool finalize_ = true;
                if (_values._size > 0)
                {
                    const auto action = on_next_with_action(*_observer
                        , value_type(ObservableValue_(XRX_MOV(_values))));
                    finalize_ = (not action._stop);
                }
                if (finalize_)
                {
                    (void)on_completed_optional(XRX_MOV(*_observer));
                }
                _state->_completed = true;
            }

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                assert(not _state->_unsubscribed);
                assert(not _state->_end_with_error);
                assert(not _state->_completed);

                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(*_observer));
                }
                else
                {
                    on_error_optional(XRX_MOV(*_observer), XRX_MOV(e...));
                }
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                // It's unknown how many observables we should emit, just end the sequence.
                (void)on_completed_optional(XRX_MOV(observer));
                return Unsubscriber();
            }
            State_ state;
            auto unsubscribe = XRX_FWD(_source).subscribe(
                ObserverImpl_<Observer>(&state, &observer, _count, Storage()));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }

        WindowProducerObservable fork() && { return WindowProducerObservable(XRX_MOV(_source), _count); };
        WindowProducerObservable fork() &  { return WindowProducerObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Window
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Impl = WindowProducerObservable<SourceObservable_
            , IsAsyncObservable<SourceObservable_>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        struct RememberWindow
        {
            std::size_t _count = 0;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Window
                    , SourceObservable, std::size_t>
            {
                return make_operator(operator_tag::Window(), XRX_MOV(source), std::size_t(_count));
            }
        };
    } // namespace detail

    inline auto window(std::size_t count)
    {
        return detail::RememberWindow(count);
    }
} // namespace xrx

// Header: operators/operator_create.h.

// #pragma once
// #include "concepts_observable.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "observable_interface.h"
// #include "utils_ref_tracking_observer.h"
#include <utility>
#include <type_traits>

namespace xrx::observable
{
    template<typename X> struct Show_;

    namespace detail
    {
        template<typename Unsubscriber_>
        struct SubscribeDispatch
        {
            static_assert(::xrx::detail::ConceptUnsubscriber<Unsubscriber_>);
            using Unsubscriber = Unsubscriber_;

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, Observer&& observer)
            {
                return XRX_FWD(on_subscribe)(XRX_FWD(observer));
            }
        };

        template<>
        struct SubscribeDispatch<void>
        {
            using Unsubscriber = ::xrx::detail::NoopUnsubscriber;

            template<typename F, typename Observer>
            static Unsubscriber invoke_(F&& on_subscribe, Observer&& observer)
            {
                using Observer_ = std::remove_reference_t<Observer>;
                using RefObserver = ::xrx::detail::RefTrackingObserver_<Observer_>;
                ::xrx::detail::RefObserverState state;
                (void)XRX_FWD(on_subscribe)(RefObserver(&observer, &state));
                assert(state.is_finalized());
                return Unsubscriber();
            }
        };

        template<typename Return_>
        constexpr auto detect_if_async_return()
        {
            if constexpr (std::is_same_v<Return_, void>)
            {
                // Assume consumer does not remember reference to observer and simply
                // returns nothing. Should be documented that stream must be complete at
                // return point.
                return std::false_type();
            }
            else if constexpr (::xrx::detail::ConceptUnsubscriber<Return_>)
            {
                // Assume completed if returned unsubscriber with no effect.
                using has_effect = typename Return_::has_effect;
                return has_effect();
            }
            else
            {
                static_assert(::xrx::detail::AlwaysFalse<Return_>()
                    , "Custom observable in .create() returns unknown type. "
                      "Either void or Unsubscriber should be returned.");
            }
        };

        template<typename Value, typename Error, typename F>
        struct CustomObservable_
        {
            F _on_subscribe;

            using value_type = Value;
            using error_type = Error;

            using ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>;
            using Return_ = decltype(XRX_MOV(_on_subscribe)(ObserverArchetype()));
            using is_async = decltype(detect_if_async_return<Return_>());
            // is_async can be false for non-void Return_ where Unsubscribed is no-op.
            // In this case we should assume that stream is completed after
            // .subscribe() end. Using CustomUnsubscriber<void> will validate those assumptions.
            using ReturnIfAsync = std::conditional_t<is_async::value
                , Return_
                , void>;
            using SubscribeDispatch_ = SubscribeDispatch<ReturnIfAsync>;
            using Unsubscriber = typename SubscribeDispatch_::Unsubscriber;

            auto fork() && { return CustomObservable_(XRX_MOV(_on_subscribe)); }
            auto fork() &  { return CustomObservable_(_on_subscribe); }

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            Unsubscriber subscribe(Observer observer) &&
            {
                return SubscribeDispatch_::invoke_(XRX_MOV(_on_subscribe), XRX_MOV(observer));
            }
        };
    } // namespace detail
} // namespace xrx::observable

namespace xrx::detail::operator_tag
{
    template<typename Value, typename Error, typename F
        , typename ObserverArchetype = ::xrx::detail::ObserverArchetype<Value, Error>>
    auto tag_invoke(::xrx::tag_t<::xrx::detail::make_operator>
        , ::xrx::detail::operator_tag::Create<Value, Error>
        , XRX_RVALUE(F&&) on_subscribe)
            requires requires(ObserverArchetype observer)
        {
            on_subscribe(XRX_MOV(observer));
        }
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        static_assert(not std::is_same_v<Value, void>);
        static_assert(not std::is_reference_v<Value>);
        using F_ = std::remove_reference_t<F>;
        using Impl = ::xrx::observable::detail::CustomObservable_<Value, Error, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(on_subscribe)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_flat_map.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "observable_interface.h"
// #include "debug/assert_mutex.h"
#include <type_traits>
#include <utility>
#include <memory>
#include <vector>
#include <optional>
#include <cassert>

namespace xrx::detail
{
    struct FlatMapIdentity
    {
        template<typename SourceValue, typename InnerValue>
        InnerValue operator()(XRX_RVALUE(SourceValue&&) /*main_value*/, XRX_RVALUE(InnerValue&&) inner_value) const
        {
            static_assert(not std::is_lvalue_reference_v<SourceValue>);
            static_assert(not std::is_lvalue_reference_v<InnerValue>);
            return XRX_MOV(inner_value);
        }
    };

    template<typename E1, typename E2>
    struct MergedErrors
    {
        using are_compatible = std::is_same<E1, E1>;
        using E = E1;
    };
    template<>
    struct MergedErrors<void, none_tag>
    {
        using are_compatible = std::true_type;
        using E = void;
    };
    template<>
    struct MergedErrors<none_tag, void>
    {
        using are_compatible = std::true_type;
        using E = void;
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map
        , bool IsSourceAsync
        , bool IsProducedAsync>
    struct FlatMapObservable;

    struct State_Sync_Sync
    {
        bool _unsubscribed = false;
        bool _end_with_error = false;
        bool _completed = false;
    };

    template<typename Observer, typename Value, typename Map, typename SourceValue>
    struct InnerObserver_Sync_Sync
    {
        Observer* _destination = nullptr;
        State_Sync_Sync* _state = nullptr;
        Map* _map = nullptr;
        SourceValue* _source_value = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(Value&&) value)
        {
            static_assert(not std::is_lvalue_reference_v<Value>);
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            const auto action = on_next_with_action(*_destination
                , (*_map)(SourceValue(*_source_value)/*copy*/, XRX_MOV(value)));
            _state->_unsubscribed = action._stop;
            return action;
        }
        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            _state->_completed = true; // on_completed(): nothing to do, move to next observable.
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
            }
            _state->_end_with_error = true;
        }
    };

    template<typename ProducedValue, typename Observer, typename Observable, typename Map, typename SourceValue>
    static bool invoke_inner_sync_sync(Observer& destination_
        , XRX_RVALUE(Observable&&) inner
        , Map& map
        , XRX_RVALUE(SourceValue&&) source_value)
    {
        static_assert(not std::is_lvalue_reference_v<Observable>);
        static_assert(not std::is_lvalue_reference_v<SourceValue>);
        using InnerSync_ = InnerObserver_Sync_Sync<Observer, ProducedValue, Map, SourceValue>;
        State_Sync_Sync state;
        auto unsubscribe = XRX_MOV(inner).subscribe(
            InnerSync_(&destination_, &state, &map, &source_value));
        static_assert(not decltype(unsubscribe)::has_effect::value
            , "Sync Observable should not have unsubscribe.");
        const bool stop = (state._unsubscribed || state._end_with_error);
        assert((state._completed || stop)
            && "Sync Observable should be ended after .subscribe() return.");
        return (not stop);
    };

    template<typename Observer, typename Producer, typename Map, typename SourceValue, typename ProducedValue>
    struct OuterObserver_Sync_Sync
    {
        Producer* _produce = nullptr;
        Map* _map = nullptr;
        Observer* _destination = nullptr;
        State_Sync_Sync* _state = nullptr;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            SourceValue copy = source_value;
            const bool continue_ = invoke_inner_sync_sync<ProducedValue>(
                *_destination
                , (*_produce)(XRX_MOV(source_value))
                , *_map
                , XRX_MOV(copy));
            _state->_unsubscribed = (not continue_);
            return ::xrx::unsubscribe(_state->_unsubscribed);
        }
        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            on_completed_optional(*_destination);
            _state->_completed = true;
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
            }
            _state->_end_with_error = true;
        }
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , Map
        , false/*Sync source Observable*/
        , false/*Sync Observables produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        [[no_unique_address]] SourceObservable _source;
        [[no_unique_address]] Produce _produce;
        [[no_unique_address]] Map _map;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::false_type;
        using Unsubscriber = NoopUnsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Sync<Observer_, Produce, Map, source_type, produce_type>;
            State_Sync_Sync state;
            auto unsubscribe = XRX_MOV(_source).subscribe(
                Root_(&_produce, &_map, &destination_, &state));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return Unsubscriber();
        }
    };

    // The state of single subscription.
    template<typename SourceValue, typename Observable>
    struct ChildObservableState_Sync_Async
    {
        using Unsubscriber = typename Observable::Unsubscriber;

        SourceValue _source_value;
        std::optional<Observable> _observable;
        std::optional<Unsubscriber> _unsubscriber;
    };

    // Tricky state for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    // 
    // `_unsubscribe` is valid if there is shared data valid.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState
    {
        using Child_ = ChildObservableState_Sync_Async<SourceValue, Observable>;
        std::vector<Child_> _children;
        std::atomic_bool* _unsubscribe = nullptr;
    };

    template<typename Observable, typename SourceValue>
    struct Unsubscriber_Sync_Async
    {
        using has_effect = std::true_type;

        using AllObservables_ = AllObservablesState<Observable, SourceValue>;
        std::shared_ptr<AllObservables_> _shared;

        bool detach()
        {
            if (not _shared)
            {
                return false;
            }
            // `_unsubscribe` is a raw pointer into `_shared`.
            // Since _shared is valid, we know this pointer is valid too.
            assert(_shared->_unsubscribe);
            (void)_shared->_unsubscribe->exchange(true);
            bool at_least_one = false;
            for (auto& child : _shared->_children)
            {
                assert(child._unsubscriber);
                at_least_one |= child._unsubscriber->detach();
            }
            return at_least_one;
        }
    };

    template<typename Observer
        , typename Map
        , typename Observable
        , typename SourceValue
        , typename InnerValue>
    struct SharedState_Sync_Async
    {
        using AllObservables = AllObservablesState<Observable, SourceValue>;
        AllObservables _observables;
        Observer _observer;
        Map _map;
        std::size_t _subscribed_count;
        std::atomic<std::size_t> _completed_count;
        std::atomic<bool> _unsubscribe = false;
#if (0)
        debug::AssertMutex<> _serialize;
#else
        std::mutex _serialize;
#endif

        explicit SharedState_Sync_Async(XRX_RVALUE(AllObservables&&) observables
            , XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Map&&) map)
                : _observables(XRX_MOV(observables))
                , _observer(XRX_MOV(observer))
                , _map(XRX_MOV(map))
                , _subscribed_count(0)
                , _completed_count(0)
                , _unsubscribe(false)
                , _serialize()
        {
        }

        void unsubscribe_all()
        {
            // Multiple, in-parallel calls to `_children`
            // should be ok. We never invalidate it.
            for (auto& child : _observables._children)
            {
                assert(child._unsubscriber);
                child._unsubscriber->detach();
                // Note, we never reset _unsubscriber optional.
                // May be accessed by external Unsubscriber user holds.
            }
        }

        unsubscribe on_next(std::size_t child_index, XRX_RVALUE(InnerValue&&) inner_value)
        {
            assert(child_index < _observables._children.size());
            if (_unsubscribe)
            {
                // Not needed: called in all cases when `_unsubscribe` is set to true.
                // unsubscribe_all();
                return unsubscribe(true);
            }

            auto source_value = _observables._children[child_index]._source_value;
            bool stop = false;
            {
                // The lock there is ONLY to serialize (possibly)
                // parallel calls to on_next()/on_error().
                auto guard = std::lock_guard(_serialize);
                const auto action = on_next_with_action(_observer
                    , _map(XRX_MOV(source_value), XRX_MOV(inner_value)));
                stop = action._stop;
            }

            if (not stop)
            {
                return unsubscribe(false);
            }
            _unsubscribe = true;
            unsubscribe_all();
            return unsubscribe(true);
        }
        template<typename... VoidOrError>
        void on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            (void)child_index;
            assert(child_index < _observables._children.size());
            assert(not _unsubscribe);
            // The lock there is ONLY to serialize (possibly)
            // parallel calls to on_next()/on_error().
            if constexpr ((sizeof...(e)) == 0)
            {
                auto guard = std::lock_guard(_serialize);
                on_error_optional(XRX_MOV(_observer));
            }
            else
            {
                auto guard = std::lock_guard(_serialize);
                on_error_optional(XRX_MOV(_observer), XRX_MOV(e...));
            }
            _unsubscribe = true;
            unsubscribe_all();
        }
        void on_completed(std::size_t child_index)
        {
            assert(child_index < _observables._children.size());
            assert(not _unsubscribe);
            const std::size_t finished = (_completed_count.fetch_add(+1) + 1);
            assert(finished <= _subscribed_count);
            if (finished == _subscribed_count)
            {
                auto guard = std::lock_guard(_serialize);
                on_completed_optional(XRX_MOV(_observer));
            }
            auto& kill = _observables._children[child_index]._unsubscriber;
            assert(kill);
            kill->detach();
        }
    };

    template<typename SharedState_, typename Value, typename Error>
    struct InnerObserver_Sync_Async
    {
        std::size_t _index;
        std::shared_ptr<SharedState_> _shared;

        XRX_FORCEINLINE() auto on_next(XRX_RVALUE(Value&&) value)
        {
            return _shared->on_next(_index, XRX_MOV(value));
        }
        XRX_FORCEINLINE() void on_completed()
        {
            return _shared->on_completed(_index);
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            if constexpr ((sizeof...(e)) == 0)
            {
                return _shared->on_error(_index);
            }
            else
            {
                return _shared->on_error(_index, XRX_MOV(e...));
            }
        }
    };

    template<typename Observer, typename Producer, typename Observable, typename SourceValue>
    struct OuterObserver_Sync_Async
    {
        Observer* _destination = nullptr;
        AllObservablesState<Observable, SourceValue>* _observables;
        Producer* _produce = nullptr;
        State_Sync_Sync* _state = nullptr;

        XRX_FORCEINLINE() void on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            SourceValue copy = source_value;
            _observables->_children.emplace_back(XRX_MOV(copy)
                , std::optional<Observable>((*_produce)(XRX_MOV(source_value))));
        }
        XRX_FORCEINLINE() void on_completed()
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            // Do not complete stream yet. Need to subscribe &
            // emit all children items.
            _state->_completed = true;
        }
        template<typename... VoidOrError>
        XRX_FORCEINLINE() void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state->_end_with_error);
            assert(not _state->_completed);
            assert(not _state->_unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(*_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e...));
            }
            _state->_end_with_error = true;
        }
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , Map
        , false /*Sync source Observable*/
        , true  /*Async Observables produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        [[no_unique_address]] SourceObservable _source;
        [[no_unique_address]] Produce _produce;
        [[no_unique_address]] Map _map;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::true_type;

        using Unsubscriber = Unsubscriber_Sync_Async<ProducedObservable, source_type>;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using Root_ = OuterObserver_Sync_Async<Observer_, Produce, ProducedObservable, source_type>;
            using AllObsevables = AllObservablesState<ProducedObservable, source_type>;
            using SharedState_ = SharedState_Sync_Async<Observer_, Map, ProducedObservable, source_type, produce_type>;
            using InnerObserver_ = InnerObserver_Sync_Async<SharedState_, produce_type, error_type>;

            State_Sync_Sync state;
            AllObsevables observables;
            auto unsubscribe = XRX_MOV(_source).subscribe(
                Root_(&destination_, &observables, &_produce, &state));
            static_assert(not decltype(unsubscribe)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            if (observables._children.size() == 0)
            {
                // Done. Nothing to subscribe to.
                return Unsubscriber();
            }

            auto shared = std::make_shared<SharedState_>(XRX_MOV(observables)
                , XRX_MOV(destination_), XRX_MOV(_map));
            shared->_observables._unsubscribe = &shared->_unsubscribe;
            shared->_subscribed_count = shared->_observables._children.size();
            std::shared_ptr<AllObsevables> unsubscriber(shared, &shared->_observables);

            for (std::size_t i = 0; i < shared->_subscribed_count; ++i)
            {
                auto& child = shared->_observables._children[i];
                assert(child._observable);
                assert(not child._unsubscriber);
                child._unsubscriber.emplace(child._observable->fork_move()
                    .subscribe(InnerObserver_(i, shared)));
                child._observable.reset();
            }

            return Unsubscriber(XRX_MOV(unsubscriber));
        }
    };

    template<typename Observer, typename Producer, typename Map, typename SourceValue, typename ProducedValue>
    struct OuterObserver_Async_Sync
    {
        Producer _produce;
        Map _map;
        Observer _destination;
        State_Sync_Sync _state;

        explicit OuterObserver_Async_Sync(XRX_RVALUE(Producer&&) produce, XRX_RVALUE(Map&&) map, XRX_RVALUE(Observer&&) observer)
            : _produce(XRX_MOV(produce))
            , _map(XRX_MOV(map))
            , _destination(XRX_MOV(observer))
            , _state()
        {
        }

        auto on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            SourceValue copy = source_value;
            const bool continue_ = invoke_inner_sync_sync<ProducedValue>(
                _destination
                , (_produce)(XRX_MOV(source_value))
                , _map
                , XRX_MOV(copy));
            _state._unsubscribed = (not continue_);
            return ::xrx::unsubscribe(_state._unsubscribed);
        }
        void on_completed()
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            on_completed_optional(_destination);
            _state._completed = true;
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e...));
            }
            _state._end_with_error = true;
        }
    };

    template<typename SourceObservable
        , typename ProducedObservable
        , typename Produce
        , typename Map>
    struct FlatMapObservable<
        SourceObservable
        , ProducedObservable
        , Produce
        , Map
        , true  /*Async source Observable*/
        , false /*Sync Observables produced*/>
    {
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");

        [[no_unique_address]] SourceObservable _source;
        [[no_unique_address]] Produce _produce;
        [[no_unique_address]] Map _map;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        using Errors = MergedErrors<source_error, produce_error>;

        static_assert(not std::is_same_v<produce_type, void>
            , "Observables with void value type are not yet supported.");

        static_assert(Errors::are_compatible::value
            , "Different error types for Source Observable and Produced Observable are not yet supported.");

        using map_value = decltype(_map(std::declval<source_type>(), std::declval<produce_type>()));
        static_assert(not std::is_same_v<map_value, void>
            , "Observables with void value type are not yet supported. "
              "As result of applying given Map.");
        static_assert(not std::is_reference_v<map_value>
            , "Map should return value-like type.");

        using value_type = map_value;
        using error_type = typename Errors::E;
        using is_async = std::true_type;

        using Unsubscriber = typename SourceObservable::Unsubscriber;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using OuterObserver_ = OuterObserver_Async_Sync<Observer, Produce, Map, source_type, produce_type>;

            OuterObserver_ outer(XRX_MOV(_produce), XRX_MOV(_map), XRX_MOV(destination_));
            return XRX_MOV(_source).subscribe(XRX_MOV(outer));
        }
    };

    template<typename SourceObservable, typename Produce, typename Map>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce, XRX_RVALUE(Map&&) map)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Map_ = std::remove_reference_t<Map>;
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable_::value_type>()));
        using Impl = FlatMapObservable<
              SourceObservable_
            , ProducedObservable
            , Produce
            , Map_
            , IsAsyncObservable<SourceObservable_>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), XRX_MOV(map)));
    }

    template<typename SourceObservable, typename Produce>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using Map_ = FlatMapIdentity;
        using ProducedObservable = decltype(produce(std::declval<typename SourceObservable_::value_type>()));
        using Impl = FlatMapObservable<
              SourceObservable_
            , ProducedObservable
            , Produce
            , Map_
            , IsAsyncObservable<SourceObservable_>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), FlatMapIdentity()));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F, typename Map>
        struct RememberFlatMap
        {
            [[no_unique_address]] F _produce;
            [[no_unique_address]] Map _map;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::FlatMap
                    , SourceObservable, F, Map>
            {
                static_assert(not std::is_lvalue_reference_v<SourceObservable>);
                return make_operator(operator_tag::FlatMap()
                    , XRX_MOV(source), XRX_MOV(_produce), XRX_MOV(_map));
            }
        };
    } // namespace detail

    template<typename F>
    auto flat_map(XRX_RVALUE(F&&) produce)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        using F_ = std::remove_reference_t<F>;
        using IdentityMap = detail::FlatMapIdentity;
        return detail::RememberFlatMap<F_, IdentityMap>(XRX_MOV(produce), IdentityMap());
    }

    template<typename F, typename Map>
    auto flat_map(XRX_RVALUE(F&&) produce, XRX_RVALUE(Map&&) map)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        static_assert(not std::is_lvalue_reference_v<Map>);
        using F_ = std::remove_reference_t<F>;
        using Map_ = std::remove_reference_t<Map>;
        return detail::RememberFlatMap<F_, Map_>(XRX_MOV(produce), XRX_MOV(map));
    }
} // namespace xrx

// Header: operators/operator_reduce.h.

// #pragma once
// #include "concepts_observer.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
// #include "utils_defines.h"
// #include "utils_wrappers.h"
// #include "observable_interface.h"
#include <utility>
#include <concepts>

namespace xrx::detail
{
    template<typename SourceObservable, typename Value, typename Op>
    struct ReduceObservable
    {
        static_assert(not std::is_reference_v<SourceObservable>);
        static_assert(not std::is_reference_v<Op>);

        using source_type = typename SourceObservable::value_type;
        using value_type   = Value;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;
        Value _initial;
        Op _op;

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct ReduceObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Op> _op;
            Value _value;

            XRX_FORCEINLINE() void on_next(XRX_RVALUE(source_type&&) v)
            {
                _value = _op.get()(XRX_MOV(_value), XRX_MOV(v));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                const auto action = ::xrx::detail::on_next_with_action(_observer.get(), XRX_MOV(_value));
                if (not action._stop)
                {
                    on_completed_optional(XRX_MOV(_observer.get()));
                }
            }

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e...));
                }
            }
        };

        ReduceObservable fork() && { return ReduceObservable(XRX_MOV(_source), XRX_MOV(_op)); }
        ReduceObservable fork() &  { return ReduceObservable(_source.fork(), _op); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using ReduceObserver_ = ReduceObserver<Observer_>;
            return XRX_MOV(_source).subscribe(ReduceObserver_(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_op), XRX_MOV(_initial)));
        }
    };

    template<typename SourceObservable, typename Value, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Reduce
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Value&&) value, XRX_RVALUE(F&&) op)
            requires ConceptObservable<SourceObservable>
                && requires { { op(XRX_MOV(value), std::declval<typename SourceObservable::value_type>()) } -> std::convertible_to<Value>; }
    {
        static_assert(not std::is_lvalue_reference_v<SourceObservable>);
        static_assert(not std::is_lvalue_reference_v<Value>);
        static_assert(not std::is_lvalue_reference_v<F>);
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using F_ = std::remove_reference_t<F>;
        using Value_ = std::remove_reference_t<Value>;
        using Impl = ReduceObservable<SourceObservable_, Value_, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(value), XRX_MOV(op)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename Value, typename F>
        struct RememberReduce
        {
            Value _value;
            F _f;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Reduce
                    , SourceObservable, Value, F>
            {
                static_assert(not std::is_lvalue_reference_v<SourceObservable>);
                return make_operator(operator_tag::Reduce()
                    , XRX_MOV(source), XRX_MOV(_value), XRX_MOV(_f));
            }
        };
    } // namespace detail

    template<typename Value, typename F>
    auto reduce(XRX_RVALUE(Value&&) initial, XRX_RVALUE(F&&) op)
    {
        static_assert(not std::is_lvalue_reference_v<Value>);
        static_assert(not std::is_lvalue_reference_v<F>);
        using F_ = std::remove_reference_t<F>;
        using Value_ = std::remove_reference_t<Value>;
        return detail::RememberReduce<Value_, F_>(XRX_MOV(initial), XRX_MOV(op));
    }
} // namespace xrx

// Header: operators/operator_filter.h.

// #pragma once
// #include "concepts_observer.h"
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
// #include "utils_defines.h"
// #include "utils_wrappers.h"
// #include "observable_interface.h"
#include <utility>
#include <concepts>

namespace xrx::detail
{
    template<typename SourceObservable, typename Filter>
    struct FilterObservable
    {
        static_assert(not std::is_reference_v<SourceObservable>);
        static_assert(not std::is_reference_v<Filter>);

        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using Unsubscriber = typename SourceObservable::Unsubscriber;

        SourceObservable _source;
        Filter _filter;

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct FilterObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Filter> _filter;

            XRX_FORCEINLINE() OnNextAction on_next(XRX_RVALUE(value_type&&) v)
            {
                if (_filter.get()(v))
                {
                    return ::xrx::detail::on_next_with_action(_observer.get(), XRX_MOV(v));
                }
                return OnNextAction();
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e...));
                }
            }
        };

        FilterObservable fork() && { return FilterObservable(XRX_MOV(_source), XRX_MOV(_filter)); }
        FilterObservable fork() &  { return FilterObservable(_source.fork(), _filter); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            using Observer_ = std::remove_reference_t<Observer>;
            using FilterObserver = FilterObserver<Observer_>;
            return XRX_MOV(_source).subscribe(FilterObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_filter)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) filter)
            requires ConceptObservable<SourceObservable>
                && std::predicate<F, typename SourceObservable::value_type>
    {
        static_assert(not std::is_lvalue_reference_v<SourceObservable>);
        static_assert(not std::is_lvalue_reference_v<F>);
        using SourceObservable_ = std::remove_reference_t<SourceObservable>;
        using F_ = std::remove_reference_t<F>;
        using Impl = FilterObservable<SourceObservable_, F_>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(filter)));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        template<typename F>
        struct RememberFilter
        {
            F _f;

            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Filter
                    , SourceObservable, F>
            {
                static_assert(not std::is_lvalue_reference_v<SourceObservable>);
                return make_operator(operator_tag::Filter()
                    , XRX_MOV(source), XRX_MOV(_f));
            }
        };
    } // namespace detail

    template<typename F>
    auto filter(XRX_RVALUE(F&&) filter)
    {
        static_assert(not std::is_lvalue_reference_v<F>);
        using F_ = std::remove_reference_t<F>;
        return detail::RememberFilter<F_>(XRX_MOV(filter));
    }
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
        using is_async = IsAsyncObservable<SourceObservable>;
        using AnyObserver_ = AnyObserver<value_type, error_type>;
        using Subscriptions = detail::HandleVector<AnyObserver_>;
        using Handle = typename Subscriptions::Handle;
        using SourceUnsubscriber = typename SourceObservable::Unsubscriber;

        struct SharedImpl_;

        struct Observer_
        {
            std::shared_ptr<SharedImpl_> _shared;
            bool _do_refcount = false;

            void on_next(value_type&& v);
            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e);
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
                : _unsubscriber(XRX_MOV(unsubscriber))
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
            using is_async = IsAsyncObservable<SourceObservable>;
            using Unsubscriber = typename ConnectObservableState_::Unsubscriber;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
            auto subscribe(Observer&& observer) &&;

            auto fork() && { return RefCountObservable_(XRX_MOV(_shared)); }
            auto fork()  & { return RefCountObservable_(_shared); }
        };

        struct SharedImpl_ : public std::enable_shared_from_this<SharedImpl_>
        {
            using Base = std::enable_shared_from_this<SharedImpl_>;

            [[no_unique_address]] debug::AssertMutex<> _assert_mutex;
            SourceObservable _source;
            Subscriptions _subscriptions;
            std::optional<SourceUnsubscriber> _connected;

            explicit SharedImpl_(XRX_RVALUE(SourceObservable&&) source)
                : Base()
                , _source(XRX_MOV(source))
                , _subscriptions()
                , _connected(std::nullopt)
            {
            }

            SourceUnsubscriber connect(bool do_refcount)
            {
                auto guard = std::unique_lock(_assert_mutex);
                if (_connected)
                {
                    return SourceUnsubscriber();
                }

                using SourceUnsubscriber = typename SourceObservable::Unsubscriber;
                SourceUnsubscriber unsubscriber;
                {
                    auto unguard = debug::ScopeUnlock(guard);
                    // #XXX: `_source` that remembers Observer that takes strong reference to this.
                    // Circular dependency ? Check this.
                    unsubscriber = _source.fork().subscribe(
                        Observer_(this->shared_from_this(), do_refcount));
                }
                // #TODO: if this fires, we need to take a lock while doing
                // .subscribe() above. Since .subscribe() itself can trigger
                // .on_next() call, we need to have recursive mutex.
                assert(not _connected && "Multiple parallel .connect() calls.");
                _connected = unsubscriber;
                return unsubscriber;
            }

            template<typename Observer>
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer, bool do_refcount = false)
            {
                static_assert(not std::is_lvalue_reference_v<Observer>);
                std::size_t count_before = 0;
                AnyObserver_ erased_observer(XRX_MOV(observer));
                Unsubscriber unsubscriber;
                unsubscriber._shared = Base::shared_from_this();
                {
                    auto guard = std::lock_guard(_assert_mutex);
                    count_before = _subscriptions.size();
                    unsubscriber._handle = _subscriptions.push_back(XRX_MOV(erased_observer));
                }
                if (do_refcount && (count_before == 0))
                {
                    connect(do_refcount);
                }
                return unsubscriber;
            }

            bool unsubscribe(Handle handle, bool do_refcount)
            {
                auto guard = std::lock_guard(_assert_mutex);
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

            void on_next_impl(XRX_RVALUE(value_type&&) v, bool do_refcount)
            {
                auto lock = std::unique_lock(_assert_mutex);
                _subscriptions.for_each([&](AnyObserver_& observer, Handle handle)
                {
                    auto unguard = debug::ScopeUnlock(lock);
                    const auto action = observer.on_next(value_type(v)); // copy.
                    if (action._stop)
                    {
                        unsubscribe(handle, do_refcount);
                    }
                });
                // #XXX: should we return there for caller's on_next() ?
            }
        };

        std::shared_ptr<SharedImpl_> _shared;

        explicit ConnectObservableState_(XRX_RVALUE(SourceObservable&&) source)
            : _shared(std::make_shared<SharedImpl_>(XRX_MOV(source)))
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
        using is_async = IsAsyncObservable<SourceObservable>;

        std::weak_ptr<SharedImpl_> _shared_weak;

        template<typename Observer>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            if (auto shared = _shared_weak.lock(); shared)
            {
                return shared->subscribe(XRX_MOV(observer));
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

        XRX_FORCEINLINE() SharedState_& state() { return static_cast<SharedState_&>(*this); }

        explicit ConnectObservable_(XRX_RVALUE(SourceObservable&&) source)
            : SharedState_(XRX_MOV(source))
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
    void ConnectObservableState_<SourceObservable>::Observer_::on_next(XRX_RVALUE(value_type&&) v)
    {
        assert(_shared);
        _shared->on_next_impl(XRX_MOV(v), _do_refcount);
    }

    template<typename SourceObservable>
    template<typename... VoidOrError>
    void ConnectObservableState_<SourceObservable>::Observer_::on_error(XRX_RVALUE(VoidOrError&&)... e)
    {
        assert(_shared);
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto unguard = debug::ScopeUnlock(lock);
            if constexpr ((sizeof...(e)) == 0)
            {
                observer.on_error();
            }
            else
            {
                observer.on_error(error_type(e)...); // copy error.
            }
        });
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::Observer_::on_completed()
    {
        assert(_shared);
        auto lock = std::unique_lock(_shared->_assert_mutex);
        _shared->_subscriptions.for_each([&](AnyObserver_& observer)
        {
            auto unguard = debug::ScopeUnlock(lock);
            observer.on_completed();
        });
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Unsubscriber::detach(bool do_refcount /*= false*/)
    {
        assert(_shared);
        return _shared->unsubscribe(_handle, do_refcount);
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_
        ::subscribe(XRX_RVALUE(Observer&&) observer) &&
    {
        static_assert(not std::is_lvalue_reference_v<Observer>);
        assert(_shared);
        const bool do_refcount = true;
        return RefCountUnsubscriber(_shared->subscribe(
            XRX_MOV(observer), do_refcount));
    }

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Publish
        , XRX_RVALUE(SourceObservable&&) source)
            requires ConceptObservable<SourceObservable>
    {
        return ConnectObservable_<SourceObservable>(XRX_MOV(source));
    }
} // namespace xrx::detail

namespace xrx
{
    namespace detail
    {
        struct RememberPublish
        {
            template<typename SourceObservable>
            auto pipe_(XRX_RVALUE(SourceObservable&&) source) &&
                requires is_cpo_invocable_v<tag_t<make_operator>, operator_tag::Publish
                    , SourceObservable>
            {
                return make_operator(operator_tag::Publish(), XRX_MOV(source));
            }
        };
    } // namespace detail

    inline auto publish()
    {
        return detail::RememberPublish();
    }
} // namespace xrx

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
    template<typename Value, typename Error = void>
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
                    auto guard = std::lock_guard(shared->_assert_mutex);
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

        explicit Subject_(std::shared_ptr<SharedImpl_> impl)
            : _shared(XRX_MOV(impl))
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
            Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
            {
                static_assert(not std::is_lvalue_reference_v<Observer>);
                if (auto shared = _shared_weak.lock(); shared)
                {
                    AnyObserver<value_type, error_type> erased(XRX_MOV(observer));
                    auto guard = std::lock_guard(shared->_assert_mutex);
                    Unsubscriber unsubscriber;
                    unsubscriber._shared_weak = _shared_weak;
                    unsubscriber._handle = shared->_subscriptions.push_back(XRX_MOV(erased));
                    return unsubscriber;
                }
                return Unsubscriber();
            }

            auto fork() { return SourceObservable(_shared_weak); }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        Unsubscriber subscribe(XRX_RVALUE(Observer&&) observer)
        {
            static_assert(not std::is_lvalue_reference_v<Observer>);
            return as_observable().subscribe(XRX_MOV(observer));
        }

        ::xrx::detail::Observable_<SourceObservable> as_observable()
        {
            return ::xrx::detail::Observable_<SourceObservable>(SourceObservable(_shared));
        }

        Subject_ as_observer()
        {
            return Subject_(_shared);
        }

        void on_next(XRX_RVALUE(value_type&&) v)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer, Handle handle)
                {
                    bool do_unsubscribe = false;
                    {
                        auto guard = debug::ScopeUnlock(lock);
                        const auto action = observer.on_next(value_type(v)); // copy.
                        do_unsubscribe = action._stop;
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

        template<typename... Es>
        void on_error(Es&&... errors)
        {
            if (_shared)
            {
                auto lock = std::unique_lock(_shared->_assert_mutex);
                _shared->_subscriptions.for_each([&](AnyObserver<Value, Error>& observer)
                {
                    auto guard = debug::ScopeUnlock(lock);
                    if constexpr (sizeof...(errors) == 0)
                    {
                        observer.on_error();
                    }
                    else
                    {
                        static_assert(sizeof...(errors) == 1);
                        observer.on_error(Es(errors)...); // copy.
                    }
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
                    auto guard = debug::ScopeUnlock(lock);
                    observer.on_completed();
                });
                _shared.reset(); // done.
            }
            // else: already completed
        }
    };
} // namespace xrx
