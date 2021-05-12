#pragma once
#include <cassert>

// This is specifically made to be ugly.
// TO BE REMOVED.
// 
namespace xrx::debug
{
    namespace detail
    {
        struct AssertFlagAction
        {
            void operator()() const
            {
                assert(false && "Debug flag is unexpectedly set.");
            }
        };
    } // namespace detail

    template<typename Action = detail::AssertFlagAction>
    class AssertFlag
    {
    public:
        explicit AssertFlag() noexcept
            : _action()
            , _state(false)
        {
        }

        void check_not_set() const
        {
            if (_state)
            {
                _action();
            }
        }

        void raise()
        {
            _state = true;
        }
        
    private:
        [[no_unique_address]] Action _action;
        bool _state = false;
    };
} // namespace xrx::debug
