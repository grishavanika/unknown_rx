#pragma once
#include "xrx_prologue.h"

namespace xrx::debug
{
    struct LifetimeStats
    {
        int _created = 0;
        int _destroyed = 0;
    };

    struct LifetimeTrack
    {
        LifetimeStats* _stats = nullptr;

        explicit LifetimeTrack(LifetimeStats& stats) noexcept
            : _stats(&stats)
        {
            ++_stats->_created;
        }
        ~LifetimeTrack() noexcept
        {
            ++_stats->_destroyed;
        }
        LifetimeTrack(const LifetimeTrack& rhs) noexcept
            : _stats(rhs._stats)
        {
            ++_stats->_created;
        }
        LifetimeTrack(LifetimeTrack&& rhs) noexcept
            : _stats(rhs._stats)
        {
            ++_stats->_created;
        }
        LifetimeTrack& operator=(const LifetimeTrack&) noexcept
        {
            return *this;
        }
        LifetimeTrack& operator=(LifetimeTrack&&) noexcept
        {
            return *this;
        }
    };

    template<typename T>
    struct WithLifetimeTrack : T
    {
        XRX_CHECK_TYPE_NOT_REF(T);
        LifetimeTrack _tracker;

        explicit WithLifetimeTrack(XRX_RVALUE(T&&) v, LifetimeStats& stats)
            : T(XRX_MOV(v))
            , _tracker(stats)
        {
        }

        WithLifetimeTrack(WithLifetimeTrack&&) = default;
        WithLifetimeTrack& operator=(WithLifetimeTrack&&) = default;
        WithLifetimeTrack(const WithLifetimeTrack&) = default;
        WithLifetimeTrack& operator=(const WithLifetimeTrack&) = default;
    };

} // namespace xrx::debug

#include "xrx_epilogue.h"
