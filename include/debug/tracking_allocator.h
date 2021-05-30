#pragma once
#include <memory>
#include <mutex>
#include <cassert>

namespace xrx::debug
{
    struct TrackingAllocatorState
    {
        std::mutex _mutex;
        std::size_t _allocs_count = 0;
        std::size_t _deallocs_count = 0;
        std::size_t _allocs_size = 0;
        std::size_t _deallocs_size = 0;
        std::size_t _constructs_count = 0;
        std::size_t _destroys_count = 0;
    };

    template<typename T, typename Alloc = std::allocator<void>>
    struct TrackingAllocator
    {
        // C++20 interface.
        using value_type = T;
        using allocator_traits = std::allocator_traits<Alloc>;
        using RightAlloc = allocator_traits::template rebind_alloc<T>;

        template<class U>
        struct rebind
        {
            using other = TrackingAllocator<U, Alloc>;
        };

        [[no_unique_address]] Alloc _alloc;
        TrackingAllocatorState* _state;

        constexpr TrackingAllocator(TrackingAllocatorState* state)
            : _alloc()
            , _state(state)
        {
        }
        // Trailing-allocator convention:
        // https://en.cppreference.com/w/cpp/memory/uses_allocator.
        constexpr explicit TrackingAllocator(TrackingAllocatorState& state, const Alloc& alloc)
            : _alloc(alloc)
            , _state(state)
        {
        }
        template<typename U, typename OtherAlloc>
        constexpr TrackingAllocator(const TrackingAllocator<U, OtherAlloc>& rhs)
            : _alloc(rhs._alloc)
            , _state(rhs._state)
        {
        }

        template<typename U, typename... Args>
        constexpr void construct(U* p, Args&&... args)
        {
            {
                assert(_state);
                auto guard = std::lock_guard(_state->_mutex);
                _state->_constructs_count += 1;
            }
            allocator_traits::construct(_alloc, p, std::forward<Args>(args)...);
        }

        template<typename U>
        constexpr void destroy(U* p)
        {
            {
                assert(_state);
                auto guard = std::lock_guard(_state->_mutex);
                _state->_destroys_count += 1;
            }
            allocator_traits::destroy(_alloc, p);
        }

        [[nodiscard]] constexpr T* allocate(std::size_t n)
        {
            {
                assert(_state);
                auto guard = std::lock_guard(_state->_mutex);
                _state->_allocs_count += 1;
                _state->_allocs_size += (n * sizeof(T));
            }
            return RightAlloc(_alloc).allocate(n);
        }

        constexpr void deallocate(T* p, std::size_t n)
        {
            {
                assert(_state);
                auto guard = std::lock_guard(_state->_mutex);
                _state->_deallocs_count += 1;
                _state->_deallocs_size += (n * sizeof(T));
            }
            RightAlloc(_alloc).deallocate(p, n);
        }
    };
} // namespace xrx::debug

