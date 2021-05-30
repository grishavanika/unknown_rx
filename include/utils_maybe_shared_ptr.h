#include "xrx_prologue.h"
#include <memory>
#include <utility>
#include <cassert>

namespace xrx::detail
{
    template<typename T>
    struct MaybeSharedPtr;
    template<typename T>
    struct MaybeWeakPtr;

    template<typename T>
    struct MaybeSharedPtr
    {
        T* _ptr = nullptr;
        std::shared_ptr<T> _shared;
        
        static MaybeSharedPtr from_ref(T& value) noexcept
        {
            return MaybeSharedPtr(&value);
        }
        static MaybeSharedPtr from_shared(std::shared_ptr<T>&& shared) noexcept
        {
            return MaybeSharedPtr(XRX_MOV(shared));
        }
        MaybeSharedPtr(const MaybeSharedPtr&) noexcept = default;
        MaybeSharedPtr& operator=(const MaybeSharedPtr&) noexcept = default;
        MaybeSharedPtr(MaybeSharedPtr&& rhs) noexcept
            : _ptr(std::exchange(rhs._ptr, nullptr))
            , _shared(std::exchange(rhs._shared, nullptr))
        {
        }
        MaybeSharedPtr& operator=(MaybeSharedPtr&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _ptr = std::exchange(rhs._ptr, nullptr);
                _shared = std::exchange(rhs._shared, nullptr);
            }
            return *this;
        }

        MaybeSharedPtr& operator=(std::nullptr_t) noexcept
        {
            _ptr = nullptr;
            _shared = nullptr;
            return *this;
        }

        T* get() const noexcept
        {
            return _ptr ? _ptr : _shared.get();
        }

        T* operator->() const noexcept
        {
            return get();
        }

        T& operator*() const noexcept
        {
            assert(get());
            return *get();
        }

        explicit operator bool() const noexcept
        {
            return !!get();
        }

    private:
        explicit MaybeSharedPtr(T* ptr) noexcept
            : _ptr(ptr)
            , _shared()
        {
            assert(_ptr);
        }
        explicit MaybeSharedPtr(std::shared_ptr<T>&& shared) noexcept
            : _ptr(nullptr)
            , _shared(XRX_MOV(shared))
        {
            assert(_shared);
        }
    };

    template<typename T>
    struct MaybeWeakPtr
    {
        T* _ptr = nullptr;
        std::weak_ptr<T> _shared_weak;

        MaybeWeakPtr()
            : _ptr(nullptr)
            , _shared_weak()
        {
        }

        MaybeWeakPtr(const MaybeSharedPtr<T>& shared) noexcept
            : _ptr(shared._ptr)
            , _shared_weak(shared._shared)
        {
        }

        MaybeWeakPtr(const MaybeWeakPtr&) noexcept = default;
        MaybeWeakPtr& operator=(const MaybeWeakPtr&) noexcept = default;
        MaybeWeakPtr(MaybeWeakPtr&& rhs) noexcept
            : _ptr(std::exchange(rhs._ptr, nullptr))
            , _shared_weak(std::exchange(rhs._shared_weak, {}))
        {
        }
        MaybeWeakPtr& operator=(MaybeWeakPtr&& rhs) noexcept
        {
            if (this != &rhs)
            {
                _ptr = std::exchange(rhs._ptr, nullptr);
                _shared_weak = std::exchange(rhs._shared_weak, {});
            }
            return *this;
        }

        MaybeWeakPtr& operator=(std::nullptr_t) noexcept
        {
            _ptr = nullptr;
            _shared_weak = nullptr;
            return *this;
        }

        MaybeSharedPtr<T> lock()
        {
            if (_ptr)
            {
                return MaybeSharedPtr<T>::from_ref(*_ptr);
            }
            return MaybeSharedPtr<T>::from_shared(_shared_weak.lock());
        }

        bool expired() const
        {
            return (_ptr ? false : _shared_weak.expired());
        }
    };

} // namespace xrx::detail

#include "xrx_epilogue.h"
