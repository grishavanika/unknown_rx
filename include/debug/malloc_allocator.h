#pragma once
#include <new>
#include <cstdlib>

namespace xrx::debug
{
    template<typename T>
    struct MallocAllocator
    {
        // C++20 interface.
        using value_type = T;

        MallocAllocator() = default;

        template<typename U>
        MallocAllocator(const MallocAllocator<U>&)
        {
        }

        [[nodiscard]] constexpr T* allocate(std::size_t n)
        {
            void* ptr = std::malloc(n * sizeof(T));
            if (not ptr)
            {
                throw std::bad_alloc();
            }
            return static_cast<T*>(ptr);
        }

        constexpr void deallocate(T* p, std::size_t n)
        {
            (void)n;
            std::free(p);
        }
    };
} // namespace xrx::debug
