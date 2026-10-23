//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#ifdef __linux__

#include <wui/framework/framework_lin_impl.hpp>

#include <wui/window/listener.hpp>

namespace wui
{

namespace framework
{
extern std::shared_ptr<listener> get_listener();

// call: main thread
bool framework_lin_impl::run(std::function<bool()> task)
{
    if (started_)
    {
        err.set(error_type::already_started, "framework_lin_impl::run()");
        return false;
    }

    auto listener__ = framework::get_listener();
    if (!listener__)
    {
        return false;
    }

    if (task)
    {
        listener__->add_task(task, [this]() { stop(); });
    }

    started_ = true;
    listener__->run();

    if (listener__->is_thread())
    {
        while (started_)
        {
            std::unique_lock<std::mutex> lm{ mtx_cv_ };
            cv_.wait(lm);
        }
        listener__->stop(); // set flag stop and wait exit thread
    }

    listener__->on_exit(); // clean

    if (!listener__->get_error().is_ok())
    {
        err = std::move(listener__->get_error());
        return false;
    }
    return true;
}

void framework_lin_impl::stop()
{
    if (!started_)
        return;

    auto listener__ = framework::get_listener();
    if (!listener__ || listener__->is_thread())
    {
        std::unique_lock<std::mutex> lm{ mtx_cv_ };
        started_ = false;
        cv_.notify_one(); // event: other thread, set flag started_, exit wait()
        return;
    }

    started_ = false;
    listener__->stop(); // event: main thread, set flag
}

bool framework_lin_impl::started() const
{
    return started_;
}

error framework_lin_impl::get_error()
{
    return err;
}

}

}

#endif
