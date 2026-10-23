//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/ud84/wui
//

#pragma once

#include <wui/window/window.hpp>

#include <wui/system/system_context.hpp>
#include <wui/common/error.hpp>

#include <thread>
#include <unordered_map>

#ifdef __linux__
#include <xcb/xcb.h>
#endif

namespace wui
{

#ifdef __linux__

namespace framework
{
class framework_lin_impl;
}

class listener
{
    friend class framework::framework_lin_impl;
public:

    listener() = default;
    ~listener();

    void add_window(xcb_window_t id, std::shared_ptr<window> window);
    void remove_window(xcb_window_t id);

    [[nodiscard]] bool init();

    void run();

    [[nodiscard]] bool is_init() const noexcept
    {
        return nullptr != context_.display;
    }

    bool is_thread() const noexcept
    {
        return thread.joinable();
    }

    void add_task(std::function<bool()> task_, std::function<void()> stop__)
    {
        task = std::move(task_);
        stop_ = std::move(stop__);
    }

    [[nodiscard]] system_context const &context() const noexcept
    {
        return context_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return windows_.empty();
    }

    [[nodiscard]] size_t count() const noexcept
    {
        return windows_.size();
    }

    /// event loop for dialog, etc (see example hello_word)
    void event_loop(system_context& context_);

    void set_scale(const double scale) noexcept
    {
        for (auto &w : windows_)
        {
            w.second->set_scale(scale);
        }
    }

    [[nodiscard]] std::shared_ptr<window> get_first() noexcept
    {
        auto it = windows_.begin();
        return it != windows_.end() ? it->second : nullptr;
    }

    [[nodiscard]] std::shared_ptr<window> get_any_window(std::function<bool(const std::shared_ptr<window>& )> cmp);

    [[nodiscard]] error get_error();

 private:
    std::atomic<bool> started{ false };
    std::atomic<bool> exit_code{ true };

    std::thread thread;

    system_context context_;

    std::function<bool()> task;
    std::function<void()> stop_;

    struct identity_hash
    {
        std::size_t operator()(xcb_window_t key) const noexcept
        {
            return static_cast<std::size_t>(key);
        }
    };

    using windows = std::unordered_map<xcb_window_t, std::shared_ptr<window>, identity_hash>;
    windows windows_;

    error err;

    void stop();
    void on_exit();
    void process_events();
};

#elif _WIN32

namespace framework
{
class framework_win_impl;
}

// Windows implementation
class listener
{
    friend class framework::framework_win_impl;
public:

    listener() = default;
    ~listener() = default;

    void add_window(void* hwnd, std::shared_ptr<window> window_)
    {
        auto w = windows_.find(hwnd);
        if (w == windows_.end())
        {
            windows_.try_emplace(hwnd, window_);
        };
    }

    void remove_window(void* hwnd)
    {
        auto w = windows_.find(hwnd);
        if (w != windows_.end())
        {
            windows_.erase(w);
        }
    }

    inline static constexpr bool init() noexcept
    {
        return true;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return windows_.empty();
    }

    [[nodiscard]] size_t count() const noexcept
    {
        return windows_.size();
    }

    void set_scale(const double scale) noexcept
    {
        for (auto& w : windows_)
        {
            w.second->set_scale(scale);
        }
    }


    [[nodiscard]] std::shared_ptr<window> get_first()
    {
        auto it = windows_.begin();
        return it != windows_.end() ? it->second : nullptr;
    }

    [[nodiscard]] std::shared_ptr<window> get_any_window(std::function<bool(const std::shared_ptr<window>&)> cmp)
    {
        auto it{ std::find_if(windows_.begin(), windows_.end(), [cmp](const auto& n) -> bool { return cmp(n.second); }) };
        return it != windows_.end() ? it->second : nullptr;
    }

    [[nodiscard]] error get_error() { return err; }

  private:

      using windows = std::unordered_map<void*, std::shared_ptr<window>>;
      windows windows_;

    error err;

    inline static void on_exit()
    {
    }

};

#endif

}

