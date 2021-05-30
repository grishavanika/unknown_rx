#pragma once
#include <mutex>

#include <cstddef>
#include <cstdio>

namespace xrx::debug
{
    struct NewAllocsStats
    {
        std::mutex _mutex;
        std::size_t _count = 0;
        std::size_t _size = 0;

        static NewAllocsStats& get()
        {
            static NewAllocsStats allocs;
            return allocs;
        }

        void reset()
        {
            auto guard = std::lock_guard(_mutex);
            _count = 0;
            _size = 0;
        }

        void dump(const char* title, std::FILE* stream = stdout)
        {
            auto guard = std::lock_guard(_mutex);
            std::fprintf(stream, "[%s] Allocations count             : %u.\n", title, unsigned(_count));
            std::fprintf(stream, "[%s] Allocations total size (bytes): %u.\n", title, unsigned(_size));
            std::fflush(stream);
        }
    };
} // namespace xrx::debug
