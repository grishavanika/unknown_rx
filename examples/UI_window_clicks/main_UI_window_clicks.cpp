#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "xrx/xrx_all.h"
#include "stub_window.h"

struct AppState
{
    StubWindow _wnd;
    HWND _debug_text = nullptr;
};

static void make_UI(AppState& state, HWND wnd)
{
    state._debug_text = ::CreateWindowA(
        "STATIC",
        nullptr,
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0,   // x
        0,   // y
        400, // width
        40,  // height
        wnd,
        (HMENU)1000, // unique id
        ::GetModuleHandle(nullptr),
        nullptr);
    Panic(!!state._debug_text);
}

static StubWindow make_window(AppState& state)
{
    StubWindow::Handlers on_startup;
    on_startup[WM_CREATE] = [app = &state](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        make_UI(*app, hwnd);
        return ::DefWindowProc(hwnd, msg, wparam, lparam);
    };
    auto wnd = StubWindow(
        "UI_window_clicks" // window class name
        , std::move(on_startup));
    Panic(!!wnd.wnd());
    Panic(::ShowWindow(wnd.wnd(), SW_SHOW) == 0); // was previously hidden
    return wnd;
}

static void set_debug_text(AppState& state, const char* fmt, ...)
{
    char log[1024]{};
    va_list args;
    va_start(args, fmt);
    (void)std::vsnprintf(log, sizeof(log), fmt, args);
    va_end(args);
    Panic(!!::SetWindowTextA(state._debug_text, log));
}

struct WindowMessage
{
    HWND _hwnd;
    UINT _msg;
    WPARAM _wparam;
    LPARAM _lparam;
};

struct Position
{
    int _x = 0;
    int _y = 0;
};

static auto make_messages_stream(StubWindow& wnd)
{
    xrx::Subject_<WindowMessage> impl;
    wnd.set_message_handler(
        [action = impl.as_observer()]
            (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) mutable
    {
        action.on_next(WindowMessage(hwnd, msg, wparam, lparam));
        return ::DefWindowProc(hwnd, msg, wparam, lparam);
    });
    return impl.as_observable();
}

static auto on_click(auto messages)
{
    return messages.fork()
        .filter([](WindowMessage msg)
    {
        return (msg._msg == WM_LBUTTONUP);
    })
        .transform([](WindowMessage msg)
    {
        const int x = GET_X_LPARAM(msg._lparam);
        const int y = GET_Y_LPARAM(msg._lparam);
        return Position(x, y);
    });
}

using namespace xrx;

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    AppState app;
    app._wnd = make_window(app);
    auto all_msgs = make_messages_stream(app._wnd);

    on_click(all_msgs)
        | take(5)
        | subscribe(observer::make([&](Position point)
    {
        set_debug_text(app, "Click at {%i, %i}.", point._x, point._y);
    }
        , [&]()
    {
        set_debug_text(app, "Finish.");
    }));

    // Message loop.
    MSG msg{};
    int status = 0;
    while ((status = GetMessage(&msg, app._wnd.wnd(), 0, 0)) != 0)
    {
        if (status != -1)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        else
        {
            break;
        }
    }
    return 0;
}
