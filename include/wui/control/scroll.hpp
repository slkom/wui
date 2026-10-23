//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/event/event.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/color.hpp>
#include <wui/common/orientation.hpp>

#include <functional>
#include <memory>
#include <thread>
#include <atomic>

namespace wui
{

enum class scroll_state
{
    activated,
    relaxed,

    up_end,
    down_end,

    moving
};

enum class scroll_view
{
    none,
    tiny,
    full
};

class scroll : public i_control, public std::enable_shared_from_this<scroll>
{
public:
    scroll(int32_t area, int32_t scroll_pos,
        orientation orientation_ = orientation::vertical,
        std::function<void(scroll_state, int32_t)> callback = nullptr,
        std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~scroll() override;

    void draw(graphic &gr, const rect) override;
    void redraw(bool clear = false) override;

    void set_position(const rect position) override;
    [[nodiscard]] rect position() const override;
    void move(const int32_t dx, const int32_t dy) override;

    void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] std::weak_ptr<window> parent() const override;
    void clear_parent() override;

    void set_topmost(bool yes) override;
    [[nodiscard]] bool topmost() const override;

    void update_theme_control_name(std::string_view theme_control_name) override;
    void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    void show() override;
    void hide() override;
    [[nodiscard]] bool showed() const override;

    void enable() override;
    void disable() override;
    [[nodiscard]] bool enabled() const override;

    [[nodiscard]] bool focused() const override;
    [[nodiscard]] bool focusing() const override;

    [[nodiscard]] error get_error() override;

    /// Scroll's interface
    void set_area(const int32_t area);
    [[nodiscard]] static int32_t get_buttons_size() noexcept; //! full size buttons [TODO: изменить, когда будет масштабирование]
    void set_scroll_pos(int32_t scroll_pos);
    [[nodiscard]] int32_t get_scroll_pos() const noexcept;

    /// Good to call from mouse whell event
    bool scroll_up();
    bool scroll_down();

    /// If you need to embed a scroll bar in your control, you can draw it with scroll::draw() and subscribe it to events
    void receive_control_events(const event& ev);
    void receive_plain_events(const event& ev);

    [[nodiscard]] scroll_view get_scroll_view() const noexcept;

    /// Control name in theme
    static constexpr const char *tc = "scroll";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_slider = "slider";
    static constexpr const char *tv_slider_acive = "slider_active";

    static constexpr const int32_t tiny_scrollbar_size = 4;
    static constexpr const int32_t full_scrollbar_size = 14;

private:
    std::string tcn; // control name
    std::shared_ptr<i_theme> theme_;

    rect position_{};

    std::weak_ptr<window> parent_;
    std::string my_control_sid, my_plain_sid;

    bool showed_{ true }, enabled_{ true }, topmost_{ false };
    bool slider_scrolling{ false };

    std::atomic<double> area, scroll_pos, prev_scroll_pos{ -1.0 };
    std::atomic<double> scroll_interval{ 1.0 };

    orientation orientation_;

    std::function<void(scroll_state, int32_t)> callback;

    enum class worker_action
    {
        undefined = 0,

        scroll_up,
        scroll_down,

        scrollbar_show
    };

    worker_action worker_action_{ worker_action::undefined };
    std::thread worker;
    std::atomic<bool> worker_started{ false };
    std::atomic<bool> worker_done{ true };

    std::atomic<int32_t> progress{ 0 };

    scroll_view scroll_view_{ scroll_view::tiny };

    int32_t slider_click_pos{ 0 };

    int32_t title_height{ 0 };

    void redraw_progress();
    void draw_arrow_up(graphic& gr, const rect button_pos);
    void draw_arrow_left(graphic& gr, const rect button_pos);
    void draw_arrow_down(graphic& gr, const rect button_pos);
    void draw_arrow_right(graphic& gr, const rect button_pos);

    void calc_scroll_interval();
    void calc_vert_scrollbar_params(rect* bar_rect = nullptr, rect* up_button_rect = nullptr, rect* down_button_rect = nullptr, rect* slider_rect = nullptr);
    void calc_hor_scrollbar_params(rect* bar_rect = nullptr, rect* up_button_rect = nullptr, rect* down_button_rect = nullptr, rect* slider_rect = nullptr);
    void calc_scrollbar_params(rect* bar_rect = nullptr, rect* up_button_rect = nullptr, rect* down_button_rect = nullptr, rect* slider_rect = nullptr);

    void move_slider(const int32_t v);

    void start_work(const worker_action action);
    void work();
    void end_work();
};

}
