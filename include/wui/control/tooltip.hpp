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
#include <wui/common/rect.hpp>
#include <wui/common/color.hpp>

#include <string>
#include <functional>
#include <memory>

namespace wui
{

class tooltip : public i_control, public std::enable_shared_from_this<tooltip>
{
public:
    explicit tooltip(std::string_view text, std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~tooltip() override;

    void draw(graphic &gr, const rect ) override;
    void redraw(bool clear = false) override;

    void set_position(const rect position) override;
    [[nodiscard]] virtual rect position() const override;
    void move(const int32_t dx, const int32_t dy) override;

    void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] std::weak_ptr<window> parent() const override;
    void clear_parent() override;

    void set_topmost(bool) override;
    [[nodiscard]] bool topmost() const override;

    void update_theme_control_name(std::string_view theme_control_name) override;
    void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    void show() override;
    void hide() override;
    [[nodiscard]] virtual bool showed() const override;

    void enable() override;
    void disable() override;
    [[nodiscard]] bool enabled() const override;

    [[nodiscard]] bool focused() const override;
    [[nodiscard]] bool focusing() const override;

    [[nodiscard]] error get_error() override;

    /// Tooltip's interface
    void show_on_control(i_control &control, int32_t indent);

    void set_text(std::string_view text_);

    /// Control name in theme
    static constexpr const char *tc = "tooltip";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_text_indent = "text_indent";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_font = "font";

private:
    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    std::string text;

    rect position_{};

    std::weak_ptr<window> parent_;

    bool showed_{ false };

    void update_size();

};

}
