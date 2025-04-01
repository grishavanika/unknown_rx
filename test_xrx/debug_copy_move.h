#pragma once
// #include <source_location>
#include <memory>
#include <cstdio>

namespace xrx::debug
{
    struct CopyMoveTrack
    {
        struct Stats
        {
            int _created = 0;
            int _destroyed = 0;
        };
        int _user;
        std::shared_ptr<Stats> _stats;
        int _id;

        CopyMoveTrack(int user) noexcept
            : _user(user)
            , _stats(std::make_shared<Stats>())
            , _id(++_stats->_created)
        {
            printf("[make %i] Ctor [%i]\n", _id, _user);
        }
        ~CopyMoveTrack() noexcept
        {
            ++_stats->_destroyed;
            printf("[kill %i] Dtor [%i]. Alive: %i\n", _id, _user
                , (_stats->_created - _stats->_destroyed));
        }
        CopyMoveTrack(const CopyMoveTrack& rhs) noexcept
            : _user(rhs._user)
            , _stats(rhs._stats)
            , _id(++_stats->_created)
        {
            printf("[make %i] Copy of %i [%i]\n", _id, rhs._id, _user);
        }
        CopyMoveTrack(CopyMoveTrack&& rhs) noexcept
            : _user(rhs._user)
            , _stats(rhs._stats) // copy, not move
            , _id(++_stats->_created)
        {
            printf("[make %i] Move of %i [%i]\n", _id, rhs._id, _user);
        }
        CopyMoveTrack& operator=(const CopyMoveTrack& rhs) noexcept
        {
            printf("[assign %i] Copy Assign of %i [%i]\n", _id, rhs._id, _user);
            _user = rhs._user;
            return *this;
        }
        CopyMoveTrack& operator=(CopyMoveTrack&& rhs) noexcept
        {
            printf("[assign %i] Move Assign of %i [%i]\n", _id, rhs._id, _user);
            _user = rhs._user;
            return *this;
        }
    };
} // namespace xrx::debug
