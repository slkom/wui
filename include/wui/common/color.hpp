//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <cstdint>
#include <cmath>

namespace wui
{

// TODO: Little-Endian/Big-Endian

/// wui *.json use XRGB/BGRX format: add alpha BGRA/ARGB

/// BGRA/ARGB, Gdi++,Cairo,Vulkan (any,swapchain)

/// RGBA/ABGR : HTML5 Canvas,CSS,OpenGL(default),Vulkan(any, texture),
/// Web,Android,PNG,Skia,Impeller,FreeType/MSDF

/// #define COLOR_RGBA_FORMAT /// configure your backend (if necessary)

///your code, sample:
///#if defined(_OPENGL_BACK)
///#define COLOR_RGBA_FORMAT
///#else
///#endif

typedef uint32_t color;

namespace detail
{

/// make color RGBA, A = 0
constexpr inline uint32_t make_color_rgb0(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return (red | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint32_t>(blue) << 16);
}

/// make color RGBA, A = 255
constexpr inline uint32_t make_color_rgb(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return make_color_rgb0(red, green, blue) | 0xFF000000U;
}

/// make color RGBA
constexpr inline uint32_t make_color_rgba(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) noexcept
{
    return make_color_rgb0(red, green, blue) | (static_cast<uint32_t>(alpha) << 24);
}

/// make color BGRA, A = 0
constexpr inline uint32_t make_color_bgr0(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return blue | (static_cast<uint16_t>(green) << 8) | (static_cast<uint32_t>(red) << 16);
}

/// make color BGRA, A = 255
constexpr inline uint32_t make_color_bgr(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return make_color_bgr0(red, green, blue) | 0xFF000000U;
}

/// make color BGRA
constexpr inline uint32_t make_color_bgra(const uint8_t red, const uint8_t green, const uint8_t blue,
    const uint8_t alpha) noexcept
{
    return make_color_bgr0(red, green, blue) | (static_cast<uint32_t>(alpha) << 24);
}

/// BGRA to RGBA, A = 0
constexpr inline uint32_t conv_bgra_to_rgb0(const uint32_t bgra) noexcept
{
    return ((bgra & 0x000000FFU) << 16) | (bgra & 0x0000FF00U) | ((bgra & 0x00FF0000U) >> 16);
}

/// BGRA to RGBA, A = 255
constexpr inline uint32_t conv_bgra_to_rgb(const uint32_t bgra) noexcept
{
    return conv_bgra_to_rgb0(bgra) | 0xFF000000U;
}

/// BGRA to RGBA
constexpr inline uint32_t conv_bgra_to_rgba(const uint32_t bgra) noexcept
{
    return conv_bgra_to_rgb0(bgra) | (bgra & 0xFF000000U);
}

/// RGBA to BGRA, A = 0
constexpr inline uint32_t conv_rgba_to_bgr0(const color rgba) noexcept
{
    return ((rgba & 0x000000FFU) << 16) | (rgba & 0x0000FF00U) | ((rgba & 0x00FF0000U) >> 16);
}

/// RGBA to BGRA, A = 255
constexpr inline uint32_t conv_rgba_to_bgr(const uint32_t rgba) noexcept
{
    return conv_rgba_to_bgr0(rgba) | 0xFF000000U;
}

/// RGBA to BGRA
constexpr inline uint32_t conv_rgba_to_bgra(const uint32_t rgba) noexcept
{
    return conv_rgba_to_bgr0(rgba) | (rgba & 0xFF000000U);
}

} // detail

/// color to A
constexpr inline uint8_t get_alpha(const uint32_t c) noexcept
{
    return (c >> 24) & 0x000000FFU;
}

constexpr inline uint32_t set_alpha(const uint32_t c, const uint8_t alpha) noexcept
{
    return (c & 0x00FFFFFFU) | (static_cast<uint32_t>(alpha) << 24);
}

/// color is use alpha
constexpr inline uint8_t is_alpha(const uint32_t c) noexcept
{
    return 255U != get_alpha(c);
}

/// color to G
constexpr inline uint8_t get_green(const uint32_t c) noexcept
{
    return (c >> 8) & 0x000000FFU;
}

namespace detail
{
/// color to G
constexpr inline uint8_t get_green_bgra(const uint32_t c) noexcept
{
    return get_green(c);
}

} // detail

#ifndef COLOR_RGBA_FORMAT

// BGRA pixel format

/// make color BGRA, A = 255
constexpr inline color make_color3(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return detail::make_color_bgr0(red, green, blue);
}

/// make color BGRA, A = 255
constexpr inline color make_color(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return detail::make_color_bgr(red, green, blue);
}

/// make color BGRA
constexpr inline color make_color(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) noexcept
{
    return detail::make_color_bgra(red, green, blue, alpha);
}

/// BGRA to RGBA, A = 0
constexpr inline uint32_t get_rgb0(const color bgra) noexcept
{
    return detail::conv_bgra_to_rgb0(bgra);
}

/// BGRA to RGBA, A = 0xFF
constexpr inline uint32_t get_rgb(const color bgra) noexcept
{
    return detail::conv_bgra_to_rgb(bgra);
}

/// BGRA to BGRA, A = 0
constexpr inline color conv_to_bgr0(const color c) noexcept
{
    return (c & 0x00FFFFFFU);
}

/// BGRA to BGRA, A = 255
constexpr inline color conv_to_bgr(const color c) noexcept
{
    return 0xFF000000U | (c & 0x00FFFFFFU);
}

constexpr inline color conv_to_bgra(const color c) noexcept
{
    return c;
}

/// BGRA to R
constexpr inline uint8_t get_red(const color bgra) noexcept
{
    return (bgra >> 16) & 0x000000FFU;
}

/// BGRA to B
constexpr inline uint8_t get_blue(const color bgra) noexcept
{
    return bgra & 0x000000FFU;
}

namespace detail
{
/// wui *.json use BGRA format
/// setup wui *.json data to current color format
constexpr inline color setup_color_alpha(const color bgra) noexcept
{
    return (0 != get_alpha(bgra) ? bgra : (bgra & 0x00FFFFFFU) | 0xFF000000U);
}

/// BGRA to R
constexpr inline uint8_t get_red_bgra(const color bgra) noexcept
{
    return get_red(bgra);
}

/// BGRA to B
constexpr inline uint8_t get_blue_bgra(const color bgra) noexcept
{
    return get_blue(bgra);
}

} // detail

#else // RGBA

/// make color RGBA, A = 255
constexpr inline color make_color(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return detail::make_color_rgb(red, green, blue);
}

/// make color RGBA
constexpr inline color make_color(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) noexcept
{
    return detail::make_color_rgba(red, green, blue, alpha);
}

/// RGBA to BGRA, A = 0
constexpr inline uint32_t conv_to_bgr0(const color rgba) noexcept
{
    return detail::conv_rgba_to_bgr0(rgba);
}

/// RGBA to BGRA, A = 255
constexpr inline uint32_t conv_to_bgr(const color rgba) noexcept
{
    return detail::conv_rgba_to_bgr(rgba);
}

/// RGBA to BGRA
constexpr inline uint32_t conv_to_bgra(const color rgba) noexcept
{
    return detail::conv_rgba_to_bgra(rgba);
}

/// RGBA to RGBA, A = 0
constexpr inline color get_rgb0(const color rgba) noexcept
{
    return rgba & 0x00FFFFFFU;
}

/// RGBA to RGBA, A = 0xFF
constexpr inline color get_rgb(const color rgba) noexcept
{
    return 0xFF000000U | (rgba & 0x00FFFFFFU);
}

/// RGBA to R
constexpr inline uint8_t get_red(const color rgba) noexcept
{
    return rgba & 0x000000FFU;
}

/// RGBA to B
constexpr inline uint8_t get_blue(const color rgba) noexcept
{
    return (rgba >> 16) & 0x000000FFU;
}

namespace detail
{
/// wui *.json use BGRA format
/// setup wui *.json data to current color format
constexpr inline color setup_color_alpha(const uint32_t bgra) noexcept
{
    return (0 != get_alpha(bgra) ? conv_bgra_to_rgba(bgra) : conv_bgra_to_rgb(bgra));
}

/// BGRA to R
constexpr inline uint8_t get_red_bgra(const uint32_t bgra) noexcept
{
    return (bgra >> 16) & 0x000000FFU;
}

/// BGRA to B
constexpr inline uint8_t get_blue_bgra(const uint32_t bgra) noexcept
{
    return bgra & 0x000000FFU;
}

} // detail

#endif // COLOR_RGBA_FORMAT

/// Common color func

/// xxx[A * v]
/// v: [0, 1]
constexpr inline uint32_t change_alpha(const uint32_t c, const float v = 0.6f) noexcept
{
    return (c & 0x00FFFFFFU) | ((static_cast<uint32_t>(get_alpha(c) * v) << 24) & 0xFF000000U);
}

// Линеаризация канала sRGB
constexpr inline float inverse_color_gamma(const float ch) noexcept
{
    return (ch <= 0.04045f) ? (ch / 12.92f) : std::pow((ch + 0.055f) / 1.055f, 2.4f);
}

/// Relative Luminance) BT.709
constexpr inline float get_luminance(const uint32_t c) noexcept
{
    const float r = inverse_color_gamma(get_red(c) / 255.0f);
    const float g = inverse_color_gamma(get_green(c) / 255.0f);
    const float b = inverse_color_gamma(get_blue(c) / 255.0f);
    const float a = get_alpha(c) / 255.0f;
    return (0.2126f * r + 0.7152f * g + 0.0722f * b) * a;
}

constexpr inline bool is_lighter_than(const uint32_t c, const uint32_t c_c) noexcept
{
    return get_luminance(c) >= get_luminance(c_c);
}

namespace detail
{

constexpr float _blend_factor = 0.5f;

} // detail

/// target [0,255U]
/// factor: [0, 1]
constexpr inline uint8_t blend_channel(const uint8_t ch, const uint8_t target, const float factor)
{
    return static_cast<uint8_t>(ch + factor * (target - ch));
}

/// factor [0, 1] : 0 - без изменений, 1 - белый
constexpr inline uint32_t lighten_color(const uint32_t c, const float factor = detail::_blend_factor)
{
    const uint8_t r = get_red(c);
    const uint8_t g = get_green(c);
    const uint8_t b = get_blue(c);
    return make_color(blend_channel(r, 255U, factor), blend_channel(g, 255U, factor), blend_channel(b, 255U, factor))
        | (c & 0xFF000000U);
}

/// factor [0, 1] : 0 - без изменений, 1 - черный
constexpr inline uint32_t darken_color(const uint32_t c, const float factor = detail::_blend_factor)
{
    const uint8_t r = get_red(c);
    const uint8_t g = get_green(c);
    const uint8_t b = get_blue(c);
    return make_color(blend_channel(r, 0U, factor), blend_channel(g, 0U, factor), blend_channel(b, 0U, factor))
        | (c & 0xFF000000U);
}

/// factor [0, 1] : 0 - без изменений, 1 - черный или белый.
/// Затемняет, если цвет c_c для сравнения темнее и обратно.
/// Используется для определения цвета рамки относительно подложки.
constexpr inline uint32_t change_color_if(const uint32_t c, const uint32_t c_c,
    const float factor = detail::_blend_factor) noexcept
{
    const uint8_t r = get_red(c);
    const uint8_t g = get_green(c);
    const uint8_t b = get_blue(c);
    const uint8_t target = is_lighter_than(c, c_c) ? 0U : 0xFFU;
    return make_color(blend_channel(r, target, factor), blend_channel(g, target, factor),
        blend_channel(b, target, factor)) | (c & 0xFF000000U);
}

}
