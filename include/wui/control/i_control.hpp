//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/common/error.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace wui
{

struct rect;
class graphic;
class window;
class i_theme;

/// default_: фокус не запоминается на выбранном input контроле, поведение окна по умолчанию.
/// free: используется для single-line input
///     Для single-line input фокус теряется по нажатию `return` или по свободному полю окна.
/// fixed: при использовании флага react, фокус возвращается в выбранный multi-line input контроль
///     по нажатию `return` для single-line input или по свободному полю окна.
/// wnd: флаг для окон, используется для быстрого определения окна и его focus mode.
/// react: используется только для класса window - фокус не возвращается в выбранный input контроль автоматически.
///        Для перемещения фокуса потребуется использовать выбор контроля [key TAB] или щечек по свободному месту окна.
/// always: поведение редактора текста. Используется только для класса window -
///     после выполнения задачи контроля, фокус возвращается в выбранный multi-line input
///     всегда.
///     Выбор контроля [key TAB] так же доступен. [Ctrl+Return] для next Input так же доступен.
enum class focus_mode : uint32_t
{
    default_ = (1 << 0), /// default mode, user-focused only
    free = (1 << 1), /// flag for single-line input
    fixed = (1 << 2), /// flag for multi-line input
    wnd = (1 << 3), /// flag for window, to control window focus
    react = (1 << 4), /// flag for window - multi-line input mode: respond to the user's desires to select input
    always = (1 << 5), /// flag for window - editor behavior, multi-line input mode

    input_set = free | fixed, // for check single/multi-line input focus mode
    react_set = react | always
};

inline constexpr focus_mode operator|(const focus_mode l, const focus_mode r)
{
    return static_cast <focus_mode> (static_cast <uint32_t> (l) | static_cast <uint32_t> (r));
}
inline constexpr bool operator&(const focus_mode l, const focus_mode r)
{
    return 0 != (static_cast <uint32_t> (l) & static_cast <uint32_t> (r));
}

class i_control
{
public:
    virtual void draw(graphic &gr, const rect& paint_rect) = 0;

    virtual void set_position(const rect& position) = 0;
    [[nodiscard]] virtual rect position() const = 0;

    virtual void move(const int32_t dx, const int32_t dy) = 0;

    virtual void set_parent(std::shared_ptr<window> window_) = 0;
    [[nodiscard]] virtual std::weak_ptr<window> parent() const = 0;
    virtual void clear_parent() = 0;

    virtual void set_topmost(bool yes) = 0;
    [[nodiscard]] virtual bool topmost() const = 0;

    virtual void update_theme_control_name(std::string_view theme_control_name) = 0;
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    [[nodiscard]] virtual bool showed() const = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;
    [[nodiscard]] virtual bool enabled() const = 0;

    [[nodiscard]] virtual bool focused() const = 0;  /// Returns true if the control is focused
    [[nodiscard]] virtual bool focusing() const = 0; /// Returns true if the control receives focus

    [[nodiscard]] virtual focus_mode get_focus_mode() const
    {
        return focus_mode::default_;
    };

    [[nodiscard]] virtual error get_error() const = 0;

    friend class window;

protected:
    virtual ~i_control() {}

};

}
