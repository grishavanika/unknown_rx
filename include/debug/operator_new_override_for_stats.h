// NO include guard.

#if defined(XRX_NEW_OVERRIDE_INCLUDED)
#  error Operator new overrides already included.
#endif
#define XRX_NEW_OVERRIDE_INCLUDED

#include "debug/new_allocs_stats.h"
#include <new>
#include <cstdlib>

// Intentionally non-inline to break ODR if included multiple times.
void* operator new(std::size_t count)
{
    {
        auto& allocs = ::xrx::debug::NewAllocsStats::get();
        auto guard = std::lock_guard(allocs._mutex);
        allocs._count += 1;
        allocs._size += count;
    }
    void* ptr = std::malloc(count);
    if (not ptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void* operator new[](std::size_t count)
{
    {
        auto& allocs = ::xrx::debug::NewAllocsStats::get();
        auto guard = std::lock_guard(allocs._mutex);
        allocs._count += 1;
        allocs._size += count;
    }
    void* ptr = std::malloc(count);
    if (not ptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}

