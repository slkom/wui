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
}

// X11 thread/main
void listener::add_window(xcb_window_t id, std::shared_ptr<window> window)
{
    auto w = windows_.find(id);
    if (w == windows_.end())
    {
        windows_.try_emplace(id, window);
    }
}

// X11 thread/main
void listener::remove_window(xcb_window_t id)
{
    auto w = windows_.find(id);
    if (w != windows_.end())
    {
        windows_.erase(w);
    }

    if (windows_.empty())
    {
        started = false;
    }
}

// X11 thread/main
// вариант function для физического окна:
// sample function: [](const wnd& w) { return w.inited() && w.window_->get_graphic().inited();}
std::shared_ptr<window> listener::get_any_window(std::function<bool(const std::shared_ptr<window> &)> cmp)
{
    auto it{ std::find_if(windows_.begin(), windows_.end(), [cmp](const auto& n) -> bool { return cmp(n.second); }) };
//    return it != windows_.end() ? it->second.window_ : nullptr;
    return it != windows_.end() ? it->second : nullptr;
}

static const char* connection_error_descripion(const int res) noexcept
{
    switch (res)
    {
        case 0:
            return "Success";
        case XCB_CONN_ERROR:
            return "Socket error. A connection error occurred on the socket, pipe, or due to an invalid display address";
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
            return "Connection shutdown because of extension not supported";
        case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
            return "Insufficient memory";
        case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
            return "Connection closed, exceeding request length that server accepts";
        case XCB_CONN_CLOSED_PARSE_ERR:
            return "Connection closed, error during parsing display string (e.g., malformed DISPLAY environment variable)";
        case XCB_CONN_CLOSED_INVALID_SCREEN:
            return "Invalid screen. Connection closed because the server does not have a screen matching the display";
        case XCB_CONN_CLOSED_FDPASSING_FAILED:
            return "Connection closed because some FD passing operation failed";
        default:
            break;
    }
    return "Unknown";
}

// main thread
bool listener::init()
{
    //TODO: надо  перейти полностью на xcb, сейчас wui использует и функции x11lib
    context_.display = XOpenDisplay(nullptr); //TODO: display_name.c_str() if use x11 lib
    if (!context_.display)
    {
        err.set(error_type::system_error, "listener", "Can't make the connection to X server");
        return false;
    }

    XSetEventQueueOwner(context_.display, XCBOwnsEventQueue);

    context_.connection = XGetXCBConnection(context_.display);
    if (!context_.connection)
    {
        XCloseDisplay(context_.display);
        context_.display = nullptr;

        err.set(error_type::system_error, "listener",
            "Could not cast the Display object to an XCBConnection object.");
        return false;
    }

    const auto ret = xcb_connection_has_error(context_.connection);
    if (ret)
    {
        XCloseDisplay(context_.display);
        context_.display = nullptr;
        context_.connection = nullptr;

        err.set(error_type::system_error, "listener",
            "xcb_connection_has_error() return: "
            + std::to_string(ret) + ", " + connection_error_descripion(ret));
        return false;
    }

    context_.screen = xcb_setup_roots_iterator(xcb_get_setup(context_.connection)).data;
    return true;
}

// main thread
void listener::run()
{
    if (started)
        return;
    started = true;
    if (task)
    {
        thread = std::thread(&listener::process_events, this);
        return;
    }
    process_events();
}

// main thread
void listener::stop()
{
    started = false;
}

// main thread
void listener::on_exit()
{
    if (thread.joinable())
    {
        thread.join();
    }

    if (!exit_code)
        err.set(error_type::external_code_error, "listener", "User code return false");

    if (context_.display)
    {
        Display* display = context_.display;
        context_.clear();
        XCloseDisplay(display);
    }
}

static inline xcb_window_t get_window_from_event(uint8_t type, xcb_generic_event_t* e)
{
    switch (type)
    {
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE:
        {
            auto* ev = reinterpret_cast<xcb_key_press_event_t*>(e);
            return ev->event;
        }
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
        {
            auto* ev = reinterpret_cast<xcb_button_press_event_t*>(e);
            return ev->event;
        }
        case XCB_MOTION_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_motion_notify_event_t*>(e);
            return ev->event;
        }
        case XCB_ENTER_NOTIFY:
        case XCB_LEAVE_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_enter_notify_event_t*>(e);
            return ev->event;
        }
        case XCB_FOCUS_IN:
        case XCB_FOCUS_OUT:
        {
            auto* ev = reinterpret_cast<xcb_focus_in_event_t*>(e);
            return ev->event;
        }

        case XCB_EXPOSE:
        {
            auto* ev = reinterpret_cast<xcb_expose_event_t*>(e);
            return ev->window;
        }
        case XCB_PROPERTY_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_property_notify_event_t*>(e);
            return ev->window;
        }
        case XCB_CLIENT_MESSAGE:
        {
            auto* ev = reinterpret_cast<xcb_client_message_event_t*>(e);
            return ev->window;
        }
        case XCB_VISIBILITY_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_visibility_notify_event_t*>(e);
            return ev->window;
        }
        case XCB_CONFIGURE_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_configure_notify_event_t*>(e);
            return ev->window;
        }
        case XCB_UNMAP_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_unmap_notify_event_t*>(e);
            return ev->window;
        }
        case XCB_MAP_NOTIFY:
        {
            auto* ev = reinterpret_cast<xcb_map_notify_event_t*>(e);
            return ev->window;
        }
        //case XCB_DESTROY_NOTIFY: // unused
        //{
        //    auto* ev = reinterpret_cast<xcb_destroy_notify_event_t*>(e);
        //    return ev->window;
        //}

        // XCB_SELECTION_NOTIFY / XCB_SELECTION_REQUEST

        default:
            return XCB_NONE; // XCB_MAPPING_NOTIFY, unused ...
    }
}

// X11 thread/main
void listener::process_events()
{
    if (task)
    {
        exit_code = task();
        task = nullptr;
        if (!exit_code)
        {
            stop_();
            return;
        }
    }

    xcb_generic_event_t *e;
    while (started && (e = xcb_wait_for_event(context_.connection)))
    {
        xcb_window_t w = get_window_from_event(e->response_type & ~0x80, e);
        windows::iterator it;
        if (XCB_NONE != w && (it = windows_.find(w)) != windows_.end())
        {
            it->second->process_events(*e);
        }
        free(e);

        //usleep(10000); // NB: visual test rect redraw
    }
}

void listener::event_loop(system_context &context__)
{
    xcb_generic_event_t* e;
    while (context__.valid() && (e = xcb_wait_for_event(context_.connection)))
    {
        xcb_window_t w = get_window_from_event(e->response_type & ~0x80, e);
        windows::iterator it;
        if (XCB_NONE != w && (it = windows_.find(w)) != windows_.end())
        {
            it->second->process_events(*e);
        }
        free(e);
    }
}

// main thread
error listener::get_error()
{
    return err;
}

#endif

}
