#pragma once
#include "utils_observers.h"

#include <gmock/gmock.h>

template<typename Value>
struct ObserverMockAny
{
    MOCK_METHOD(void, on_next, (Value));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, (::xrx::void_));

    auto ref() & { return ::xrx::observer::ref(*this); }
};

using ObserverMock = ObserverMockAny<int>;

struct ObserverMock_Error
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, (int));

    auto ref() & { return ::xrx::observer::ref(*this); }
};
