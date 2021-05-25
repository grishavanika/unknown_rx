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
    struct NoopDetach
    {
        using has_effect = std::false_type;
        constexpr bool operator()() const noexcept { return false; }
    };
} // namespace xrx::detail

// Header: utils_fast_fwd.h.

// #pragma once
#include <utility>
#include <type_traits>

// #XXX: this needs to be without include-guard
// and in something like XRX_prelude.h header
// so it can be undefined in XRX_conclusion.h.
// #XXX: do with static_cast<>. Get read of <utility> include.
// https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html
// https://github.com/SuperV1234/vrm_core/issues/1
#define XRX_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define XRX_MOV(...) ::std::move(__VA_ARGS__)

// #XXX: move to utils_defines.h.
// Just an annotation that T&& should be rvalue reference
// (because of constraints from other places).
// XRX_MOV() can be used.
#define XRX_RVALUE(...) __VA_ARGS__
#define XRX_CHECK_RVALUE(...) \
    static_assert(not std::is_lvalue_reference_v<decltype(__VA_ARGS__)> \
        , "Expected to have rvalue reference. " \
        # __VA_ARGS__)

#define XRX_CHECK_TYPE_NOT_REF(...) \
    static_assert(not std::is_reference_v<__VA_ARGS__> \
        , "Expected to have non-reference type (not T& ot T&&; to be moved from). " \
        # __VA_ARGS__)

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

// Header: utils_defines.h.

// #pragma once

#if defined(_MSC_VER)
  #define XRX_FORCEINLINE() __forceinline
  #define XRX_FORCEINLINE_LAMBDA()
#else
  // #XXX: check why GCC says:
  // warning: 'always_inline' function might not be inlinable [-Wattributes]
  // on simple functions. Something done in a wrong way.
#  if (1)
#    define XRX_FORCEINLINE() __attribute__((always_inline))
#    define XRX_FORCEINLINE_LAMBDA() __attribute__((always_inline))
#  else
#    define XRX_FORCEINLINE() 
#    define XRX_FORCEINLINE_LAMBDA() 
#  endif
#endif

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

    template<typename Detach_>
    concept ConceptDetachHandle =
           ConceptHandle<Detach_>
        && requires(Detach_ detach)
    {
        // MSVC fails to compile ConceptObservable<> if this line
        // is enabled (because of subscribe() -> ConceptObservable<> check.
        // typename Detach_::has_effect;

        { detach() } -> std::convertible_to<bool>;
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
        typename ObservableLike_::DetachHandle;

        { std::declval<ObservableLike_>().subscribe(ObserverArchetype<
              typename ObservableLike_::value_type
            , typename ObservableLike_::error_type>()) }
                -> ConceptDetachHandle<>;
        // #TODO: should return Self - ConceptObservable<> ?
        { std::declval<ObservableLike_&>().fork() }
            -> ConceptHandle<>;
    };
} // namespace xrx::detail

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

    struct Iterate
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .iterate() operator implementation. "
                  "Missing `operators/operator_iterate.h` include ?");
        };
    };

    struct TapOrDo
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .tap()/.do_() operator implementation. "
                  "Missing `operators/operator_tap_do.h` include ?");
        };
    };

    struct WindowToggle
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .window_toggle() operator implementation. "
                  "Missing `operators/operator_window_toggle.h` include ?");
        };
    };

    struct StartsWith
    {
        template<typename _>
        struct NotFound
        {
            static_assert(AlwaysFalse<_>()
                , "Failed to find .start_with() operator implementation. "
                  "Missing `operators/operator_start_with.h` include ?");
        };
    };

} // namespace xrx::detail::operator_tag

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
        std::recursive_mutex _mutex;
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

            XRX_FORCEINLINE() explicit operator bool() const
            {
                return (_version > 0);
            }

            XRX_FORCEINLINE() friend bool operator==(Handle lhs, Handle rhs) noexcept
            {
                return (lhs._version == rhs._version)
                    && (lhs._index == rhs._index);
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

        XRX_FORCEINLINE() std::size_t size() const
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

        struct ValueTraits
        {
            using value_type = T;
            using pointer = T*;
            using reference = T&;

            XRX_FORCEINLINE() static T& as_reference(std::size_t, Value& v)
            {
                return v._data;
            }
            XRX_FORCEINLINE() static T* as_pointer(std::size_t, Value& v)
            {
                return &v._data;
            }
        };

        struct ValueWithHandle
        {
            T& _value;
            Handle _handle;
        };

        struct ValueWithHandleTraits
        {
            struct Proxy_
            {
                ValueWithHandle _data;
                explicit Proxy_(Value& value, Handle handle)
                    : _data(value, XRX_MOV(handle))
                {
                }

                ValueWithHandle* operator->() const
                {
                    return &_data;
                }
            };

            using value_type = ValueWithHandle;
            using pointer = Proxy_;
            using reference = ValueWithHandle;

            XRX_FORCEINLINE() static ValueWithHandle as_reference(std::size_t index, Value& v)
            {
                return ValueWithHandle(v._data, Handle(v._version, std::uint32_t(index)));
            }
            XRX_FORCEINLINE() static Proxy_ as_pointer(std::size_t index, Value& v)
            {
                return Proxy_(v._data, Handle(v._version, std::uint32_t(index)));
            }
        };

        template<typename Traits>
        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = typename Traits::value_type;
            using pointer = typename Traits::pointer;
            using reference = typename Traits::reference;

            HandleVector* _self = nullptr;
            std::size_t _index = 0;

            XRX_FORCEINLINE() static Iterator invalid(HandleVector& self)
            {
                return Iterator(&self, std::size_t(-1));
            }

            XRX_FORCEINLINE() static Iterator first_valid(HandleVector& self)
            {
                const std::size_t count = self._values.size();
                for (std::size_t index = 0; index < count; ++index)
                {
                    if (self._values[index]._version > 0)
                    {
                        return Iterator(&self, index);
                    }
                }
                return invalid(self);
            }

            XRX_FORCEINLINE() reference operator*() const
            {
                assert(_index < _self->_values.size());
                assert(_self->_values[_index]._version > 0);
                return Traits::as_reference(_index, _self->_values[_index]);
            }

            XRX_FORCEINLINE() pointer operator->() const
            {
                assert(_index < _self->_values.size());
                assert(_self->_values[_index]._version > 0);
                return Traits::as_pointer(_index, _self->_values[_index]);
            }

            // Prefix increment: ++it.
            XRX_FORCEINLINE() Iterator& operator++()
            {
                ++_index;
                for (auto count = _self->_values.size(); _index < count; ++_index)
                {
                    if (_self->_values[_index]._version > 0)
                    {
                        return *this;
                    }
                }
                // Past the end.
                _index = std::size_t(-1);
                return *this;
            }

            // Postfix increment: ++it.
            XRX_FORCEINLINE() Iterator operator++(int)
            {
                Iterator copy = *this;
                ++(*this);
                return copy;
            }

            XRX_FORCEINLINE() friend bool operator==(const Iterator& lhs, const Iterator& rhs)
            {
                return (lhs._self == rhs._self)
                    && (lhs._index  == rhs._index);
            }
            XRX_FORCEINLINE() friend bool operator!=(const Iterator& lhs, const Iterator& rhs)
            {
                return !operator==(lhs, rhs);
            }
        };

        using iterator = Iterator<ValueTraits>;

        iterator begin() &  { return iterator::first_valid(*this); }
        iterator end() &    { return iterator::invalid(*this); }
        iterator cbegin() & { return iterator::first_valid(*this); }
        iterator cend() &   { return iterator::invalid(*this); }

        struct IterateWithHandle
        {
            HandleVector* _self = nullptr;

            using iterator_ = Iterator<ValueWithHandleTraits>;

            iterator_ begin()  { return iterator_::first_valid(*_self); }
            iterator_ end()    { return iterator_::invalid(*_self); }
            iterator_ cbegin() { return iterator_::first_valid(*_self); }
            iterator_ cend()   { return iterator_::invalid(*_self); }
        };

        IterateWithHandle iterate_with_handle() &
        {
            return IterateWithHandle(this);
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
            XRX_CHECK_RVALUE(f);
            XRX_CHECK_RVALUE(state);
            XRX_CHECK_TYPE_NOT_REF(F);
            XRX_CHECK_TYPE_NOT_REF(State);
            using Action_ = ActionCallback_<F, State>;
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
                for (auto&& [action, handle] : _tick_actions.iterate_with_handle())
                {
                    const clock_duration point = (action->_last_tick + action->_period).time_since_epoch();
                    if (point < smallest)
                    {
                        smallest = point;
                        to_execute = handle;
                    }
                }
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
        XRX_CHECK_RVALUE(scheduler);
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Interval()
            , XRX_MOV(period), XRX_MOV(scheduler));
    }

    template<typename Value, typename Error = void, typename F>
    auto create(XRX_RVALUE(F&&) on_subscribe)
    {
        static_assert(not std::is_same_v<Value, void>);
        XRX_CHECK_RVALUE(on_subscribe);
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
        XRX_CHECK_RVALUE(observable1);
        XRX_CHECK_RVALUE(observable2);
        static_assert(((not std::is_lvalue_reference_v<ObservablesRest>) && ...));
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Concat()
            , XRX_MOV(observable1), XRX_MOV(observable2), XRX_MOV(observables)...);
    }

    template<typename Container>
    auto iterate(XRX_RVALUE(Container&&) values)
    {
        XRX_CHECK_RVALUE(values);
        return ::xrx::detail::make_operator(xrx::detail::operator_tag::Iterate()
            , XRX_MOV(values));
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
            return OnNextAction{._stop = state._stop};
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
            return OnNextAction{ ._stop = state._stop };
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

    template<typename Observer_, bool InvokeComplete = true>
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

            if constexpr (InvokeComplete)
            {
                on_completed_optional(XRX_MOV(*_observer));
            }
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
                on_error_optional(XRX_MOV(*_observer), XRX_MOV(e)...);
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
                XRX_CHECK_RVALUE(o);
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
            XRX_CHECK_RVALUE(o);
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
            XRX_CHECK_RVALUE(o);
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
// #include "utils_defines.h"

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
        using DetachHandle = typename SourceObservable::DetachHandle;
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
            XRX_CHECK_RVALUE(observer);
            // #XXX: we can require nothing if is_async == false, I guess.
            static_assert(std::is_move_constructible_v<Observer>);
            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        Observable_&& fork() &&      { return XRX_MOV(*this); }
        Observable_   fork() &       { return Observable_(_source.fork()); }
        Observable_&& fork_move() && { return XRX_MOV(*this); }
        Observable_   fork_move() &  { return XRX_MOV(*this); }

        template<typename F, typename... Fs>
        XRX_FORCEINLINE() auto pipe(XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs) &&;

        template<typename Scheduler>
        auto subscribe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            XRX_CHECK_RVALUE(scheduler);
            return make_operator(detail::operator_tag::SubscribeOn()
                , XRX_MOV(*this), XRX_MOV(scheduler));
        }

        template<typename Scheduler>
        auto observe_on(XRX_RVALUE(Scheduler&&) scheduler) &&
        {
            XRX_CHECK_RVALUE(scheduler);
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
        template<typename Observer>
        auto tap(Observer&& observer) &&
        {
            return make_operator(detail::operator_tag::TapOrDo()
                , XRX_MOV(*this), XRX_FWD(observer));
        }
        template<typename Observer> // same as tap().
        auto do_(Observer&& observer) &&
        {
            return make_operator(detail::operator_tag::TapOrDo()
                , XRX_MOV(*this), XRX_FWD(observer));
        }
        template<typename OpeningsObservable, typename CloseObservableProducer>
        auto window_toggle(OpeningsObservable&& openings, CloseObservableProducer&& close_producer) &&
        {
            return make_operator(detail::operator_tag::WindowToggle()
                , XRX_MOV(*this), XRX_FWD(openings), XRX_FWD(close_producer));
        }
        template<typename V, typename... Vs>
        auto starts_with(V&& v0, Vs&&... vs)
        {
            return ::xrx::detail::make_operator(xrx::detail::operator_tag::StartsWith()
                , XRX_MOV(*this), XRX_FWD(v0), XRX_FWD(vs)...);
        }
    };
} // namespace xrx::detail

namespace xrx
{
    template<typename Observer>
    inline auto subscribe(XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(observer);
        return [_observer = XRX_MOV(observer)](XRX_RVALUE(auto&&) source) mutable
        {
            return XRX_MOV(source).subscribe(XRX_MOV(_observer));
        };
    }
} // namespace xrx

template<typename SourceObservable, typename PipeConnect>
auto operator|(XRX_RVALUE(::xrx::detail::Observable_<SourceObservable>&&) source_rvalue, XRX_RVALUE(PipeConnect&&) connect)
    requires requires
    {
        // { XRX_MOV(connect)(XRX_MOV(source_rvalue)) } -> ::xrx::detail::ConceptObservable;
        XRX_MOV(connect)(XRX_MOV(source_rvalue));
    }
{
    XRX_CHECK_RVALUE(source_rvalue);
    XRX_CHECK_RVALUE(connect);
    return XRX_MOV(connect)(XRX_MOV(source_rvalue));
}

namespace xrx::detail
{
    struct PipeFold_
    {
        template<typename O>
        XRX_FORCEINLINE() auto operator()(XRX_RVALUE(O&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return XRX_MOV(source);
        }
        template<typename O, typename F, typename... Fs>
        XRX_FORCEINLINE() auto operator()(XRX_RVALUE(O&&) source, XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs)
        {
            XRX_CHECK_RVALUE(source);
            XRX_CHECK_RVALUE(f);
            return (*this)(XRX_MOV(f)(XRX_MOV(source)), XRX_MOV(fs)...);
        }
    };
} // namespace xrx::detail

namespace xrx
{
    template<typename F, typename... Fs>
    XRX_FORCEINLINE() auto pipe(XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs)
    {
        using PipeFold = ::xrx::detail::PipeFold_;
        return [_f = XRX_MOV(f), ..._fs = XRX_MOV(fs)](XRX_RVALUE(auto&&) source) mutable XRX_FORCEINLINE_LAMBDA()
        {
            return PipeFold()(XRX_MOV(_f)(XRX_MOV(source)), XRX_MOV(_fs)...);
        };
    }

    template<typename SourceObservable, typename F, typename... Fs>
    XRX_FORCEINLINE() auto pipe(XRX_RVALUE(::xrx::detail::Observable_<SourceObservable>&&) source
        , XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs)
    {
        using PipeFold = ::xrx::detail::PipeFold_;
        return PipeFold()(XRX_MOV(f)(XRX_MOV(source)), XRX_MOV(fs)...);
    }
} // namespace xrx

namespace xrx::detail
{
    template<typename SourceObservable>
    template<typename F, typename... Fs>
    XRX_FORCEINLINE() auto Observable_<SourceObservable>::pipe(XRX_RVALUE(F&&) f, XRX_RVALUE(Fs&&)... fs) &&
    {
        return ::xrx::pipe(XRX_MOV(*this), XRX_MOV(f), XRX_MOV(fs)...);
    }
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
            using SourceDetach = typename SourceObservable::DetachHandle;

            struct SubscribeInProgress
            {
                std::int32_t _waiting_unsubsribers = 0;
                std::mutex _mutex;
                std::condition_variable _on_finish;
                std::optional<SourceDetach> _detach;
            };

            struct SubscribeEnded
            {
                SourceDetach _detach;
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
                    SourceDetach detach = XRX_MOV(source_).subscribe(XRX_MOV(observer_));

                    if (in_progress)
                    {
                        assert(shared);
                        const std::lock_guard lock(shared->_mutex);

                        if (shared->_try_cancel_subscribe)
                        {
                            // Racy unsubscribe. Detach now, but also process & resume everyone waiting.
                            detach();
                            detach = {};
                        }

                        if (in_progress->_waiting_unsubsribers > 0)
                        {
                            // We were too late. Can't destroy `SubscribeInProgress`.
                            const std::lock_guard lock2(in_progress->_mutex);
                            // Set the data everyone is waiting for.
                            in_progress->_detach.emplace(XRX_MOV(detach));
                            in_progress->_on_finish.notify_all();
                        }
                        else
                        {
                            // No-one is waiting yet. We can destroy temporary in-progress data
                            // and simply put final Unsubscriber.
                            Subscribed* subscribed = std::get_if<Subscribed>(&shared->_unsubscribers);
                            assert(subscribed && "No-one should change Subscribed state when it was already set");
                            subscribed->_state.template emplace<SubscribeEnded>(XRX_MOV(detach));
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
                    assert(false && "Unexpected. Calling DetachHandle() without valid subscription state");
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

                    std::optional<SourceDetach> detach;
                    {
                        std::unique_lock wait_lock(in_progress->_mutex);
                        in_progress->_on_finish.wait(wait_lock, [in_progress]()
                        {
                            return !!in_progress->_detach;
                        });
                        // Don't move unsubscriber, there may be multiple unsubscribers waiting.
                        detach = in_progress->_detach;
                    }
                    if (detach)
                    {
                        return (*detach)();
                    }
                    return false;
                }
                else if (auto* ended = std::get_if<SubscribeEnded>(&subscribed->_state); ended)
                {
                    return ended->_detach();
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

            struct Detach
            {
                using has_effect = std::true_type;

                std::shared_ptr<StateShared_> _shared;

                bool operator()()
                {
                    auto shared = std::exchange(_shared, {});
                    if (shared)
                    {
                        return shared->detach_impl();
                    }
                    return false;
                }
            };
            using DetachHandle = Detach;

            SourceObservable _source;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                auto shared = StateShared_::make(XRX_MOV(_scheduler));
#if (0)
                // #TODO: implement subscribe_impl(..., std::false_type);
                using remember_source = typename StateShared_::SourceDetach::has_effect;
#else
                using remember_source = std::true_type;
#endif
                shared->subscribe_impl(XRX_MOV(_source), XRX_MOV(observer)
                    , remember_source());
                return DetachHandle(shared);
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
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(scheduler);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Scheduler);
        using Impl = ::xrx::observable::detail::SubscribeOnObservable_<SourceObservable, Scheduler>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(scheduler)));
    }
} // namespace xrx::detail::operator_tag

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

            struct Detach
            {
                using SourceDetach = typename SourceObservable::DetachHandle;
                using has_effect = std::true_type;

                SourceDetach _detach;
                // #TODO: does shared_ptr needed to be there.
                // Looks like weak should work in this case.
                std::shared_ptr<std::atomic_bool> _unsubscribed;

                bool operator()()
                {
                    auto unsubscribed = std::exchange(_unsubscribed, {});
                    if (unsubscribed)
                    {
                        *unsubscribed = true;
                        return std::exchange(_detach, {})();
                    }
                    return false;
                }
            };
            using DetachHandle = Detach;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                using ObserverImpl_ = ObserveOnObserver_<value_type, error_type, Scheduler, Observer>;
                auto shared = ObserverImpl_::make_state(XRX_MOV(_scheduler), XRX_MOV(observer));
                
                // #XXX: here, in theory, if `_source` Observable is Sync.
                // AND we know amount of items it can emit (not infinity),
                // we can cache all of them and then scheduler only _few_
                // tasks, each of which will emit several items per task.
                // (Instead of scheduling task per item).
                auto handle = XRX_MOV(_source).subscribe(ObserverImpl_(shared));

                DetachHandle detach;
                detach._detach = handle;
                // Share only stop flag with unsubscriber.
                detach._unsubscribed = {shared, &shared->_unsubscribed};
                return detach;
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
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(scheduler);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Scheduler);
        using Impl = ::xrx::observable::detail::ObserveOnObservable_<SourceObservable, Scheduler>;
        // #TODO: add overload that directly accepts `StreamScheduler` so
        // client can pass more narrow interface.
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(scheduler).stream_scheduler()));
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

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async   = IsAsyncObservable<SourceObservable>;
        using DetachHandle     = typename SourceObservable::DetachHandle;

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
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                (void)::xrx::detail::on_completed_optional(XRX_MOV(observer));
                return DetachHandle();
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
    inline auto take(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Take()
                , XRX_MOV(source), std::size_t(_count));
        };
    }
} // namespace xrx

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

            struct Detach
            {
                using has_effect = std::true_type;

                Handle _handle;
                Scheduler _scheduler;

                bool operator()()
                {
                    auto handle = std::exchange(_handle, {});
                    if (handle)
                    {
                        return _scheduler.tick_cancel(handle);
                    }
                    return false;
                }
            };
            using DetachHandle = Detach;

            Duration _period;
            Scheduler _scheduler;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                XRX_CHECK_TYPE_NOT_REF(Observer);

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
                    const auto action = ::xrx::detail::on_next_with_action(state._observer, value_type(now));
                    return action._stop;
                }
                    , State(XRX_MOV(observer), value_type(0)));

                return DetachHandle(handle, XRX_MOV(_scheduler));
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
        XRX_CHECK_RVALUE(scheduler);
        XRX_CHECK_TYPE_NOT_REF(Scheduler);
        using Impl = ::xrx::observable::detail::IntervalObservable_<Duration, Scheduler>;
        return Observable_<Impl>(Impl(XRX_MOV(period), XRX_MOV(scheduler)));
    }
} // namespace xrx::detail::operator_tag

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
        using DetachHandle = typename SourceObservable::DetachHandle;

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
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using TransformObserver = TransformObserver<Observer>;
            return XRX_MOV(_source).subscribe(TransformObserver(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_transform)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Transform
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) transform)
            requires requires(typename SourceObservable::value_type v) { transform(v); }
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(transform);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(F);
        using Impl = TransformObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(transform)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename F>
    auto transform(XRX_RVALUE(F&&) transform_)
    {
        XRX_CHECK_RVALUE(transform_);
        return [_transform = XRX_MOV(transform_)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Transform()
                , XRX_MOV(source), XRX_MOV(_transform));
        };
    }
} // namespace xrx

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
        using DetachHandle = NoopDetach;

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
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            constexpr std::bool_constant<Endless> _edless;
            Integer current = _first;
            while (compare_(current, _last, _step, _edless))
            {
                const OnNextAction action = on_next_with_action(observer, Integer(current));
                if (action._stop)
                {
                    return DetachHandle();
                }
                current = do_step_(current, _step);
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return DetachHandle();
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

// Header: operators/operator_tap_do.h.

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
    template<typename SourceObservable, typename Listener>
    struct TapOrDoObservable
    {
        SourceObservable _source;
        Listener _listener;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async   = IsAsyncObservable<SourceObservable>;
        using DetachHandle     = typename SourceObservable::DetachHandle;

        TapOrDoObservable fork() && { return TapOrDoObservable(XRX_MOV(_source), XRX_MOV(_listener)); }
        TapOrDoObservable fork()  & { return TapOrDoObservable(_source.fork(), _listener); }

        template<typename T>
        using MaybeRef_ = MaybeRef<T, is_async::value>;

        template<typename Observer>
        struct ListenerObserver
        {
            MaybeRef_<Observer> _observer;
            MaybeRef_<Listener> _listener;

            XRX_FORCEINLINE() auto on_next(XRX_RVALUE(value_type&&) v)
            {
                (void)::xrx::detail::on_next(_listener.get(), value_type(v)); // copy.
                return on_next_with_action(_observer.get(), XRX_MOV(v));
            }

            XRX_FORCEINLINE() auto on_completed()
            {
                (void)on_completed_optional(XRX_MOV(_listener.get()));
                return on_completed_optional(XRX_MOV(_observer.get()));
            }

            template<typename... VoidOrError>
            XRX_FORCEINLINE() auto on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    (void)on_error_optional(XRX_MOV(_listener.get()));
                    return on_error_optional(XRX_MOV(_observer.get()));
                }
                else
                {
                    auto copy = [](auto e) { return XRX_MOV(e); };
                    (void)on_error_optional(XRX_MOV(_listener.get()), copy(e)...);
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            return XRX_MOV(_source).subscribe(ListenerObserver<Observer>(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_listener)));
        }
    };

    template<typename SourceObservable, typename Observer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::TapOrDo
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(observer);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Observer);
        using Impl = TapOrDoObservable<SourceObservable, Observer>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(observer)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename Observer>
    auto tap(XRX_RVALUE(Observer&&) observer)
    {
        XRX_CHECK_RVALUE(observer);
        return [_observer = XRX_MOV(observer)](XRX_RVALUE(auto&&) source) mutable
        {
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::TapOrDo()
                , XRX_MOV(source), XRX_MOV(_observer));
        };
    }
} // namespace xrx

// Header: operators/operator_iterate.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
#include <utility>
#include <type_traits>
#include <concepts>
#include <cassert>

namespace xrx::detail
{
    template<typename Container>
    struct IterateObservable
    {
        using value_type = std::remove_reference_t<decltype(*std::begin(std::declval<Container>()))>;
        using error_type = none_tag;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;

        Container _vs;

        template<typename Observer>
        DetachHandle subscribe(Observer&& observer) &&
        {
            for (auto& v : _vs)
            {
                const auto action = on_next_with_action(observer, XRX_MOV(v));
                if (action._stop)
                {
                    return DetachHandle();
                }
            }
            (void)on_completed_optional(XRX_FWD(observer));
            return DetachHandle();
        }

        IterateObservable fork() && { return IterateObservable(XRX_MOV(_vs)); }
        IterateObservable fork() &  { return IterateObservable(_vs); }
    };

    template<typename Container>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Iterate
        , XRX_RVALUE(Container&&) values)
    {
        XRX_CHECK_RVALUE(values);
        XRX_CHECK_TYPE_NOT_REF(Container);
        using Iterator = decltype(std::begin(values));
        static_assert(std::forward_iterator<Iterator>);

        using Impl = IterateObservable<Container>;
        return Observable_<Impl>(Impl(XRX_MOV(values)));
    }
} // namespace xrx::detail

// Header: operators/operator_start_with.h.

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
    template<typename SourceObservable, typename Tuple>
    struct StartWithObservable
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

        SourceObservable _source;
        Tuple _tuple;

        template<typename Observer>
        DetachHandle subscribe(Observer&& observer) &&
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
            if (not all_processed)
            {
                return DetachHandle();
            }

            return XRX_MOV(_source).subscribe(XRX_MOV(observer));
        }

        StartWithObservable fork() && { return FromObservable(XRX_MOV(_source), XRX_MOV(_tuple)); }
        StartWithObservable fork() &  { return FromObservable(_source.fork(), _tuple); }
    };

    template<typename SourceObservable, typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::StartsWith
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(V&&) v0, XRX_RVALUE(Vs&&)... vs)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(v0);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(V);
        static_assert(((not std::is_reference_v<Vs>) && ...)
            , ".start_with(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<V, Vs...>;
        // #XXX: finalize this.
#if (0)
        static_assert(std::is_same_v<typename std::tuple_element<0, Tuple>::type
            , typename SourceObservable::value_type>);
#endif

        using Impl = StartWithObservable<SourceObservable, Tuple>;
        return Observable_<Impl>(Impl(XRX_MOV(source), Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail

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
        using DetachHandle = NoopDetach;

        Tuple _tuple;

        template<typename Observer>
        DetachHandle subscribe(Observer&& observer) &&
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
            return DetachHandle();
        }

        FromObservable fork() && { return FromObservable(XRX_MOV(_tuple)); }
        FromObservable fork() &  { return FromObservable(_tuple); }
    };

    template<typename V, typename... Vs>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::From
        , XRX_RVALUE(V&&) v0, XRX_RVALUE(Vs&&)... vs)
    {
        XRX_CHECK_RVALUE(v0);
        XRX_CHECK_TYPE_NOT_REF(V);
        static_assert(((not std::is_lvalue_reference_v<Vs>) && ...)
            , ".from(Vs...) requires owns passed values.");
        static_assert(((not std::is_reference_v<Vs>) && ...)
            , ".from(Vs...) requires Vs to be value-like type.");

        using Tuple = std::tuple<V, Vs...>;
        using Impl = FromObservable<Tuple>;
        return Observable_<Impl>(Impl(Tuple(XRX_MOV(v0), XRX_MOV(vs)...)));
    }
} // namespace xrx::detail

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
        using value_type   = typename SourceObservable::value_type;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

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
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        FilterObservable fork() && { return FilterObservable(XRX_MOV(_source), XRX_MOV(_filter)); }
        FilterObservable fork() &  { return FilterObservable(_source.fork(), _filter); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        auto subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            return XRX_MOV(_source).subscribe(FilterObserver<Observer>(
                XRX_MOV_IF_ASYNC(observer), XRX_MOV_IF_ASYNC(_filter)));
        }
    };

    template<typename SourceObservable, typename F>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Filter
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(F&&) filter)
            requires ConceptObservable<SourceObservable>
                && std::predicate<F, typename SourceObservable::value_type>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(filter);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(F);
        using Impl = FilterObservable<SourceObservable, F>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(filter)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename F>
    auto filter(XRX_RVALUE(F&&) f)
    {
        XRX_CHECK_RVALUE(f);
        return [_f = XRX_MOV(f)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Filter()
                , XRX_MOV(source), XRX_MOV(_f));
        };
    }
} // namespace xrx

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
        using DetachHandle = NoopDetach;

        using Storage = SmallVector<value_type, 1>;
        Storage _values;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
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
            return NoopDetach();
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
        using DetachHandle = NoopDetach;

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
                    on_error_optional(XRX_MOV(*_observer), XRX_MOV(e)...);
                }
                _state->_end_with_error = true;
            }
        };

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            if (_count == 0)
            {
                // It's unknown how many observables we should emit, just end the sequence.
                (void)on_completed_optional(XRX_MOV(observer));
                return DetachHandle();
            }
            State_ state;
            auto detach = XRX_FWD(_source).subscribe(
                ObserverImpl_<Observer>(&state, &observer, _count, Storage()));
            static_assert(not decltype(detach)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return DetachHandle();
        }

        WindowProducerObservable fork() && { return WindowProducerObservable(XRX_MOV(_source), _count); };
        WindowProducerObservable fork() &  { return WindowProducerObservable(_source.fork(), _count); };
    };

    template<typename SourceObservable>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Window
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count)
            requires ConceptObservable<SourceObservable>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        using Impl = WindowProducerObservable<SourceObservable
            , IsAsyncObservable<SourceObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), count));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto window(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Window()
                , XRX_MOV(source), std::size_t(_count));
        };
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
        using source_type = typename SourceObservable::value_type;
        using value_type   = Value;
        using error_type   = typename SourceObservable::error_type;
        using is_async     = IsAsyncObservable<SourceObservable>;
        using DetachHandle = typename SourceObservable::DetachHandle;

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
                    return on_error_optional(XRX_MOV(_observer.get()), XRX_MOV(e)...);
                }
            }
        };

        ReduceObservable fork() && { return ReduceObservable(XRX_MOV(_source), XRX_MOV(_op)); }
        ReduceObservable fork() &  { return ReduceObservable(_source.fork(), _op); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        decltype(auto) subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using ReduceObserver_ = ReduceObserver<Observer>;
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
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(value);
        XRX_CHECK_RVALUE(op);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Value);
        XRX_CHECK_TYPE_NOT_REF(F);
        using Impl = ReduceObservable<SourceObservable, Value, F>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(value), XRX_MOV(op)));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename Value, typename F>
    auto reduce(XRX_RVALUE(Value&&) initial, XRX_RVALUE(F&&) op)
    {
        XRX_CHECK_RVALUE(initial);
        XRX_CHECK_RVALUE(op);
        return [_value = XRX_MOV(initial), _f = XRX_MOV(op)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Reduce()
                , XRX_MOV(source), XRX_MOV(_value), XRX_MOV(_f));
        };
    }
} // namespace xrx

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
        InnerValue operator()(XRX_RVALUE(SourceValue&&) main_value, XRX_RVALUE(InnerValue&&) inner_value) const
        {
            XRX_CHECK_RVALUE(main_value);
            XRX_CHECK_RVALUE(inner_value);
            return XRX_MOV(inner_value);
        }
    };

    template<typename E1, typename E2>
    struct MergedErrors
    {
        using are_compatible = std::is_same<E1, E2>;
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
            XRX_CHECK_RVALUE(value);
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
            }
            _state->_end_with_error = true;
        }
    };

    template<typename ProducedValue, typename Observer, typename Observable, typename Map, typename SourceValue>
    XRX_FORCEINLINE() static bool invoke_inner_sync_sync(Observer& destination_
        , XRX_RVALUE(Observable&&) inner
        , Map& map
        , XRX_RVALUE(SourceValue&&) source_value)
    {
        XRX_CHECK_RVALUE(inner);
        XRX_CHECK_RVALUE(source_value);
        using InnerSync_ = InnerObserver_Sync_Sync<Observer, ProducedValue, Map, SourceValue>;
        State_Sync_Sync state;
        auto detach = XRX_MOV(inner).subscribe(
            InnerSync_(&destination_, &state, &map, &source_value));
        static_assert(not decltype(detach)::has_effect::value
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
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
        using DetachHandle = NoopDetach;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using Root_ = OuterObserver_Sync_Sync<Observer, Produce, Map, source_type, produce_type>;
            State_Sync_Sync state;
            auto detach = XRX_MOV(_source).subscribe(
                Root_(&_produce, &_map, &destination_, &state));
            static_assert(not decltype(detach)::has_effect::value
                , "Sync Observable should not have unsubscribe.");
            const bool stop = (state._unsubscribed || state._end_with_error);
            (void)stop;
            assert((state._completed || stop)
                && "Sync Observable should be ended after .subscribe() return.");
            return DetachHandle();
        }
    };

    // The state of single subscription.
    template<typename SourceValue, typename Observable>
    struct ChildObservableState_Sync_Async
    {
        using DetachHandle = typename Observable::DetachHandle;

        SourceValue _source_value;
        std::optional<Observable> _observable;
        std::optional<DetachHandle> _detach;
    };

    // State for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState_Sync_Async
    {
        using Child_ = ChildObservableState_Sync_Async<SourceValue, Observable>;
        std::vector<Child_> _children;
        bool _unsubscribe = false;
#if (0)
        using Mutex = debug::AssertMutex<>;
#else
        using Mutex = std::mutex;
#endif
        Mutex* _mutex = nullptr;
    };

    template<typename Observable, typename SourceValue>
    struct Detach_Sync_Async
    {
        using has_effect = std::true_type;

        using AllObservables_ = AllObservablesState_Sync_Async<Observable, SourceValue>;
        std::shared_ptr<AllObservables_> _shared;

        bool operator()()
        {
            // Note: not thread-safe.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                return false;
            }
            assert(shared->_mutex);
            auto guard = std::lock_guard(*shared->_mutex);
            shared->_unsubscribe = true;
            bool at_least_one = false;
            for (auto& child : shared->_children)
            {
                // #XXX: invalidate unsubscribers references.
                assert(child._detach);
                const bool detached = (*child._detach)();
                at_least_one |= detached;
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
        using AllObservables = AllObservablesState_Sync_Async<Observable, SourceValue>;
        using Mutex = typename AllObservables::Mutex;
        AllObservables _observables;
        Observer _observer;
        Map _map;
        std::size_t _subscribed_count;
        std::size_t _completed_count;
        Mutex _mutex;

        explicit SharedState_Sync_Async(XRX_RVALUE(AllObservables&&) observables
            , XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Map&&) map)
                : _observables(XRX_MOV(observables))
                , _observer(XRX_MOV(observer))
                , _map(XRX_MOV(map))
                , _subscribed_count(0)
                , _completed_count(0)
                , _mutex()
        {
            _observables._mutex = &_mutex;
        }

        void unsubscribe_all(std::size_t ignore_index)
        {
            for (std::size_t i = 0, count = _observables._children.size(); i < count; ++i)
            {
                if (i == ignore_index)
                {
                    // #TODO: explain.
                    continue;
                }
                auto& child = _observables._children[i];
                assert(child._detach);
                (*child._detach)();
                // Note, we never reset _detach optional.
                // May be accessed by external Unsubscriber user holds.
            }
        }

        unsubscribe on_next(std::size_t child_index, XRX_RVALUE(InnerValue&&) inner_value)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size());
            if (_observables._unsubscribe)
            {
                // #XXX: suspicious.
                // Not needed: called in all cases when `_unsubscribe` is set to true.
                // unsubscribe_all();
                return unsubscribe(true);
            }

            auto source_value = _observables._children[child_index]._source_value;
            const auto action = on_next_with_action(_observer
                , _map(XRX_MOV(source_value), XRX_MOV(inner_value)));
            if (action._stop)
            {
                _observables._unsubscribe = true;
                unsubscribe_all(child_index);
                return unsubscribe(true);
            }
            return unsubscribe(false);
        }
        template<typename... VoidOrError>
        void on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size());
            assert(not _observables._unsubscribe);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_observer));
            }
            else
            {
                auto guard = std::lock_guard(_observables._serialize);
                on_error_optional(XRX_MOV(_observer), XRX_MOV(e)...);
            }
            _observables._unsubscribe = true;
            unsubscribe_all(child_index);
        }
        void on_completed(std::size_t child_index)
        {
            auto guard = std::lock_guard(*_observables._mutex);
            assert(child_index < _observables._children.size()); (void)child_index;
            assert(not _observables._unsubscribe);
            ++_completed_count;
            assert(_completed_count <= _subscribed_count);
            if (_completed_count == _subscribed_count)
            {
                on_completed_optional(XRX_MOV(_observer));
            }
#if (0) // #XXX: suspicious.
            auto& kill = _observables._children[child_index]._detach;
            assert(kill);
            kill->DetachHandle();
#endif
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
                return _shared->on_error(_index, XRX_MOV(e)...);
            }
        }
    };

    template<typename Observer, typename Producer, typename Observable, typename SourceValue>
    struct OuterObserver_Sync_Async
    {
        Observer* _destination = nullptr;
        AllObservablesState_Sync_Async<Observable, SourceValue>* _observables;
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
                on_error_optional(XRX_MOV(*_destination), XRX_MOV(e)...);
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

        using DetachHandle = Detach_Sync_Async<ProducedObservable, source_type>;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using Root_ = OuterObserver_Sync_Async<Observer, Produce, ProducedObservable, source_type>;
            using AllObsevables = AllObservablesState_Sync_Async<ProducedObservable, source_type>;
            using SharedState_ = SharedState_Sync_Async<Observer, Map, ProducedObservable, source_type, produce_type>;
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
                return DetachHandle();
            }

            auto shared = std::make_shared<SharedState_>(XRX_MOV(observables)
                , XRX_MOV(destination_), XRX_MOV(_map));
            shared->_subscribed_count = shared->_observables._children.size();
            std::shared_ptr<AllObsevables> unsubscriber(shared, &shared->_observables);

            for (std::size_t i = 0; i < shared->_subscribed_count; ++i)
            {
                auto& child = shared->_observables._children[i];
                assert(child._observable);
                assert(not child._detach);
                child._detach.emplace(child._observable->fork_move()
                    .subscribe(InnerObserver_(i, shared)));
                child._observable.reset();
            }

            return DetachHandle(XRX_MOV(unsubscriber));
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
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
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

        using DetachHandle = typename SourceObservable::DetachHandle;

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using OuterObserver_ = OuterObserver_Async_Sync<Observer, Produce, Map, source_type, produce_type>;
            return XRX_MOV(_source).subscribe(OuterObserver_(
                XRX_MOV(_produce), XRX_MOV(_map), XRX_MOV(destination_)));
        }
    };

    // The state of single subscription.
    template<typename Observable, typename SourceValue>
    struct ChildObservableState_Async_Async
    {
        using DetachHandle = typename Observable::DetachHandle;

        SourceValue _source_value;
        DetachHandle _detach;
    };

    // Tricky state for all subscriptions AND stop flag.
    // This is made into one struct to be able to share
    // this data with Unsubscriber returned to user.
    // 
    // `_unsubscribe` is valid if there is shared data valid.
    template<typename Observable, typename SourceValue>
    struct AllObservablesState_Async_Async
    {
        using Child_ = ChildObservableState_Async_Async<Observable, SourceValue>;
        
#if (0)
        std::mutex _mutex;
#else
        debug::AssertMutex<> _mutex;
#endif
        std::vector<Child_> _children;
        bool _unsubscribe = false;

        bool detach_all_unsafe(std::size_t ignore_index)
        {
            // Should be locked.
            // auto guard = std::lock_guard(_mutex);
            if (_unsubscribe)
            {
                return false;
            }
            _unsubscribe = true;
            for (std::size_t i = 0, count = _children.size(); i < count; ++i)
            {
                if (i == ignore_index)
                {
                    // We may be asked to unsubscribe during
                    // observable's on_next/on_completed/on_error calls.
                    // DetachHandle() may destroy observer's state, hence we
                    // skip such cases.
                    // It should be fine, since we do unsubscribe in other ways
                    // (i.e., return unsubscribe(true)).
                    continue;
                }
                Child_& child = _children[i];
                // #XXX: invalidate all unsubscribers references.
                (void)child._detach();
            }
            return true;
        }
    };

    template<typename OuterDetach, typename ObservablesState>
    struct Detach_Async_Async
    {
        OuterDetach _outer;
        std::shared_ptr<ObservablesState> _observables;

        using has_effect = std::true_type;
        
        bool operator()()
        {
            // Note: not thread-safe.
            auto observables = std::exchange(_observables, {});
            if (not observables)
            {
                return false;
            }
            const bool root_detached = std::exchange(_outer, {})();
            auto guard = std::lock_guard(observables->_mutex);
            const bool at_least_one_child = observables->detach_all_unsafe(
                std::size_t(-1)/*nothing to ignore*/);
            return (root_detached or at_least_one_child);
        }
    };

    template<typename Map
        , typename Observer
        , typename Produce
        , typename SourceValue
        , typename ChildObservable>
    struct SharedState_Async_Async
    {
        using AllObservables = AllObservablesState_Async_Async<ChildObservable, SourceValue>;
        using child_value = typename ChildObservable::value_type;

        Map _map;
        Observer _destination;
        Produce _produce;
        AllObservables _observables;
        int _subscriptions_count;

        explicit SharedState_Async_Async(XRX_RVALUE(Map&&) map, XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Produce&&) produce)
            : _map(XRX_MOV(map))
            , _destination(XRX_MOV(observer))
            , _produce(XRX_MOV(produce))
            , _observables()
            , _subscriptions_count(1) // start with subscription to source
        {
        }

        bool on_next_child(XRX_RVALUE(SourceValue&&) source_value
            , std::shared_ptr<SharedState_Async_Async> self)
        {
            using InnerObserver = InnerObserver_Sync_Async<SharedState_Async_Async
                , child_value
                , typename ChildObservable::error_type>;

            // Need to guard subscription to be sure racy complete (on_completed_source())
            // will not think there are no children anymore; also external
            // unsubscription may miss child we just construct now.
            auto guard = std::lock_guard(_observables._mutex);
            if (_observables._unsubscribe)
            {
                return true;
            }
            ++_subscriptions_count;
            const std::size_t index = _observables._children.size();
            // Now, we need to insert _before_ subscription first
            // so if there will be some inner values emitted during subscription
            // we will known about Observable.
            auto copy = source_value;
            auto& state = _observables._children.emplace_back(XRX_MOV(source_value));
            state._detach = _produce(XRX_MOV(copy)).subscribe(InnerObserver(index, XRX_MOV(self)));
            return false;
        }

        void on_completed_source(std::size_t child_index)
        {
            auto lock = std::lock_guard(_observables._mutex);
            --_subscriptions_count;
            assert(_subscriptions_count >= 0);
            if (_subscriptions_count >= 1)
            {
                // Not everyone completed yet.
                return;
            }
            _observables.detach_all_unsafe(child_index);
            on_completed_optional(XRX_MOV(_destination));
        }

        template<typename... VoidOrError>
        void on_error_source(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            auto lock = std::lock_guard(_observables._mutex);
            _observables.detach_all_unsafe(child_index);
            if constexpr ((sizeof...(e)) == 0)
            {
                on_error_optional(XRX_MOV(_destination));
            }
            else
            {
                on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
            }
        }

        template<typename... VoidOrError>
        auto on_error(std::size_t child_index, XRX_RVALUE(VoidOrError&&)... e)
        {
            (void)child_index;
            on_error_source(child_index, XRX_MOV(e)...);
        }

        auto on_completed(std::size_t child_index)
        {
            (void)child_index;
            on_completed_source(child_index);
        }

        auto on_next(std::size_t child_index, XRX_RVALUE(child_value&&) value)
        {
            auto guard = std::unique_lock(_observables._mutex);
            assert(child_index < _observables._children.size());
            auto source_value = _observables._children[child_index]._source_value;
            const auto action = on_next_with_action(
                _destination, _map(XRX_MOV(source_value), XRX_MOV(value)));
            if (action._stop)
            {
                _observables.detach_all_unsafe(child_index/*ignore*/);
                return unsubscribe(true);
            }
            return unsubscribe(false);
        }
    };

    template<typename Shared, typename SourceValue, typename Produce>
    struct OuterObserver_Async_Async
    {
        std::shared_ptr<Shared> _shared;
        Produce _produce;
        State_Sync_Sync _state;

        unsubscribe on_next(XRX_RVALUE(SourceValue&&) source_value)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _state._unsubscribed = _shared->on_next_child(
                XRX_MOV(source_value)
                , _shared);
            return unsubscribe(_state._unsubscribed);
        }
        void on_completed()
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _shared->on_completed_source(std::size_t(-1));
            _state._completed = true;
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            assert(not _state._end_with_error);
            assert(not _state._completed);
            assert(not _state._unsubscribed);
            _shared->on_error_source(std::size_t(-1), XRX_MOV(e)...);
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
        , true /*Async source Observable*/
        , true /*Async Observables produced*/>
    {
        [[no_unique_address]] SourceObservable _source;
        [[no_unique_address]] Produce _produce;
        [[no_unique_address]] Map _map;

        using source_type = typename SourceObservable::value_type;
        using source_error = typename SourceObservable::error_type;
        using produce_type = typename ProducedObservable::value_type;
        using produce_error = typename ProducedObservable::error_type;

        using OuterDetach = typename SourceObservable::DetachHandle;
        using InnerDetach = typename ProducedObservable::DetachHandle;
        using AllObservables = AllObservablesState_Async_Async<ProducedObservable, source_type>;
        using DetachHandle = Detach_Async_Async<OuterDetach, AllObservables>;

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

        FlatMapObservable fork() && { return FlatMapObservable(XRX_MOV(_source), XRX_MOV(_produce), XRX_MOV(_map)); }
        FlatMapObservable fork() &  { return FlatMapObservable(_source.fork(), _produce, _map); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) destination_) &&
        {
            XRX_CHECK_RVALUE(destination_);
            using Shared_ = SharedState_Async_Async<Map, Observer, Produce, source_type, ProducedObservable>;
            using AllObservablesRef = std::shared_ptr<typename Shared_::AllObservables>;
            using OuterObserver_ = OuterObserver_Async_Async<Shared_, source_type, Produce>;

            auto shared = std::make_shared<Shared_>(XRX_MOV(_map), XRX_MOV(destination_), XRX_MOV(_produce));
            auto observables_ref = AllObservablesRef(shared, &shared->_observables);
            auto unsubscriber = XRX_MOV(_source).subscribe(OuterObserver_(XRX_MOV(shared), XRX_MOV(_produce)));
            return DetachHandle(XRX_MOV(unsubscriber), XRX_MOV(observables_ref));
        }
    };

    template<typename SourceObservable, typename Produce, typename Map>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce, XRX_RVALUE(Map&&) map)
            requires ConceptObservable<SourceObservable>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(produce);
        XRX_CHECK_RVALUE(map);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Map);
        XRX_CHECK_TYPE_NOT_REF(Produce);
        using value_type = typename SourceObservable::value_type;
        using ProducedObservable = decltype(produce(std::declval<value_type>()));
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");
        using Impl = FlatMapObservable<
              SourceObservable
            , ProducedObservable
            , Produce
            , Map
            , IsAsyncObservable<SourceObservable>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), XRX_MOV(map)));
    }

    template<typename SourceObservable, typename Produce>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::FlatMap
        , XRX_RVALUE(SourceObservable&&) source, XRX_RVALUE(Produce&&) produce)
            requires ConceptObservable<SourceObservable>
                  && ConceptObservable<decltype(produce(std::declval<typename SourceObservable::value_type>()))>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(produce);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(Produce);
        using value_type = typename SourceObservable::value_type;
        using Map_ = FlatMapIdentity;
        using ProducedObservable = decltype(produce(std::declval<value_type>()));
        static_assert(ConceptObservable<ProducedObservable>
            , "Return value of Produce should be Observable.");
        using Impl = FlatMapObservable<
              SourceObservable
            , ProducedObservable
            , Produce
            , Map_
            , IsAsyncObservable<SourceObservable>::value
            , IsAsyncObservable<ProducedObservable>::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(produce), Map_()));
    }
} // namespace xrx::detail

namespace xrx
{
    template<typename F>
    auto flat_map(XRX_RVALUE(F&&) produce)
    {
        XRX_CHECK_RVALUE(produce);
        return [_produce = XRX_MOV(produce)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            using IdentityMap = detail::FlatMapIdentity;
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::FlatMap()
                , XRX_MOV(source), XRX_MOV(_produce), IdentityMap());
        };
    }

    template<typename F, typename Map>
    auto flat_map(XRX_RVALUE(F&&) produce, XRX_RVALUE(Map&&) map)
    {
        XRX_CHECK_RVALUE(produce);
        XRX_CHECK_RVALUE(map);
        return [_produce = XRX_MOV(produce), _map = XRX_MOV(map)](XRX_RVALUE(auto&&) source) mutable
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::FlatMap()
                , XRX_MOV(source), XRX_MOV(_produce), XRX_MOV(_map));
        };
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
    namespace detail
    {
        template<typename Detach_>
        struct SubscribeDispatch
        {
            static_assert(::xrx::detail::ConceptDetachHandle<Detach_>);
            using DetachHandle = Detach_;

            template<typename F, typename Observer>
            static auto invoke_(F&& on_subscribe, XRX_RVALUE(Observer&&) observer)
            {
                XRX_CHECK_RVALUE(observer);
                XRX_CHECK_TYPE_NOT_REF(Observer);
                return XRX_FWD(on_subscribe)(XRX_MOV(observer));
            }
        };

        template<>
        struct SubscribeDispatch<void>
        {
            using DetachHandle = ::xrx::detail::NoopDetach;

            template<typename F, typename Observer>
            static DetachHandle invoke_(F&& on_subscribe, XRX_RVALUE(Observer&&) observer)
            {
                XRX_CHECK_RVALUE(observer);
                XRX_CHECK_TYPE_NOT_REF(Observer);
                using RefObserver = ::xrx::detail::RefTrackingObserver_<Observer>;
                ::xrx::detail::RefObserverState state;
                (void)XRX_FWD(on_subscribe)(RefObserver(&observer, &state));
                assert(state.is_finalized());
                return DetachHandle();
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
            else if constexpr (::xrx::detail::ConceptDetachHandle<Return_>)
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
            using DetachHandle = typename SubscribeDispatch_::DetachHandle;

            auto fork() && { return CustomObservable_(XRX_MOV(_on_subscribe)); }
            auto fork() &  { return CustomObservable_(_on_subscribe); }

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, value_type>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                XRX_CHECK_TYPE_NOT_REF(Observer);
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
        XRX_CHECK_RVALUE(on_subscribe);
        XRX_CHECK_TYPE_NOT_REF(F);
        static_assert(not std::is_same_v<Value, void>);
        static_assert(not std::is_reference_v<Value>);
        using Impl = ::xrx::observable::detail::CustomObservable_<Value, Error, F>;
        return Observable_<Impl>(Impl(XRX_MOV(on_subscribe)));
    }
} // namespace xrx::detail::operator_tag

// Header: operators/operator_concat.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_fast_FWD.h"
// #include "utils_ref_tracking_observer.h"
#include <utility>
#include <type_traits>
#include <tuple>
#include <variant>
#include <mutex>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename Transform, typename Tuple>
    struct TupleAsVariant;

    template<typename Transform, typename... Args>
    struct TupleAsVariant<Transform, std::tuple<Args...>>
    {
        using variant_type = std::variant<
            typename Transform::template invoke_<Args>...>;
    };

    template<typename Tuple, typename F>
    static bool runtime_tuple_get(Tuple& tuple, std::size_t index, F&& f)
    {
        constexpr std::size_t Size_ = std::tuple_size_v<Tuple>;
        if (index >= Size_)
        {
            return false;
        }
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            const bool yes_ = (((Is == index)
                ? ((bool)XRX_FWD(f)(std::get<Is>(tuple)))
                : false) || ...);
            return yes_;
        }(std::make_index_sequence<Size_>());
    }

    template<std::size_t I, typename Variant, typename Value>
    static bool variant_emplace_at(Variant& variant, std::size_t index, Value&& value)
    {
        using T = std::variant_alternative_t<I, Variant>;
        if constexpr (std::is_constructible_v<T, Value&&>)
        {
            assert(index == I); (void)index;
            variant.emplace<I>(XRX_FWD(value));
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename Variant, typename Value>
    static bool runtime_variant_emplace(Variant& variant, std::size_t index, Value&& value)
    {
        constexpr std::size_t Size_ = std::variant_size_v<Variant>;
        if (index >= Size_)
        {
            return false;
        }
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            const bool yes_ = (((Is == index)
                ? variant_emplace_at<Is>(variant, index, XRX_FWD(value))
                : false) || ...);
            return yes_;
        }(std::make_index_sequence<Size_>());
    }

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
        using DetachHandle = NoopDetach;

        Tuple _tuple;

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_tuple)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_tuple); }

        template<typename Observer>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            auto invoke_ = [](auto&& observer, auto&& observable)
            {
                RefObserverState state;
                using OnePass = RefTrackingObserver_<Observer, false/*no on_completed*/>;
                auto detach = XRX_FWD(observable).subscribe(OnePass(&observer, &state));
                static_assert(not decltype(detach)::has_effect::value
                    , "Sync Observable should not have unsubscribe.");
                const bool stop = (state._unsubscribed || state._with_error);
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
            return NoopDetach();
        }
    };

    template<typename Tuple>
    struct ConcatObservable<Tuple, true/*Async*/>
    {
        static constexpr std::size_t Size_ = std::tuple_size_v<Tuple>;
        static_assert(Size_ >= 2);
        using ObservablePrototype = typename std::tuple_element<0, Tuple>::type;

        using value_type = typename ObservablePrototype::value_type;
        using error_type = typename ObservablePrototype::error_type;
        using is_async = std::true_type;

        Tuple _observables;

        struct ObservableToDetach
        {
            template<typename O>
            using invoke_ = typename O::DetachHandle;
        };

        using DetachVariant = typename TupleAsVariant<ObservableToDetach, Tuple>::variant_type;

        struct Unsubscription
        {
            std::recursive_mutex _mutex;
            DetachVariant _unsubscribers;
        };

        struct Detach
        {
            using has_effect = std::true_type;
            std::shared_ptr<Unsubscription> _unsubscription;
            bool operator()()
            {
                auto shared = std::exchange(_unsubscription, {});
                if (not shared)
                {
                    return false;
                }
                auto guard = std::lock_guard(shared->_mutex);
                auto handle = [](auto&& DetachHandle)
                {
                    return DetachHandle();
                };
                return std::visit(XRX_MOV(handle), shared->_unsubscribers);
            }
        };

        using DetachHandle = Detach;

        template<typename Observer>
        struct Shared_;

        template<typename Observer>
        struct OnePassObserver
        {
            std::shared_ptr<Shared_<Observer>> _shared;

            auto on_next(XRX_RVALUE(value_type&&) value);
            void on_completed();
            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e);
        };

        template<typename Observer>
        struct Shared_ : public std::enable_shared_from_this<Shared_<Observer>>
        {
            Observer _destination;
            Tuple _observables;
            std::size_t _cursor;
            Unsubscription _unsubscription;

            XRX_FORCEINLINE() std::recursive_mutex& mutex() { return _unsubscription._mutex; }

            explicit Shared_(XRX_RVALUE(Observer&&) observer, XRX_RVALUE(Tuple&&) observables)
                : _destination(XRX_MOV(observer))
                , _observables(XRX_MOV(observables))
                , _cursor(0)
                , _unsubscription()
            {
            }

            void start_impl(std::size_t index)
            {
                auto guard = std::lock_guard(mutex());
                assert(_cursor == index);
                const bool ok = runtime_tuple_get(_observables, index
                    , [&](auto& observable)
                {
                    using Impl = OnePassObserver<Observer>;
                    return runtime_variant_emplace(
                        _unsubscription._unsubscribers
                        , index
                        , observable.fork_move() // We move/own observable.
                            .subscribe(Impl(this->shared_from_this())));
                });
                assert(ok); (void)ok;
            }

            auto on_next(XRX_RVALUE(value_type&&) value)
            {
                auto guard = std::lock_guard(mutex());
                return on_next_with_action(_destination, XRX_MOV(value));
            }

            void on_completed()
            {
                auto guard = std::lock_guard(mutex());
                assert(_cursor < Size_);
                ++_cursor;
                if (_cursor == Size_)
                {
                    return on_completed_optional(_destination);
                }
                start_impl(_cursor);
            }

            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e)
            {
                auto guard = std::lock_guard(mutex());
                if constexpr ((sizeof...(e)) == 0)
                {
                    on_error_optional(XRX_MOV(_destination));
                }
                else
                {
                    on_error_optional(XRX_MOV(_destination), XRX_MOV(e)...);
                }
            }
        };

        ConcatObservable fork() && { return ConcatObservable(XRX_MOV(_observables)); }
        // #XXX: wrong - make a tuple with .fork() for each Observable.
        ConcatObservable fork() &  { return ConcatObservable(_observables); }

        template<typename Observer>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            auto shared = std::make_shared<Shared_<Observer>>(XRX_MOV(observer), XRX_MOV(_observables));
            shared->start_impl(0);
            std::shared_ptr<Unsubscription> unsubscription(shared, &shared->_unsubscription);
            return DetachHandle(XRX_MOV(unsubscription));
        }
    };

    template<typename Tuple>
    template<typename Observer>
    auto ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_next(XRX_RVALUE(value_type&&) value)
    {
        return _shared->on_next(XRX_MOV(value));
    }
    template<typename Tuple>
    template<typename Observer>
    void ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_completed()
    {
        return _shared->on_completed();
    }
    template<typename Tuple>
    template<typename Observer>
    template<typename... VoidOrError>
    void ConcatObservable<Tuple, true/*Async*/>::OnePassObserver<Observer>::on_error(XRX_RVALUE(VoidOrError&&)... e)
    {
        return _shared->on_error(XRX_MOV(e)...);
    }

    template<typename Observable1, typename Observable2, bool>
    struct HaveSameStreamTypes
    {
        // #XXX: there is another similar code somewhere; move to helper.
        using error1 = typename Observable1::error_type;
        using error2 = typename Observable2::error_type;

        static constexpr bool is_void_like_error1 =
               std::is_same_v<error1, void>
            or std::is_same_v<error1, none_tag>;
        static constexpr bool is_void_like_error2 =
               std::is_same_v<error2, void>
            or std::is_same_v<error2, none_tag>;
        static constexpr bool are_compatibe_errors =
               (is_void_like_error1 and is_void_like_error2)
            or std::is_same_v<error1, error2>;

        static constexpr bool value =
               std::is_same_v<typename Observable1::value_type
                            , typename Observable2::value_type>
            && are_compatibe_errors;
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
        XRX_CHECK_TYPE_NOT_REF(Observable1);
        XRX_CHECK_TYPE_NOT_REF(Observable2);
        static_assert(((not std::is_reference_v<ObservablesRest>) && ...)
            , "Expected to have non-reference types (not T& ot T&&; to be moved from). ");

        constexpr bool IsAnyAsync = false
            || ( IsAsyncObservable<Observable1>())
            || ( IsAsyncObservable<Observable2>())
            || ((IsAsyncObservable<ObservablesRest>()) || ...);

        using Tuple = std::tuple<Observable1, Observable2, ObservablesRest...>;
        using Impl = ConcatObservable<Tuple, IsAnyAsync>;
        return Observable_<Impl>(Impl(Tuple(
            XRX_MOV(observable1)
            , XRX_MOV(observable2)
            , XRX_MOV(observables)...)));
    }
} // namespace xrx::detail

// Header: operators/operator_repeat.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "observable_interface.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_ref_tracking_observer.h"
// #include "utils_fast_FWD.h"
#include <utility>
#include <mutex>
#include <type_traits>
#include <variant>
#include <cstdint>
#include <cassert>

namespace xrx::detail
{
    template<typename SourceObservable, bool Endless, bool IsSourceAsync>
    struct RepeatObservable;

    template<typename SourceObservable, bool Endless>
    struct RepeatObservable<SourceObservable, Endless, false/*Sync*/>
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = std::false_type;
        using DetachHandle = NoopDetach;
        using SourceDetach = typename SourceObservable::DetachHandle;
        static_assert(not SourceDetach::has_effect::value
            , "Sync Observable's Detach should have no effect.");

        SourceObservable _source;
        int _max_repeats;

        template<typename Observer>
        NoopDetach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using RefObserver_ = RefTrackingObserver_<Observer, false/*do not call on_complete*/>;

            auto check_repeat = [this, index = 0]() mutable
            {
                if constexpr (Endless)
                {
                    return true;
                }
                else
                {
                    return (index++ < (_max_repeats - 1));
                }
            };
            // Iterate N - 1 times; last iteration is "optimization":
            // _source.fork_move() is used instead of _source.fork().
            while (check_repeat())
            {
                RefObserverState state;
                (void)_source.fork()
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopDetach();
                }
            }
            if (_max_repeats > 0)
            {
                // Last loop - move-forget source Observable.
                RefObserverState state;
                (void)_source.fork_move() // Difference.
                    .subscribe(RefObserver_(&observer, &state));
                assert(state.is_finalized()
                    && "Sync Observable is not finalized after .subscribe()'s end.");
                if (state._unsubscribed || state._with_error)
                {
                    return NoopDetach();
                }
            }
            (void)on_completed_optional(XRX_MOV(observer));
            return NoopDetach();
        }

        RepeatObservable fork() && { return RepeatObservable(XRX_MOV(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    struct RepeatObservable<SourceObservable, Endless, true/*Async*/>
    {
        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using SourceDetach = typename SourceObservable::DetachHandle;
        using is_async = std::true_type;

        SourceObservable _source;
        int _max_repeats;

        struct DetachBlock
        {
            std::recursive_mutex _mutex;
            SourceDetach _source;
            bool _do_detach = false;
        };

        struct Detach
        {
            using has_effect = std::true_type;
            using State = std::variant<std::monostate
                , SourceDetach
                , std::shared_ptr<DetachBlock>>;
            State _state;
            explicit Detach() = default;
            explicit Detach(XRX_RVALUE(SourceDetach&&) source_detach)
                : _state(XRX_MOV(source_detach))
            {}
            explicit Detach(XRX_RVALUE(std::shared_ptr<DetachBlock>&&) block)
                : _state(XRX_MOV(block))
            {}

            bool operator()()
            {
                auto state = std::exchange(_state, {});
                if (std::get_if<std::monostate>(&state))
                {
                    return false;
                }
                else if (auto* source_detach = std::get_if<SourceDetach>(&state))
                {
                    return (*source_detach)();
                }
                else if (auto* block_ptr = std::get_if<std::shared_ptr<DetachBlock>>(&state))
                {
                    DetachBlock& block = **block_ptr;
                    auto guard = std::lock_guard(block._mutex);
                    block._do_detach = true;
                    return block._source();
                }
                assert(false);
                return false;
            }
        };

        using DetachHandle = Detach;

        template<typename Observer>
        struct Shared_
        {
            Observer _observer;
            SourceObservable _source;
            DetachBlock _detach;
            int _max_repeats;
            int _repeats;
            explicit Shared_(XRX_RVALUE(Observer&&) observer, XRX_RVALUE(SourceObservable&&) source, int max_repeats)
                : _observer(XRX_MOV(observer))
                , _source(XRX_MOV(source))
                , _detach()
                , _max_repeats(max_repeats)
                , _repeats(0)
            {
            }
            std::recursive_mutex& mutex()
            {
                return _detach._mutex;
            }

            struct OneRepeatObserver
            {
                std::shared_ptr<Shared_> _shared;

                auto on_next(value_type&& v)
                {
                    auto guard = std::lock_guard(_shared->mutex());
                    return on_next_with_action(_shared->_observer, XRX_FWD(v));
                }

                XRX_FORCEINLINE() void on_completed()
                {
                    auto self = _shared;
                    self->try_subscribe_next(XRX_MOV(_shared));
                }

                template<typename... VoidOrError>
                void on_error(XRX_RVALUE(VoidOrError&&)... e)
                {
                    auto guard = std::lock_guard(_shared->mutex());
                    if constexpr ((sizeof...(e)) == 0)
                    {
                        on_error_optional(XRX_MOV(_shared->_observer));
                    }
                    else
                    {
                        on_error_optional(XRX_MOV(_shared->_observer), XRX_MOV(e)...);
                    }
                }
            };

            static void try_subscribe_next(std::shared_ptr<Shared_>&& self)
            {
                auto guard = std::lock_guard(self->mutex());
                if (self->_detach._do_detach)
                {
                    // Just unsubscribed.
                    return;
                }

                auto repeat_copy = [&]()
                {
                    auto copy = self;
                    // Repeat-on-the-middle, copy source.
                    self->_detach._source = self->_source.fork()
                        .subscribe(OneRepeatObserver(XRX_MOV(copy)));
                };

                if constexpr (Endless)
                {
                    repeat_copy();
                }
                else
                {
                    ++self->_repeats;
                    if (self->_repeats > self->_max_repeats)
                    {
                        // Done.
                        (void)on_completed_optional(XRX_MOV(self->_observer));
                        return;
                    }
                    if (self->_repeats == self->_max_repeats)
                    {
                        // Last repeat, move-destroy source.
                        auto copy = self;
                        self->_detach._source = self->_source.fork_move()
                            .subscribe(OneRepeatObserver(XRX_MOV(copy)));
                    }
                    else
                    {
                        assert(self->_repeats < self->_max_repeats);
                        repeat_copy();
                    }
                }
            }
        };

        template<typename Observer>
        Detach subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            XRX_CHECK_RVALUE(observer);
            XRX_CHECK_TYPE_NOT_REF(Observer);
            using Shared = Shared_<Observer>;
            if (not Endless)
            {
                if (_max_repeats == 0)
                {
                    (void)on_completed_optional(XRX_MOV(observer));
                    return Detach();
                }
                if (_max_repeats == 1)
                {
                    auto source_detach = XRX_MOV(_source).subscribe(XRX_MOV(observer));
                    return Detach(XRX_MOV(source_detach));
                }
            }
            // Endless or with repeats >= 2. Need to create shared state.
            auto shared = std::make_shared<Shared>(XRX_MOV(observer), XRX_MOV(_source), _max_repeats);
            std::shared_ptr<DetachBlock> DetachHandle(shared, &shared->_detach);
            auto self = shared;
            shared->try_subscribe_next(XRX_MOV(self));
            return Detach(XRX_MOV(DetachHandle));
        }

        RepeatObservable fork() && { return RepeatObservable(XRX_MOV(_source), _max_repeats); }
        RepeatObservable fork() &  { return RepeatObservable(_source.fork(), _max_repeats); }
    };

    template<typename SourceObservable, bool Endless>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::Repeat
        , XRX_RVALUE(SourceObservable&&) source, std::size_t count, std::bool_constant<Endless>)
            requires ConceptObservable<SourceObservable>
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        using IsAsync_ = IsAsyncObservable<SourceObservable>;
        using Impl = RepeatObservable<SourceObservable, Endless, IsAsync_::value>;
        return Observable_<Impl>(Impl(XRX_MOV(source), int(count)));
    }
} // namespace xrx::detail

namespace xrx
{
    inline auto repeat()
    {
        return [](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Repeat()
                , XRX_MOV(source), 0, std::true_type());
        };
    }

    inline auto repeat(std::size_t count)
    {
        return [_count = count](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Repeat()
                , XRX_MOV(source), _count, std::false_type());
        };
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
#include <mutex>
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
        using SourceDetach = typename SourceObservable::DetachHandle;

        struct SharedImpl_;

        struct ConnectObserver_
        {
            std::shared_ptr<SharedImpl_> _shared;
            bool _do_refcount = false;

            void on_next(value_type&& v);
            template<typename... VoidOrError>
            void on_error(XRX_RVALUE(VoidOrError&&)... e);
            void on_completed();
        };

        struct Detach_Child
        {
            using has_effect = std::true_type;

            std::shared_ptr<SharedImpl_> _shared;
            Handle _handle;

            bool operator()(bool do_refcount = false);
        };

        struct Detach_Connect
        {
            using has_effect = std::true_type;

            std::shared_ptr<SharedImpl_> _shared;

            bool operator()();

            explicit operator bool() const
            {
                return !!_shared;
            }
        };

        struct Detach_RefCount
        {
            Detach_Child _child;

            bool operator()()
            {
                return std::exchange(_child, {})(true/*do_refcount*/);
            }
        };

        struct RefCountObservable_
        {
            using value_type = typename SourceObservable::value_type;
            using error_type = typename SourceObservable::error_type;
            using is_async = IsAsyncObservable<SourceObservable>;
            using DetachHandle = Detach_RefCount;

            std::shared_ptr<SharedImpl_> _shared;

            template<typename Observer>
            auto subscribe(Observer&& observer) &&;

            auto fork() && { return RefCountObservable_(XRX_MOV(_shared)); }
            auto fork()  & { return RefCountObservable_(_shared); }
        };

        struct SharedImpl_ : public std::enable_shared_from_this<SharedImpl_>
        {
            std::recursive_mutex _mutex;
            SourceObservable _source;
            Subscriptions _subscriptions;
            std::optional<SourceDetach> _connected;

            explicit SharedImpl_(XRX_RVALUE(SourceObservable&&) source)
                : _mutex()
                , _source(XRX_MOV(source))
                , _subscriptions()
                , _connected(std::nullopt)
            {
            }

            Detach_Connect connect_once(bool do_refcount)
            {
                auto guard = std::unique_lock(_mutex);
                if (not _connected)
                {
                    auto self = this->shared_from_this();
                    _connected.emplace(_source.fork().subscribe(ConnectObserver_(self, do_refcount)));
                    return Detach_Connect(XRX_MOV(self));
                }
                return Detach_Connect();
            }

            bool disconnect_once()
            {
                auto guard = std::unique_lock(_mutex);
                if (_connected)
                {
                    return (*std::exchange(_connected, {}))();
                }
                return false;
            }

            template<typename Observer>
            Detach_Child subscribe_child(XRX_RVALUE(Observer&&) observer, bool do_refcount)
            {
                XRX_CHECK_RVALUE(observer);
                AnyObserver_ erased_observer(XRX_MOV(observer));
                auto guard = std::lock_guard(_mutex);
                std::size_t count_before = _subscriptions.size();
                auto handle = _subscriptions.push_back(XRX_MOV(erased_observer));
                if (do_refcount && (count_before == 0))
                {
                    (void)connect_once(do_refcount);
                }
                return Detach_Child(this->shared_from_this(), XRX_MOV(handle));
            }

            bool unsubscribe_child(Handle handle, bool do_refcount)
            {
                auto guard = std::lock_guard(_mutex);
                const std::size_t count_before = _subscriptions.size();
                const bool ok = _subscriptions.erase(handle);
                const std::size_t count_now = _subscriptions.size();
                if (do_refcount && (count_now == 0) && (count_before > 0))
                {
                    (void)disconnect_once();
                }
                return ok;
            }

            void on_next_impl(XRX_RVALUE(value_type&&) v, bool do_refcount)
            {
                auto guard = std::unique_lock(_mutex);
                for (auto&& [observer, handle] : _subscriptions.iterate_with_handle())
                {
                    const auto action = observer.on_next(value_type(v)); // copy.
                    if (action._stop)
                    {
                        // #XXX: when refcount is active AND child is last one,
                        // this will trigger disconnect_once() that will trigger
                        // our Observer's DetachHandle; that is self-destroy ?
                        // Just increase self-shared_ptr ref-count ?
                        unsubscribe_child(handle, do_refcount);
                    }
                }
            }
            template<typename... VoidOrError>
            void on_error_impl(XRX_RVALUE(VoidOrError&&)... e)
            {
                auto guard = std::unique_lock(_mutex);
                for (AnyObserver_& observer : _subscriptions)
                {
                    if constexpr ((sizeof...(e)) == 0)
                    {
                        observer.on_error();
                    }
                    else
                    {
                        observer.on_error(error_type(e)...); // copy error.
                    }
                }
            }
            void on_completed_impl()
            {
                auto guard = std::unique_lock(_mutex);
                for (AnyObserver_& observer : _subscriptions)
                {
                    observer.on_completed();
                }
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
        using Detach_Child = typename State_::Detach_Child;
        using DetachHandle = Detach_Child;

        using value_type = typename SourceObservable::value_type;
        using error_type = typename SourceObservable::error_type;
        using is_async = IsAsyncObservable<SourceObservable>;

        std::weak_ptr<SharedImpl_> _shared_weak;

        template<typename Observer>
        Detach_Child subscribe(XRX_RVALUE(Observer&&) observer)
        {
            XRX_CHECK_RVALUE(observer);
            if (auto shared = _shared_weak.lock(); shared)
            {
                return shared->subscribe_child(XRX_MOV(observer), false/*no refcount*/);
            }
            return Detach_Child();
        }

        ConnectObservableSource_ fork() { return ConnectObservableSource_(_shared_weak); }
    };

    template<typename SourceObservable
        , typename SharedState_ = ConnectObservableState_<SourceObservable>
        , typename ConnectObservableSource = ConnectObservableSource_<SourceObservable>
        , typename ConnectObservableInterface = Observable_<ConnectObservableSource>>
    struct ConnectObservable_
        : private SharedState_
        , public ConnectObservableInterface
    {
        using value_type = typename ConnectObservableInterface::value_type;
        using error_type = typename ConnectObservableInterface::error_type;
        using SharedImpl_ = typename SharedState_::SharedImpl_;
        using RefCountObservable_ = typename SharedState_::RefCountObservable_;
        using Detach_Connect = typename SharedState_::Detach_Connect;
        using DetachHandle = Detach_Connect;
        using RefCountObservableInterface = Observable_<RefCountObservable_>;

        XRX_FORCEINLINE() SharedState_& state() { return static_cast<SharedState_&>(*this); }

        explicit ConnectObservable_(XRX_RVALUE(SourceObservable&&) source)
            : SharedState_(XRX_MOV(source))
            , ConnectObservableInterface(ConnectObservableSource(state()._shared))
        {
        }

        Detach_Connect connect()
        {
            return state()._shared->connect_once(false/*no refcount*/);
        }

        RefCountObservableInterface ref_count()
        {
            return RefCountObservableInterface(RefCountObservable_(state()._shared));
        }
    };

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_next(XRX_RVALUE(value_type&&) v)
    {
        assert(_shared);
        _shared->on_next_impl(XRX_MOV(v), _do_refcount);
    }

    template<typename SourceObservable>
    template<typename... VoidOrError>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_error(XRX_RVALUE(VoidOrError&&)... e)
    {
        assert(_shared);
        _shared->on_error_impl(XRX_MOV(e)...);
    }

    template<typename SourceObservable>
    void ConnectObservableState_<SourceObservable>::ConnectObserver_::on_completed()
    {
        assert(_shared);
        _shared->on_completed_impl();
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Detach_Child::operator()(bool do_refcount /*= false*/)
    {
        auto shared = std::exchange(_shared, {});
        if (shared)
        {
            return shared->unsubscribe_child(_handle, do_refcount);
        }
        return false;
    }

    template<typename SourceObservable>
    bool ConnectObservableState_<SourceObservable>::Detach_Connect::operator()()
    {
        auto shared = std::exchange(_shared, {});
        if (shared)
        {
            return shared->disconnect_once();
        }
        return false;
    }

    template<typename SourceObservable>
    template<typename Observer>
    auto ConnectObservableState_<SourceObservable>::RefCountObservable_::subscribe(
        XRX_RVALUE(Observer&&) observer) &&
    {
        XRX_CHECK_RVALUE(observer);
        assert(_shared);
        return Detach_RefCount(_shared->subscribe_child(
            XRX_MOV(observer), true/*refcount*/));
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
    inline auto publish()
    {
        return [](XRX_RVALUE(auto&&) source)
        {
            XRX_CHECK_RVALUE(source);
            return ::xrx::detail::make_operator(::xrx::detail::operator_tag::Publish()
                , XRX_MOV(source));
        };
    }
} // namespace xrx

// Header: subject.h.

// #pragma once
// #include "any_observer.h"
// #include "utils_containers.h"
// #include "observable_interface.h"
// #include "debug/assert_mutex.h"

#include <mutex>
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
            std::mutex _mutex;
            Subscriptions _subscriptions;
        };

        using value_type = Value;
        using error_type = Error;

        struct Detach
        {
            using has_effect = std::true_type;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // Nothing to unsubscribe from.
            std::weak_ptr<SharedImpl_> _shared_weak;
            Handle _handle;

            bool operator()()
            {
                auto shared = std::exchange(_shared_weak, {}).lock();
                if (not shared)
                {
                    return false;
                }
                auto guard = std::lock_guard(shared->_mutex);
                return shared->_subscriptions.erase(_handle);
            }

            explicit operator bool() const
            {
                return (not _shared_weak.expired());
            }
        };
        using DetachHandle = Detach;

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
            using DetachHandle = Subject_::DetachHandle;

            // [weak_ptr]: if weak reference is expired, there is no actual
            // reference to Subject<> that can push/emit new items to the stream.
            // The stream is dangling; any new Observer will not get any event.
            std::weak_ptr<SharedImpl_> _shared_weak;

            template<typename Observer>
                requires ConceptValueObserverOf<Observer, Value>
            DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
            {
                XRX_CHECK_RVALUE(observer);
                auto shared = _shared_weak.lock();
                if (not shared)
                {
                    // #XXX: this is also the case when we try to
                    // subscribe on the subject that is already completed.
                    // Should we assert ? What's the expected behavior ?
                    return DetachHandle();
                }
                AnyObserver<value_type, error_type> erased(XRX_MOV(observer));
                auto guard = std::lock_guard(shared->_mutex);
                DetachHandle detach;
                detach._shared_weak = _shared_weak;
                detach._handle = shared->_subscriptions.push_back(XRX_MOV(erased));
                return detach;
            }

            auto fork() && { return SourceObservable(XRX_MOV(_shared_weak)); }
            auto fork() &  { return SourceObservable(_shared_weak); }
        };

        using Observable = ::xrx::detail::Observable_<SourceObservable>;
        using Observer = Subject_;

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, Value>
        // This subscribe() is not ref-qualified (like the rest of Observables)
        // since it doesn't make sense for Subject<> use-case:
        // once subscribed, _same_ Subject instance is used to emit values.
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer)
        {
            XRX_CHECK_RVALUE(observer);
            return as_observable().subscribe(XRX_MOV(observer));
        }

        Observable as_observable()
        {
            return Observable(SourceObservable(_shared));
        }

        Observer as_observer()
        {
            return Observer(_shared);
        }

        void on_next(XRX_RVALUE(value_type&&) v)
        {
            // Note: on_next() and {on_error(), on_completed()} should not be called in-parallel.
            if (not _shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(_shared->_mutex);
            for (auto&& [observer, handle] : _shared->_subscriptions.iterate_with_handle())
            {
                const auto action = observer.on_next(value_type(v)); // copy.
                if (action._stop)
                {
                    // Notice: we have mutex lock.
                    _shared->_subscriptions.erase(handle);
                }
            }
            // Note: should we unsubscribe if there are no observers/subscriptions anymore ?
            // Seems like nope, someone can subscribe later.
        }

        template<typename... Es>
        void on_error(Es&&... errors)
        {
            // Note: on_completed() should not be called in-parallel.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(shared->_mutex);
            for (AnyObserver<Value, Error>& observer : shared->_subscriptions)
            {
                if constexpr (sizeof...(errors) == 0)
                {
                    observer.on_error();
                }
                else
                {
                    static_assert(sizeof...(errors) == 1);
                    observer.on_error(Es(errors)...); // copy.
                }
            }
        }

        void on_completed()
        {
            // Note: on_completed() should not be called in-parallel.
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                assert(false);
                return;
            }
            auto lock = std::unique_lock(shared->_mutex);
            for (AnyObserver<Value, Error>& observer : shared->_subscriptions)
            {
                observer.on_completed();
            }
        }
    };
} // namespace xrx

// Header: operators/operator_window_toggle.h.

// #pragma once
// #include "operator_tags.h"
// #include "cpo_make_operator.h"
// #include "utils_observers.h"
// #include "utils_observable.h"
// #include "utils_wrappers.h"
// #include "utils_defines.h"
// #include "concepts_observer.h"
// #include "subject.h"
// #include "observable_interface.h"
// #include "utils_containers.h"
#include <type_traits>
#include <utility>
#include <memory>

namespace xrx::detail
{
    template<typename SourceDetach
        , typename OpeningsDetach
        , typename ClosingsDetach
        , typename WindowHandle>
    struct Unsubscribers
    {
        struct CloseState
        {
            WindowHandle _window;
            ClosingsDetach _detach;
            bool close()
            {
                return std::exchange(_detach, {})();
            }
        };

        SourceDetach _source;
        OpeningsDetach _openings;
        std::vector<CloseState> _closings;
        debug::AssertMutex<> _mutex;
    };

    template<typename Unsubscribers_
        , typename Observer
        , typename SourceValue
        , typename ErrorValue>
    struct WindowToggleShared_
    {
        using Subject = Subject_<SourceValue, ErrorValue>;
        using Windows = HandleVector<Subject>;
        using WindowHandle = typename Windows::Handle;
        Observer _observer;
        Windows _windows;
        Unsubscribers_ _unsubscribers;

        auto& mutex() { return _unsubscribers._mutex; }

        explicit WindowToggleShared_(XRX_RVALUE(Observer&&) observer)
            : _observer(XRX_MOV(observer))
            , _windows()
            , _unsubscribers()
        {
        }

        // From Openings stream.
        WindowHandle on_new_window()
        {
            // 1. We own the mutex.
            WindowHandle handle = _windows.push_back(Subject());
            Subject* subject = _windows.get(handle);
            assert(subject);
            const auto action = on_next_with_action(_observer, XRX_MOV(subject->as_observable()));
            return (action._stop ? WindowHandle() : handle);
        }
        // From Closings stream.
        bool close_window(WindowHandle handle)
        {
            // 1. We own the mutex.
            Subject* subject = _windows.get(handle);
            assert(subject);
            subject->on_completed();
            return _windows.erase(handle);
        }
        // From Source stream.
        void on_next_source(auto source_value)
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                auto copy = source_value;
                // Unsubscriptions are handled by the Subject_<> itself.
                window.on_next(XRX_MOV(copy));
            }
        }
        // From Source stream.
        void on_completed_source()
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                window.on_completed();
            }
        }
        // From any of (Openings, Closings, Source) stream.
        template<typename... VoidOrError>
        void on_any_error_unsafe(XRX_RVALUE(VoidOrError&&)... e)
        {
            // 1. We own the mutex.
            for (Subject& window : _windows)
            {
                if constexpr ((sizeof...(e)) == 0)
                {
                    window.on_error();
                }
                else
                {
                    window.on_error(XRX_MOV(e)...);
                }
            }
        }
    };

    template<typename Shared_>
    struct ClosingsObserver_
    {
        using WindowHandle = typename Shared_::WindowHandle;
        WindowHandle _window;
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            _shared->_unsubscribers._source();
            _shared->_unsubscribers._openings();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                if (closer._window == _window)
                {
                    // Invalidate self-reference.
                    closer._detach = {};
                }
                else
                {
                    closer.close();
                }
            }
            closings.clear();
        }
        auto on_next(auto)
        {
            auto guard = std::lock_guard(_shared->mutex());
            _shared->close_window(_window);
            return unsubscribe(true);
        }
        void on_completed()
        {
            // Opened window remains opened if no
            // on_next() close event triggered.
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            // Do not ignore errors. Propagate them from anywhere (closings stream).
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e)...);
        }
    };

    template<typename CloseObservableProducer, typename CloseObservable, typename Shared_>
    struct OpeningsObserver_
    {
        using WindowHandle = typename Shared_::WindowHandle;
        CloseObservableProducer _close_producer;
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            // 1. We own mutex.
            // 2. We cant unsubscribe ourself (self-destroy), just invalidate the reference.
            _shared->_unsubscribers._openings = {};
            _shared->_unsubscribers._source();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                closer.close();
            }
            closings.clear();
        }
        unsubscribe on_next(XRX_RVALUE(auto) open_value)
        {
            CloseObservable closer = _close_producer(XRX_MOV(open_value));

            auto guard = std::lock_guard(_shared->mutex());
            WindowHandle handle = _shared->on_new_window();
            if (not handle)
            {
                unsubscribe_rest_unsafe();
                return unsubscribe(true);
            }
            using ClosingsObserver = ClosingsObserver_<Shared_>;
            auto unsubscriber = XRX_MOV(closer).subscribe(ClosingsObserver(handle, _shared));
            _shared->_unsubscribers._closings.push_back({XRX_MOV(handle), XRX_MOV(unsubscriber)});
            return unsubscribe(false);
        }
        auto on_completed()
        {
            // Done. No more windows possible.
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            // Do not ignore errors. Propagate them from anywhere (openings stream).
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e)...);
        }
    };

    template<typename Shared_>
    struct WindowSourceObserver_
    {
        std::shared_ptr<Shared_> _shared;

        void unsubscribe_rest_unsafe()
        {
            // 1. We own mutex.
            // 2. We cant unsubscribe ourself (self-destroy), just invalidate the reference.
            _shared->_unsubscribers._source = {};
            _shared->_unsubscribers._openings();
            auto& closings = _shared->_unsubscribers._closings;
            for (auto& closer : closings)
            {
                closer.close();
            }
            closings.clear();
        }

        auto on_next(auto source_value)
        {
            auto guard = std::lock_guard(_shared->mutex());
            _shared->on_next_source(XRX_MOV(source_value));
        }
        auto on_completed()
        {
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_completed_source();
        }
        template<typename... VoidOrError>
        void on_error(XRX_RVALUE(VoidOrError&&)... e)
        {
            auto guard = std::lock_guard(_shared->mutex());
            unsubscribe_rest_unsafe();
            _shared->on_any_error_unsafe(XRX_MOV(e)...);
        }
    };

    template<typename Unsubscribers_>
    struct WindowToggleDetach
    {
        using has_effect = std::true_type;

        std::shared_ptr<Unsubscribers_> _shared;

        bool operator()()
        {
            auto shared = std::exchange(_shared, {});
            if (not shared)
            {
                return false;
            }
            auto guard = std::lock_guard(shared->_mutex);
            const bool source_detached = std::exchange(shared->_source, {})();
            const bool openings_detached = std::exchange(shared->_openings, {})();
            bool at_least_one_closer = false;
            for (auto& closer : shared->_closings)
            {
                const bool detached = closer();
                at_least_one_closer |= detached;
            }
            shared->closings.clear();
            return (source_detached
                or openings_detached
                or at_least_one_closer);
        }
    };

    template<typename E1, typename E2, typename E3>
    struct MergedErrors3
    {
        static constexpr bool is_void_like1 = (std::is_same_v<E1, void> or std::is_same_v<E1, none_tag>);
        static constexpr bool is_void_like2 = (std::is_same_v<E2, void> or std::is_same_v<E2, none_tag>);
        static constexpr bool is_void_like3 = (std::is_same_v<E3, void> or std::is_same_v<E3, none_tag>);

        static constexpr bool are_all_voids = (is_void_like1 and is_void_like2 and is_void_like3);
        static constexpr bool are_all_same = (std::is_same_v<E1, E2> and std::is_same_v<E1, E3>);
        using are_compatible = std::bool_constant<are_all_voids or are_all_same>;
        // #TODO: it should be void if at least one is void.
        // Should be none_tag if all of them are none_tag.
        using E = E1;
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer, typename CloseObservable>
    struct WindowToggleObservableImpl_
    {
        SourceObservable _source;
        OpeningsObservable _openings;
        CloseObservableProducer _close_producer;

        using MergedErrors = MergedErrors3<
              typename SourceObservable::error_type
            , typename OpeningsObservable::error_type
            , typename CloseObservable::error_type>;

        static_assert(MergedErrors::are_compatible::value
            , "All steams (Opening, Closings, Source) should have same errors.");

        using source_type = typename SourceObservable::value_type;
        using error_type = typename MergedErrors::E;
        using Subject = Subject_<source_type, error_type>;
        using Windows = HandleVector<Subject>;
        using WindowHandle = typename Windows::Handle;
        using UnsubscribersState = Unsubscribers<
              typename SourceObservable::DetachHandle
            , typename OpeningsObservable::DetachHandle
            , typename CloseObservable::DetachHandle
            , WindowHandle>;
        using Window = typename Subject::Observable;
        using value_type = Window;
        using is_async = std::true_type;
        using DetachHandle = WindowToggleDetach<UnsubscribersState>;

        WindowToggleObservableImpl_ fork() && { return WindowToggleObservableImpl_(XRX_MOV(_source), XRX_MOV(_openings), XRX_MOV(_close_producer)); }
        WindowToggleObservableImpl_ fork() &  { return WindowToggleObservableImpl_(_source.fork(), _openings, _close_producer); }

        template<typename Observer>
            requires ConceptValueObserverOf<Observer, value_type>
        DetachHandle subscribe(XRX_RVALUE(Observer&&) observer) &&
        {
            using Shared_ = WindowToggleShared_<UnsubscribersState, Observer, source_type, error_type>;
            using OpeningsObserver = OpeningsObserver_<CloseObservableProducer, CloseObservable, Shared_>;
            using WindowSourceObserver = WindowSourceObserver_<Shared_>;

            std::shared_ptr<Shared_> shared = std::make_shared<Shared_>(XRX_MOV(observer));
            std::shared_ptr<UnsubscribersState> unsubscribers(shared, &shared->_unsubscribers);
            unsubscribers->_openings = XRX_MOV(_openings).subscribe(OpeningsObserver(XRX_MOV(_close_producer), shared));
            unsubscribers->_source = XRX_MOV(_source).subscribe(WindowSourceObserver(shared));
            return DetachHandle(XRX_MOV(unsubscribers));
        }
    };

    template<typename SourceObservable, typename OpeningsObservable, typename CloseObservableProducer>
    auto tag_invoke(tag_t<make_operator>, ::xrx::detail::operator_tag::WindowToggle
        , XRX_RVALUE(SourceObservable&&) source
        , XRX_RVALUE(OpeningsObservable&&) openings
        , XRX_RVALUE(CloseObservableProducer&&) close_producer)
    {
        XRX_CHECK_RVALUE(source);
        XRX_CHECK_RVALUE(openings);
        XRX_CHECK_RVALUE(close_producer);
        using opening_value = typename OpeningsObservable::value_type;
        using CloseObservable = decltype(close_producer(std::declval<opening_value>()));
        XRX_CHECK_TYPE_NOT_REF(CloseObservable);
        static_assert(ConceptObservable<CloseObservable>);

        static_assert(IsAsyncObservable<SourceObservable>()
            , ".window_toggle() does not make sense for Sync Source Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Openings Observable.");
        static_assert(IsAsyncObservable<OpeningsObservable>()
            , ".window_toggle() does not make sense for Sync Closing Observable.");

        XRX_CHECK_TYPE_NOT_REF(SourceObservable);
        XRX_CHECK_TYPE_NOT_REF(OpeningsObservable);
        XRX_CHECK_TYPE_NOT_REF(CloseObservableProducer);
        using Impl = WindowToggleObservableImpl_<SourceObservable, OpeningsObservable, CloseObservableProducer, CloseObservable>;
        return Observable_<Impl>(Impl(XRX_MOV(source), XRX_MOV(openings), XRX_MOV(close_producer)));
    }
} // namespace xrx::detail
