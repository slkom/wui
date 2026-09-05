//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/ud84/wui
//

#include <wui/window/listener.hpp>
#include <algorithm>

namespace wui
{
#if __linux__

listener::~listener()
{
    stop();
}

void listener::add_window(xcb_window_t id, std::shared_ptr<window> window)
{
    auto w = windows.find(id);
    if (w == windows.end())
    {
        windows[id] = { std::move(window), false };
    }
}

void listener::delete_window(xcb_window_t id)
{
    auto w = windows.find(id);
    if (w != windows.end())
    {
        windows.erase(w);
    }

    if (windows.empty())
    {
        started = false;
    }
}

// вариант function для физического окна:
// sample function: [](const wnd& w) { return w.inited() && w.window_->get_graphic().inited();}
std::shared_ptr<window> listener::get_any_window(std::function<bool(const wnd&)> cmp)
{
    auto it{ std::find_if(windows.begin(), windows.end(), [cmp](const auto& n) -> bool { return cmp(n.second); }) };
    return it != windows.end() ? it->second.window_ : nullptr;
}

bool listener::is_init() const
{
    return nullptr != context_.display;
}

bool listener::init()
{
    context_.display = XOpenDisplay(nullptr); //TODO: display_name.c_str()
    if (!context_.display)
    {
        err.set(error_type::system_error, "listener::init()", "Can't make the connection to X server");
        return false;
    }

    XSetEventQueueOwner(context_.display, XCBOwnsEventQueue);
    context_.connection = XGetXCBConnection(context_.display);
    if (!context_.connection)
    {
        XCloseDisplay(context_.display);
        context_.display = nullptr;

        err.set(error_type::system_error,
            "listener::init()", "Could not cast the Display object to an XCBConnection object.");
        return false;
    }

    context_.screen = xcb_setup_roots_iterator(xcb_get_setup(context_.connection)).data;

    start();

    return true;
}

void listener::start()
{
    if (started)
    {
        return;
    }

    started = true;
    thread = std::thread(std::bind(&listener::process_events, this));
}

void listener::stop()
{
    started = false;
    if (thread.joinable())
    {
        thread.join();
        // assert(windows.empty());
        windows.clear();
    }

    if (context_.display)
    {
        Display* display = context_.display;
        context_.clear();
        XCloseDisplay(display);
    }
}

system_context const &listener::context() const
{
    return context_;
}

void listener::process_events()
{
    xcb_generic_event_t *e = nullptr;
    xcb_flush(context_.connection);
    while (started && (e = xcb_wait_for_event(context_.connection)))
    {
        xcb_window_t w = e->pad[2];

        switch (e->response_type & 0x7f)
        {
            case XCB_EXPOSE:
            {
                auto ev = (xcb_expose_event_t*)e;
                w = ev->window;
            }
            break;
            case XCB_CONFIGURE_NOTIFY:
            case XCB_PROPERTY_NOTIFY:
            case XCB_CLIENT_MESSAGE:
            {
                auto ev = (xcb_configure_notify_event_t*)&e;
                w = e->pad[0];
            }
            break;
            default:
            break;
        }

        auto wnd = windows.find(w);
        if (wnd != windows.end())
        {
            auto &w = wnd->second;
            if (!w.created)
            {
                w.created = true;
                event ev;
                ev.type = event_type::internal;
                ev.internal_event_.type = internal_event_type::window_created;
                w.window_->receive_control_events(ev);
            }
            w.window_->process_events(*e);
        }
        free(e);
    }
}

error listener::get_error() const
{
    return err;
}

#endif

}
