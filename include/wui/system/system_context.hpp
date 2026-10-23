//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <X11/Xlib-xcb.h>
#endif

namespace wui
{

class graphic;

#ifdef _WIN32

struct system_context
{
    HWND hwnd{ NULL };
    double scale{ 1.0 };
    [[nodiscard]] bool valid() const noexcept
    {
        return hwnd != NULL;
    }

    // only clear
    void clear() noexcept
    {
        hwnd = NULL;
        scale = 1.0;
    }

};

#elif __linux__

struct system_context
{
    Display* display{ nullptr }; // installed in the listener::init()
    xcb_connection_t *connection{ nullptr }; // installed in the listener::init()

    xcb_screen_t *screen{ nullptr }; // and installed in the graphic::init()
    xcb_window_t wnd{ XCB_WINDOW_NONE }; // [id] installed in the window::init()
    double scale{ 1.0 };

    [[nodiscard]] bool valid() const noexcept
    {
        return screen != nullptr; // xcb
    }

    void clear()
    {
        display = nullptr;
        connection = nullptr;
        screen = nullptr;
        wnd = XCB_WINDOW_NONE;
        scale = 1.0;
    }
};

#endif

}
