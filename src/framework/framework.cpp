//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/framework/framework.hpp>

#include <wui/framework/framework_win_impl.hpp>
#if __linux__
#include <wui/framework/framework_lin_impl.hpp>
#endif

#include <wui/framework/i_framework.hpp>

#include <wui/window/listener.hpp>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#endif

#include <memory>
#include <iostream>

namespace wui
{

namespace framework
{
namespace
{
std::shared_ptr<listener> listener_;
std::shared_ptr<i_framework> instance;
}
/// Interface

[[nodiscard]] std::shared_ptr <listener> get_listener()
{
    return listener_;
}

bool init()
{
#ifdef _WIN32
    const auto ret = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (S_OK != ret && S_FALSE != ret)
    {
        std::cerr << "CoInitializeEx() return: " << ret << std::endl;
        return false;
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    const Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    if (Gdiplus::Ok != status)
    {
        std::cerr << "GdiplusStartup() return: " << status << std::endl;
        return false;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); // for DPI

#elif __linux__
    if (setlocale(LC_ALL, "") == NULL)
    {
        std::cerr << "warning: could not set default locale" << std::endl;
    }
#endif

    listener_ = std::make_shared<listener>();
    if (!listener_->init())
    {
        std::cerr << "framework::init : listener init(). " << listener_->get_error().str() << std::endl;
        return false;
    }
    return true;
}

int run(std::function<bool()> task)
{
    if (instance)
    {
        return -1;
    }

#ifdef _WIN32
    instance = std::make_shared<framework_win_impl>();
#elif __linux__
    instance = std::make_shared<framework_lin_impl>();
#elif __APPLE__
    instance = std::make_shared<framework_mac_impl>();
#endif

    const bool ret = instance->run(task);
    if (!ret)
    {
        std::cerr << "framework::run" << instance->get_error().str() << std::endl;
    }

    instance.reset();
    listener_.reset();

#ifdef _WIN32
    CoUninitialize();
#endif
    return ret ? 0 : -1;
}

void stop()
{
    if (instance)
    {
        auto listener__ = get_listener();
        if (!listener__ || !listener__->count()) // added for compatibility
        {
            instance->stop();
        }
    }
}

bool started()
{
    return instance && instance->started();
}

error get_error()
{
    if (instance)
    {
        return std::move(instance->get_error());
    }
    return {};
}

} // framework

}
