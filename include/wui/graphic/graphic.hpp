//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/system/system_context.hpp>

#include <wui/common/color.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>

#include <wui/graphic/primitive_container.hpp>

#include <string_view>
#include <cstdint>
#include <vector>

#ifdef __linux__
struct _cairo_surface;
struct _cairo_device;
#endif

namespace wui
{

class graphic
{
public:

    struct text_line
    {
        std::string str;
        rect rc{ };  /// local coordinate
    };
    typedef std::vector<text_line> text_lines_t;

    graphic(system_context &context);
    graphic() = default;
    ~graphic();

    [[nodiscard]] bool init(const rect& max_size,
        const color background_color, const bool clear_ = true);
    void release();
    [[nodiscard]] bool inited() const noexcept;

    [[nodiscard]] rect max_size() const noexcept;

    void set_background_color(color background_color, const bool clear_ = true);

    void clear(const rect& position = { 0 });

    void flush(const rect& updated_size);

    void draw_pixel(const rect& position, const color color_);

    ///TODO, NB: linux width = 1 always.
    void draw_line(const rect& position, const color color_, const int32_t width = 1);

    /// <summary>
    /// get font height for lines spacing
    /// </summary>
    /// <param name="font__"></param>
    /// <returns>height</returns>
    [[nodiscard]] int32_t get_font_ideal_height(const font& font__);

#ifdef _WIN32
    /// <summary>
    /// GDI++ : get font height for lines spacing
    /// </summary>
    /// <param name="font__"></param>
    /// <returns>height</returns>
    [[nodiscard]] int32_t get_font_ideal_height_gdiplus(const font& font__);
#endif

    [[nodiscard]] rect measure_text(std::string_view text_, const font &font__);
#ifdef _WIN32
    [[nodiscard]] rect measure_text_gdiplus(std::string_view text_, const font &font__);
#endif

    void draw_text(const rect& position, std::string_view text,
        const color color_, const font &font_);

    /// support clip and alpha
    void draw_text_clip(const rect& position,
        const text_lines_t& lines_data,
        const color color_, const font& font__, const bool clip_);

#ifdef _WIN32
    void draw_text_clip_rgb(const rect& position, const text_lines_t& lines,
        const color color_, const font& font__, const bool clip_);
#elif __linux__
    void draw_text_clip_rgb(const rect& position, const text_lines_t& lines,
        const color color_, const font& font__, const bool clip_)
    {
        draw_text_clip(position, lines, get_rgb_opaqui(color_), font__, clip_);
    }
#endif

    void draw_rect(const rect& position, const color fill_color);
    void draw_rect(const rect& position, const color border_color,
        const color fill_color, const int32_t border_width, const int32_t round);

    /// draw some buffer on context
    void draw_buffer(const rect& position, uint8_t *buffer, const int32_t left_shift,
        const int32_t top_shift);

    /// deprecated : old version used (see ver. <= 1.3.260401) call
    ///  draw_graphic({ control_pos.left, control_pos.top, control_pos.width(), control_pos.height() }, );
    [[deprecated("deprecated, use `copy_area(control_pos, )`")]]
    void draw_graphic(const rect& position, graphic &graphic_,
        const int32_t left_shift, const int32_t top_shift)
    {
        copy_area({ position.left, position.top,
                position.left + position.right, // left + width
                position.top + position.bottom  // top + height
            }, graphic_, left_shift, top_shift);
    }

    /// draw another graphic area on context
    void copy_area(const rect& position, graphic &graphic_,
        const int32_t left_shift, const int32_t top_shift);

#ifdef _WIN32
    [[nodiscard]] HDC drawable()
    {
        return mem_dc;
    }

#elif __linux__
    [[nodiscard]] xcb_drawable_t drawable()
    {
        return mem_pixmap;
    }

    /// workaround on linux
    void draw_surface(_cairo_surface &surface, const rect& position, const bool grayscale = false);
#endif

    [[nodiscard]] error get_error() const;

    static void set_text_measurer(graphic* gr) noexcept;
    [[nodiscard]] static graphic* get_text_measurer() noexcept;
    [[nodiscard]] static bool text_measurer_inited() noexcept;
    [[nodiscard]] static bool is_me_text_measurer(const graphic &gr) noexcept;

#ifdef _WIN32
    HBITMAP mem_bitmap;
#endif
private:
#ifdef _WIN32
    HGDIOBJ mem_bitmap_old;
#endif

    system_context &context_;

    primitive_container pc;

    rect max_size_;

    color background_color;

#ifdef _WIN32
    HDC mem_dc;
#elif __linux__
    xcb_pixmap_t mem_pixmap;

    _cairo_surface *surface;
    _cairo_device *device; // not used
#endif

    error err;
};

//для совместимости с wui-1.3.260215 example simple
[[deprecated("The first WUI::window during initialization creates a graphical context.")]]
void init_text_measurer(graphic* gr) noexcept;

[[nodiscard]] rect measure_text(std::string_view text, const font &font_, graphic *gr = nullptr);
/// measure text, hash not use
[[nodiscard]] rect measure_text_direct(std::string_view text, const font &font_, graphic *gr = nullptr);

[[nodiscard]] int32_t get_font_ideal_height(const font& font_, graphic* gr);

#ifdef _WIN32
[[nodiscard]] int32_t get_font_ideal_height_gdiplus(const font& font_, graphic* gr);
[[nodiscard]] rect measure_text_gdiplus(std::string_view text, const font &font_, graphic *gr = nullptr);
/// measure text, hash not use
[[nodiscard]] rect measure_text_gdiplus_direct(std::string_view text, const font &font_, graphic *gr = nullptr);
#endif


}
