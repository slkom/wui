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
#include <wui/control/list.hpp>
#include <wui/theme/theme.hpp>

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace wui
{

struct select_item; // TODO: ? move to class select

typedef std::vector<select_item> select_items_t;

struct select_item
{
    int64_t id;

    std::string text;

    inline bool operator==(int64_t id_) const noexcept
    {
        return id == id_;
    }
};

class select : public i_control, public std::enable_shared_from_this<select>
{
public:
    explicit select(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~select() override;

    void draw(graphic &gr, const rect) override;
    void redraw(bool clear = false) override;

    void set_position(const rect position) override;
    [[nodiscard]] rect position() const override;
    void move(const int32_t dx, const int32_t dy) override;

    void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] std::weak_ptr<window> parent() const override;
    void clear_parent();

    void set_topmost(bool yes) override;
    [[nodiscard]] bool topmost() const override;

    void update_theme_control_name(std::string_view theme_control_name) override;
    void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    void show() override;
    void hide() override;
    [[nodiscard]] bool showed() const override;

    void enable() override;
    void disable() override;
    [[nodiscard]] virtual bool enabled() const override;

    [[nodiscard]] bool focused() const override;
    [[nodiscard]] bool focusing() const override;

    [[nodiscard]] error get_error() override;

    /// Select's interface
    void set_items(const select_items_t &items);
    void update_item(const select_item &mi);
    void swap_items(const int64_t first_item_id, const int64_t second_item_id);
    void delete_item(const int64_t id);

    void set_item_height(const int32_t item_height) noexcept;

    void select_item_number(const int32_t index);
    void select_item_id(const int64_t id);
    [[nodiscard]] select_item selected_item() const;
    [[nodiscard]] const select_items_t &items() const;

    void set_change_callback(std::function<void(int32_t /* n_item */, int64_t /* id */)> change_callback) noexcept;

    /// возвращает заданный размер шрифта
    [[nodiscard]] int32_t get_font_size() const
    {
        return theme_font(tcn, tv_font, theme_).size;
    }

    /// returns recommended distance between the baselines of two adjacent lines of text
    [[nodiscard]] int32_t font_line_height(int32_t* max_height = nullptr) const
    {
        return wui::font_line_height(theme_font(tcn, tv_font, theme_), nullptr, max_height);
    }
    /// returns max text height
    [[nodiscard]] int32_t font_max_height() const
    {
        return wui::font_max_height(theme_font(tcn, tv_font, theme_), nullptr);
    }

    /// Control name in theme
    static constexpr const char *tc = "select";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_hover_border = "hover_border";
    static constexpr const char *tv_focused_border = "focused_border";
    static constexpr const char *tv_button_calm = "button_calm";
    static constexpr const char *tv_button_active = "button_active";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_scrollbar = "scrollbar";
    static constexpr const char *tv_scrollbar_slider = "scrollbar_slider";
    static constexpr const char *tv_scrollbar_slider_acive = "scrollbar_slider_active";
    static constexpr const char *tv_selected_item = "selected_item";
    static constexpr const char *tv_active_item = "active_item";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char* tv_border_item = "border_item";
    static constexpr const char* tv_item_indent = "item_indent";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

    static constexpr int32_t select_horizontal_indent = 5;
    static constexpr int32_t item_height__ = 32;

private:
    std::vector<select_item> items_;

    std::function<void(int32_t n_item, int64_t id)> change_callback;

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_{};

    std::weak_ptr<window> parent_;
    std::string my_control_sid, my_plain_sid;

    std::shared_ptr<i_theme> list_theme;
    std::shared_ptr<list> list_;

    bool showed_{ true }, enabled_{ true }, active_{ false }, topmost_{ false };
    bool focused_{ false };
    bool focusing_{ true };

    int32_t left_shift{ 0 };

    int32_t item_height_{ item_height__ };

    void receive_control_events(const event &ev);
    void receive_plain_events(const event &ev);

    void update_list_theme();

    void draw_arrow_down(graphic &gr, const rect pos);

    void select_up();
    void select_down();

    void show_list();

    void draw_list_item(graphic &gr, const int32_t n_item, const rect item_rect_, const list::item_state state);
    void activate_list_item(const int32_t n_item);
    void change_list_item(const int32_t n_item);
};

}
