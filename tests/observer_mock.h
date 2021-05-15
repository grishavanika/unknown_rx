#pragma once
#include "utils_observers.h"

#include <gmock/gmock.h>

struct ObserverMock
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, ());

    auto ref() & { return ::xrx::observer::ref(*this); }
};

struct ObserverMock_Error
{
    MOCK_METHOD(void, on_next, (int));
    MOCK_METHOD(void, on_completed, ());
    MOCK_METHOD(void, on_error, (int));

    auto ref()& { return ::xrx::observer::ref(*this); }
};
