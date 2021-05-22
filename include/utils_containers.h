#pragma once
#include <type_traits>
#include <vector>
#include <limits>
#include <algorithm>

#include <cstdint>
#include <cassert>
#include <cstring>

#include "utils_fast_FWD.h"
#include "utils_defines.h"

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

            friend bool operator==(Handle lhs, Handle rhs) noexcept
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
