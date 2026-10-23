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
#include <wui/common/alignment.hpp>
#include <wui/theme/theme.hpp>

#include <string>
#include <memory>
#include <vector>

#ifdef _UI_CHECK
// если _UI_CHECK задана при компиляции внешнего кода,
// это приведет к ошибке при запуске программы при рассогласовании полей классов
#   undef _UI_CHECK
#else
// отладочный параметр, пока только для text
#ifndef NDEBUG
//# define _UI_CHECK
#endif
#endif

namespace wui
{

/// abstract class
class a_text : public i_control
{
public:
    a_text(std::string_view text__,
        hori_alignment hori_alignment__,
        vert_alignment vert_alignment__,
        std::string_view theme_control_name,
        std::shared_ptr<i_theme> theme__,
        bool clip__);

protected:
    virtual ~a_text() = 0;

public:
    void draw(graphic& gr, const rect) override;
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

    /// Text's interface

    // избегаем лишних redraw
    template <bool t_redraw = true>
    inline void set_text(std::string_view text__)
    {
        text_ = text__;
        update_ = true;
        if constexpr (t_redraw)
        {
            redraw();
        }
    }

    template <bool t_redraw = true>
    inline void clear()
    {
        text_.clear();
        update_ = true;
        lines_.clear();
#ifdef _UI_CHECK
        text_position_.clear();
#endif
        if constexpr (t_redraw)
        {
            redraw();
        }
    }

    template <bool t_redraw = false>
    void set_text_position(const rect position)
    {
        position_ = position;
        update_ = true;
        if constexpr (t_redraw)
        {
            redraw();
        }
    }

    void set_alignment(hori_alignment hori_alignment_,
        vert_alignment vert_alignment_);

    std::string_view get_text() const;

    /// set line spacing coefficient, >= 1.0: spacing = font_height * value
    /// default 1.0, CSS 1.2
    void set_space_coeff(const double coeff) noexcept;

    /// get default line spacing coefficient
    [[nodiscard]] static constexpr double get_default_space_coeff() noexcept
    {
        return _default_space_coeff;
    }

    /// get line spacing value
    double get_space_coeff() const noexcept;

    /// returns the preferred text size
    [[nodiscard]] rect get_preferred_size();

    /// Returns the given font size
    [[nodiscard]] int32_t get_font_size() const
    {
        return theme_font(tcn, tv_font, theme_).size;
    }

    /// recommended distance between the baselines of two adjacent lines of text
    [[nodiscard]] int32_t font_line_height()
    {
        return static_cast<int32_t>(0.5 + space_coeff_
            * font_line_height_internal(theme_font(tcn, tv_font, theme_), nullptr, nullptr));
    }

    [[nodiscard]] bool get_clip_state() const noexcept
    {
        return clip_;
    }

    void set_clipping(const bool clip__)
    {
        clip_ = clip__;
        update_ = true;
        redraw();
    }

    /// Control name in theme
    static constexpr const char* tc = "text";

    /// Used theme values
    static constexpr const char* tv_color = "color";
    static constexpr const char* tv_font = "font";

protected:

    static constexpr double _default_space_coeff = 1.0; // 1.2 CSS

    /// recommended distance between the baselines of two adjacent lines of text
    virtual int32_t font_line_height_internal(const font& font_,
        graphic* gr, int32_t *max_height) = 0;

    /// return text width
    [[nodiscard]] virtual int32_t measure_text_line(const std::string& text__,
        const font& font__) = 0;

    /// truncate the text, if necessary. return text width
    [[nodiscard]] virtual int32_t truncate_and_measure_text(std::string& text__,
        const font& font__, const int32_t width__,
        graphic* gr = nullptr) = 0;

    virtual void draw_text(graphic& gr) = 0;

    static void make_lines(const std::string_view text,
        std::vector<std::string>& lines);

    /// обновляет список строк, рассчитывает позицию строк
    void update_text(graphic* gr, const bool clip__);

#ifdef _UI_CHECK
    /// возвращает позицию текста
    rect position_text() const;

    /// область текста, занятая при рисовании
    rect text_position_{ };
#endif

    graphic::text_lines_t lines_; // local coordinate

    /// используется для определения количества строк
    rect position_{ };

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    std::weak_ptr<window> parent_{ };

    bool showed_{ true };
    bool topmost_{ false };
    bool update_{ true };

    std::string text_;

    double space_coeff_{ _default_space_coeff }; //! line spacing = font_line_height() * coef

    hori_alignment hori_alignment_;
    vert_alignment vert_alignment_;

    bool clip_;
};

/// support clip
class text : public a_text, public std::enable_shared_from_this<text>
{
public:
    explicit text(std::string_view text__ = "",
        hori_alignment hori_alignment__ = hori_alignment::left,
        vert_alignment vert_alignment__ = vert_alignment::center,
        std::string_view theme_control_name = tc,
        std::shared_ptr<i_theme> theme__ = nullptr,
        bool clip__ = true);

    ~text() override;

protected:
    /// recommended distance between the baselines of two adjacent lines of text
    int32_t font_line_height_internal(const font& font_,
        graphic* gr, int32_t* max_height) override;

    /// return text width
    [[nodiscard]] int32_t measure_text_line(const std::string& text__,
        const font& font__) override;

    /// return text width
    [[nodiscard]] int32_t truncate_and_measure_text(std::string& text__,
        const font& font__, const int32_t width__,
        graphic* gr = nullptr) override;

    void draw_text(graphic& gr) override;
};

/// support alpha channel and clip
class text_ex : public a_text, public std::enable_shared_from_this<text_ex>
{
public:
    explicit text_ex(std::string_view text__ = "",
        hori_alignment hori_alignment__ = hori_alignment::left,
        vert_alignment vert_alignment__ = vert_alignment::center,
        std::string_view theme_control_name = tc,
        std::shared_ptr<i_theme> theme__ = nullptr,
        bool clip__ = true);

    ~text_ex() override;

protected:
    /// recommended distance between the baselines of two adjacent lines of text
    int32_t font_line_height_internal(const font& font_,
        graphic* gr, int32_t* max_height) override;

    /// return: text width
    [[nodiscard]] int32_t measure_text_line(const std::string& text__,
        const font& font__) override;

    /// return: text width
    [[nodiscard]] int32_t truncate_and_measure_text(std::string& text__,
        const font& font__, const int32_t width__,
        graphic* gr = nullptr) override;

    void draw_text(graphic& gr) override;
};

}
