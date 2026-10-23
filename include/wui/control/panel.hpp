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

class panel : public i_control, public std::enable_shared_from_this<panel>
{
public:
    explicit panel(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    explicit panel(std::function<void(graphic&)> draw_callback, std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~panel() override;

    void draw(graphic &gr, const rect) override;
    void redraw(bool clear = false) override;

    void set_position(const rect position) override;
    [[nodiscard]] rect position() const override;
    void move(const int32_t dx, const int32_t dy) override;

    void add_control(std::shared_ptr<i_control> control);

    void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] std::weak_ptr<window> parent() const override;
    void clear_parent() override;

    void set_topmost(bool yes) override;
    [[nodiscard]] bool topmost() const override;

    void update_theme_control_name(std::string_view theme_control_name) override;
    void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    void show() override;
    void hide() override;
    [[nodiscard]] virtual bool showed() const override;

    virtual void enable() override;
    virtual void disable() override;
    [[nodiscard]] bool enabled() const override;

    [[nodiscard]] bool focused() const override;
    [[nodiscard]] bool focusing() const override;

    [[nodiscard]] error get_error() override;

    /// Control name in theme
    static constexpr const char *tc = "panel";

    /// Used theme values
    static constexpr const char *tv_background = "background";

private:
    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_{};

    std::vector<std::shared_ptr<i_control>> controls;

    std::weak_ptr<window> parent_;

    bool showed_{ true }, topmost_{ false };

    std::function<void(graphic&)> draw_callback;
};

}
