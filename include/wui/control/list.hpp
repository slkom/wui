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
#include <wui/control/scroll.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace wui
{

class list : public i_control, public std::enable_shared_from_this<list>
{
public:
    list(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    virtual ~list();

    virtual void draw(graphic &gr, const rect&) override;

    virtual void set_position(const rect& position) override;
    [[nodiscard]] virtual rect position() const override;
    virtual void move(const int32_t dx, const int32_t dy) override;

    virtual void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] virtual std::weak_ptr<window> parent() const override;
    virtual void clear_parent();

    virtual void set_topmost(bool yes) override;
    [[nodiscard]] virtual bool topmost() const override;

    virtual void update_theme_control_name(std::string_view theme_control_name) override;
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    virtual void show() override;
    virtual void hide() override;
    [[nodiscard]] virtual bool showed() const override;

    virtual void enable() override;
    virtual void disable() override;
    [[nodiscard]] virtual bool enabled() const override;

    [[nodiscard]] virtual bool focused() const override;
    [[nodiscard]] virtual bool focusing() const override;

    [[nodiscard]] virtual error get_error() const override;

public:

    /// List's interface
    struct column
    {
        int32_t width;
        std::string caption;
    };
    void update_columns(const std::vector<column> &columns__);
    [[nodiscard]] const std::vector<column> &columns();

    enum class list_mode
    {
        simple,
        auto_select,
        simple_topmost
    };
    void set_mode(list_mode mode) noexcept;

    void select_item(int32_t n_item); // TODO: ? rename to select_item_number() (class list use this name)

    [[nodiscard]] int32_t selected_item() const noexcept
    {
        return selected_item_;
    }

    void set_column_width(int32_t n_column, int32_t width);
    [[nodiscard]] int32_t get_item_height(int32_t n_item) const;

    void set_item_count(int32_t count);

    [[nodiscard]] int32_t get_item_count() const noexcept
    {
        return item_count;
    }

    void make_selected_visible();
    void scroll_to_start();
    void scroll_to_end();

    [[nodiscard]] int32_t get_item_top(int32_t n_item) const;
    [[nodiscard]] int32_t get_font_size() const;
    /// возвращает предпочтительный размер, (height = 0) calc max height
    [[nodiscard]] rect get_preferred_size(const int32_t height = 0);

    void set_keyboard_auto_select(const bool auto_select) noexcept
    {
        keyboard_auto_select = auto_select;
    }

    void redraw();

    enum class item_state
    {
        normal,
        active,
        selected
    };

    enum class click_button
    {
        left,
        center,
        right
    };

    //NB: для совместимости (если важно?) можно оставить `const rect`.
    // Если используется std::bind(), вызов не замечает разницы так как forward() использует `&&` ...
    //void set_draw_callback(std::function<void(graphic&, int32_t nItem, const rect, item_state)> draw_callback_) noexcept;
    void set_draw_callback(std::function<void(graphic&, int32_t nItem, const rect&, item_state)> draw_callback_) noexcept;
    void set_item_height_callback(std::function<void(int32_t nItem, int32_t& height)> item_height_callback_) noexcept;
    void set_item_click_callback(std::function<void(click_button, int32_t nItem, int32_t x, int32_t y)> item_click_callback_) noexcept;
    void set_item_change_callback(std::function<void(int32_t nItem)> item_change_callback_) noexcept;
    void set_item_activate_callback(std::function<void(int32_t nItem)> item_activate_callback_) noexcept;
    void set_column_click_callback(std::function<void(int32_t column)> column_click_callback_) noexcept;
    void set_scroll_callback(std::function<void(scroll_state, int32_t v)> scroll_callback_) noexcept;

public:
    /// Control name in theme
    static constexpr const char *tc = "list";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_hover_border = "hover_border";
    static constexpr const char *tv_focused_border = "focused_border";
    static constexpr const char *tv_title = "title";
    static constexpr const char *tv_title_column = "title_column";
    static constexpr const char *tv_title_text = "title_text";
    static constexpr const char *tv_selected_item = "selected_item";
    static constexpr const char *tv_active_item = "active_item";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_border_item = "border_item";
    static constexpr const char *tv_item_indent = "item_indent";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

    static constexpr const int32_t column_ident_ = 2;
    static constexpr const int32_t text_indent = 5;

    int32_t get_left_position_text(int32_t n_col) const noexcept;

    struct theme_data
    {
        color background{ make_color(0, 0, 0, 0) };
        color border{ make_color(0, 0, 0, 0) };
        color hover_border{ make_color(0, 0, 0, 0) };
        color focused_border{ make_color(0, 0, 0, 0) };
        color title{ make_color(0, 0, 0, 0) };
        color title_column{ make_color(0, 0, 0, 0) };
        color title_text{ make_color(0, 0, 0, 0) };
        color selected_item{ make_color(0, 0, 0, 0) };
        color active_item{ make_color(0, 0, 0, 0) };
        int32_t border_width{ };
        int32_t border_item{ };
        int32_t item_indent{ };
        int32_t round{ };
        font font_;
    };

    const theme_data& get_theme_data() const noexcept
    {
        return theme_data_;
    }

private:

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;
    theme_data theme_data_;
    rect position_;

    std::weak_ptr<window> parent_;
    std::string my_control_sid;

    bool showed_, enabled_, focused_, mouse_on_control, mouse_on_slider;
    bool lock_changes_item_count{ false };

    // true : key select new item
    // false : key (exclude `return`) not select new item
    bool keyboard_auto_select{ false };

    std::vector<column> columns_;

    list_mode mode;

    // TODO: int64_t
    int32_t item_count, selected_item_, active_item_;

    std::unique_ptr<graphic> mem_gr;

    int32_t title_height;

    int32_t scroll_area;

    std::shared_ptr<scroll> vert_scroll;

    // TODO: int64_t n_item
    std::function<void(graphic&, int32_t, const rect&, item_state)> draw_callback;
    std::function<void(int32_t, int32_t&)> item_height_callback;
    std::function<void(click_button, int32_t, int32_t, int32_t)> item_click_callback;
    std::function<void(int32_t)> item_change_callback;
    std::function<void(int32_t)> item_activate_callback;
    std::function<void(int32_t)> column_click_callback;
    std::function<void(scroll_state, int32_t)> scroll_callback;

    void receive_control_events(const event &ev);

    void on_scroll(scroll_state, int32_t);

    void redraw_item(int32_t item);

    void calc_title_height();
    void draw_titles(graphic &gr_);

    void draw_items(graphic &gr_);

    [[nodiscard]] int32_t check_items_height(const int32_t height);

    [[nodiscard]] bool has_scrollbar() const noexcept
    {
        return scroll_area > 2;
    }

    void update_selected_item(int32_t y);
    void update_active_item(int32_t y);

    void update_scroll_area();
    void update_scroll(const bool changed);

    // Management for performance
    void update_theme_data();

    // подходит для не перекрывающихся control, не используется...
    //[[nodiscard]] color get_background_color() const;

    [[nodiscard]] bool update_mem_gr();
};

}
