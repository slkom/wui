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
#include <wui/common/orientation.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>

#include <wui/graphic/primitive_container.hpp>

#include <string_view>
#include <cstdint>
#include <vector>
#include <memory>

#ifdef __linux__
struct _cairo_surface;
struct _cairo_device;
struct _cairo_region;
#endif

#define _CAIRO_BAK_SIMILAR

namespace wui
{

class graphic
{
public:

    struct point2i
    {
        int32_t x{}, y{};

    };
    struct size2i
    {
        int32_t width{}, height{};
    };

    struct text_line
    {
        rect rc{};  /// local coordinate
        std::string str;
    };
    typedef std::vector<text_line> text_lines_t;

    explicit graphic(system_context &context);
    //graphic() = default;
    ~graphic();

#if !defined(_WIN32)
    void set_target(const graphic& target_gr_) noexcept
    {
        target_surface = target_gr_.surface;
screen_surface_use = false;
    }

    bool resize(const int32_t width, const int32_t height, const bool clear_ = true);
#endif

    [[nodiscard]] bool init(const int32_t width, const int32_t height, const color background_color, const bool clear_ = true);

    // הכÿ סמגלוסעטלמסעט
    [[nodiscard]] bool init(const rect max_size, const color background_color_, const bool clear_ = true)
    {
        return init(max_size.width(), max_size.height(), background_color_, clear_);
    }

    void release();

    [[nodiscard]] bool inited() const noexcept
    {
#ifdef _WIN32
        return NULL != mem_bitmap;
#elif __linux__
        return nullptr != surface;
#endif
    }

    [[nodiscard]] rect size() const noexcept
    {
        return { 0, 0, width_, height_ };
    }
    [[nodiscard]] int32_t width() const noexcept
    {
        return width_;
    }
    [[nodiscard]] int32_t height() const noexcept
    {
        return height_;
    }

    void set_background_color(color background_color, const bool clear_ = true);

    void clear(const rect position = { 0 });

    void flush(const rect updated_size);

    void draw_pixel(const int32_t x, const int32_t y, const color color_);

    void draw_pixel(const rect position, const color color_)
    {
        draw_pixel(position.left, position.top, color_);
    }

    void draw_lines(const rect* rcs, uint32_t size, const color color_, const int32_t width,
        const bool antialias_none = true);

    void draw_line(const rect position, const color color_, const int32_t width = 1,
        const bool antialias_none = true);

    /// <summary>Get font height for lines spacing </summary>
    /// <param name="font_">font</param>
    /// <param name="gr">graphic*</param>
    /// <param name="max_height">ptr, font max height GDI/GDI++</param>
    /// <returns> recommended distance between the baselines of two adjacent lines of text </returns>
    [[nodiscard]] int32_t font_line_height(const font& font_, int32_t *max_height = nullptr);

#ifdef _WIN32
    /// <summary> GDI++ : get font height for lines spacing</summary>
    /// <param name="font_"></param>
    /// <returns>lines spacing height</returns>
    [[nodiscard]] int32_t font_line_height_gdiplus(const font& font_);
#endif

    /// <summary>Get font height (max)</summary>
    /// <param name="font_">font</param>
    /// <returns>height</returns>
    [[nodiscard]] int32_t font_max_height(const font& font_);

    /// <summary> measure text, if error: return [0, 0, 0, font size] </summary>
    /// <param name="text">string</param>
    /// <param name="font_">font</param>
    /// <returns>rect [0, 0, width, height]</returns>
    [[nodiscard]] rect measure_text(std::string_view text_, const font &font_);

#ifdef _WIN32
    /// <summary> measure text, GDI++, if error: return [0, 0, 0, font size] </summary>
    /// <param name="text">string</param>
    /// <param name="font_">font</param>
    /// <returns>rect [0, 0, width, height]</returns>
    [[nodiscard]] rect measure_text_gdiplus(std::string_view text_, const font &font_);
#endif

    void draw_text(const rect position, std::string_view text,
        const color color_, const font &font_);

    /// support clip and alpha [win32 use gdi++]
    void draw_text_clip(const rect position,
        const text_lines_t& lines_data,
        const color color_, const font& font_, const bool clip_ = true);

#ifdef _WIN32
    void draw_text_clip_rgb(const rect position, const text_lines_t& lines,
        const color color_, const font& font_, const bool clip_ = true);
#elif __linux__
    void draw_text_clip_rgb(const rect position, const text_lines_t& lines,
        const color color_, const font& font_, const bool clip_ = true)
    {
        draw_text_clip(position, lines, get_rgb(color_), font_, clip_);
    }
#endif

    void draw_rect(const rect position, const color fill_color);
    void draw_rect(const rect position, const color border_color,
        const color fill_color, const int32_t border_width, int32_t rnd);

    void fill_path(const rect position, const point2i* pts, const int n, const color color_,
        const bool antialias_none = true);

    /// draw some buffer on context, default stride_data meets the required
    void draw_buffer(const rect position, uint8_t *buffer, const int32_t left_shift,
        const int32_t top_shift, const int32_t stride_data = 0);

    /// draw another graphic area on context
    void copy_area(const rect position, graphic &gr_src,
        const int32_t left_shift, const int32_t top_shift);

    /// deprecated: added for compatibility, for old version used (see ver. <= 1.3.260401).
    /// Cause: bad arguments.
    /// Current call right: draw_graphic({ control_pos.left, control_pos.top, control_pos.width(), control_pos.height() }, );
    [[deprecated("deprecated, use `copy_area(control_pos, )`")]]
    void draw_graphic(const rect position, graphic &graphic_,
        const int32_t left_shift, const int32_t top_shift)
    {
        copy_area({ position.left, position.top,
                position.left + position.right, // left + width
                position.top + position.bottom  // top + height
            }, graphic_, left_shift, top_shift);
    }

#ifdef _WIN32
    [[nodiscard]] HDC drawable()
    {
        return mem_dc;
    }

#elif __linux__

    void dirty_region_union(const rect rc);
    [[nodiscard]] rect get_dirty_region_extents();

    void surface_flush();

    /// workaround on linux
    void draw_surface(_cairo_surface* surface, const rect position, const bool grayscale = false);
#endif

    [[nodiscard]] error get_error();

    static void set_text_measurer(graphic* gr) noexcept;
    [[nodiscard]] static graphic* get_text_measurer() noexcept;
    [[nodiscard]] static bool text_measurer_inited() noexcept;
    [[nodiscard]] static bool is_me_text_measurer(const graphic &gr) noexcept;

    [[nodiscard]] inline static uint32_t clamp_corner_radius(const rect bounds, const uint32_t requested)
    {
        if (bounds.width() <= 0 || bounds.height() <= 0)
            return 0;
        const auto shortest = bounds.width() < bounds.height() ? bounds.width() : bounds.height();
        const auto maximum = static_cast<uint32_t>(shortest / 2);
        return requested < maximum ? requested : maximum;
    }

private:
#ifdef _WIN32
    HBITMAP mem_bitmap{ NULL };
    HGDIOBJ mem_bitmap_old{ NULL };
#endif

    system_context &context_;

    primitive_container pc;

    int32_t width_{}, height_{};

    color background_color{ make_color(0, 0, 0, 0) };

#ifdef _WIN32
    HDC mem_dc{NULL};
#elif __linux__
    _cairo_surface* screen_surface{ nullptr };
    _cairo_surface* target_surface{ nullptr };
    _cairo_surface* surface{ nullptr };
    _cairo_region* dirty_region{ nullptr };

bool screen_surface_use{ }; // TODO tmp
#endif

    error err;
};

/// Left for compatibility wui-1.3.260215
/// Cause: The first WUI::window during initialization creates a set measurer graphical context.
[[deprecated("The first WUI::window during initialization creates a graphical context.")]]
inline void init_text_measurer(graphic*) noexcept
{}

/// <summary> measure text, if error: return [0, 0, 0, font size] </summary>
/// <param name="text">string</param>
/// <param name="font_">font</param>
/// <param name="gr">graphic</param>
/// <returns>rect [0, 0, width, height]</returns>
[[nodiscard]] rect measure_text(std::string_view text, const font &font_, graphic *gr = nullptr);

/// measure text, hash not use
[[nodiscard]] rect measure_text_direct(std::string_view text, const font &font_, graphic *gr = nullptr);

/// <summary>
/// get font height for lines spacing
/// </summary>
/// <param name="font_">font</param>
/// <param name="gr">graphic*</param>
/// <param name="max_height">ptr, font max height GDI/GDI++</param>
/// <returns>height (lines spacing)</returns>
[[nodiscard]] int32_t font_line_height(const font& font_, graphic* gr = nullptr, int32_t* max_height = nullptr);

/// <summary>
/// get font height GDI/GDI++ (max)
/// </summary>
/// <param name="font_">font</param>
/// <param name="gr">graphic ptr</param>
/// <returns>height</returns>
[[nodiscard]] int32_t font_max_height(const font& font_, graphic* gr = nullptr);

#ifdef _WIN32
[[nodiscard]] int32_t font_line_height_gdiplus(const font& font_, graphic* gr);
[[nodiscard]] rect measure_text_gdiplus(std::string_view text, const font &font_, graphic *gr = nullptr);
/// measure text, hash not use
[[nodiscard]] rect measure_text_gdiplus_direct(std::string_view text, const font &font_, graphic *gr = nullptr);
#endif


}
