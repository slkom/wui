//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#ifdef _WIN32

#include <wui/framework/framework_win_impl.hpp>

#include <wui/window/listener.hpp>

#include <windows.h>

namespace wui
{

namespace framework
{
extern std::shared_ptr<listener> get_listener();

bool framework_win_impl::run(std::function<bool()> task)
{
    if (started_)
    {
        err.set(error_type::already_started, "framework_win_impl::run()");
        return false;
    }
    auto listener__ = framework::get_listener();
    if (!listener__)
    {
        return false;
    }

    started_ = true;

    if (task)
    {
        const auto ret = task();
        task = nullptr;
        if (!ret)
        {
            started_ = false;
            err.set(error_type::external_code_error, "run", "User code return false");
            return false;
        }
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    listener__->on_exit();
    started_ = false;
    return true;
}

void framework_win_impl::stop()
{
    if (started_)
    {
        PostQuitMessage(IDCANCEL);
        started_ = false;
    }
}

bool framework_win_impl::started() const
{
    return started_;
}

error framework_win_impl::get_error()
{
    return err;
}

}

} // framework

#endif
