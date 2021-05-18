#pragma once
#include "utils_fast_FWD.h"
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

