//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <cstdint>

namespace wui
{

/// Cairo. RGBA is opaque black (0.0, 0.0, 0.0, 1.0).
/// Gdiplus. ARGB is opaque black (255, 0.0, 0.0, 0.0)
/// wui *.json use BGRA format: new opaque and alpha

typedef uint32_t color; /// RGBA

/// make color RGBA, A = 0xFF
constexpr color make_color(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return (0xFF000000U | red | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint32_t>(blue) << 16);
}

/// make color RGBA
constexpr color make_color(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) noexcept
{
    return (red | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint32_t>(blue) << 16) | (static_cast<uint32_t>(alpha) << 24);
}

/// wui *.json use BGRA format
/// make color BGRA, A = 0xFF
constexpr color make_color_bgra(const uint8_t red, const uint8_t green, const uint8_t blue) noexcept
{
    return (0xFF000000U | blue | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint32_t>(red) << 16);
}

/// make color BGRA
constexpr color make_color_bgra(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) noexcept
{
    return (blue | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(alpha) << 24);
}

constexpr uint32_t conv_bgra_to_rgba(const color bgra) noexcept
{
    return ((bgra & 0x000000FFU) << 16) | (bgra & 0x0000FF00U) | ((bgra & 0x00FF0000U) >> 16) | (bgra & 0xFF000000U);
}

constexpr uint32_t conv_rgba_to_bgra(const color rgba) noexcept
{
    return ((rgba & 0x000000FFU) << 16) | (rgba & 0x0000FF00U) | ((rgba & 0x00FF0000U) >> 16) | (rgba & 0xFF000000U);
}

/// BGRA to RGB, A = 0
constexpr uint32_t conv_bgra_to_rgb(const color bgra) noexcept
{
    return ((bgra & 0x000000FFU) << 16) | (bgra & 0x0000FF00U) | ((bgra & 0x00FF0000U) >> 16);
}

/// RGBA to BGR, A = 0
constexpr uint32_t conv_rgba_to_bgr(const color rgba) noexcept
{
    return ((rgba & 0x000000FFU) << 16) | (rgba & 0x0000FF00U) | ((rgba & 0x00FF0000U) >> 16);
}


/// RGBA to RGB, A = 0
constexpr color get_rgb(const color rgba) noexcept
{
    return rgba & 0x00FFFFFFU;
}

/// RGBA to RGB, A = 0xFF
constexpr color get_rgb_opaqui(const color rgba) noexcept
{
    return 0xFF000000U | (rgba & 0x00FFFFFFU);
}

/// RGBA to A
constexpr uint8_t get_alpha(const color rgba) noexcept
{
    return (rgba >> 24) & 0x000000FFU;
}

/// RGBA is use alpha
constexpr uint8_t is_alpha(const color rgba) noexcept
{
    return 255 != get_alpha(rgba);
}

/// RGBA to R
constexpr uint8_t get_red(const color rgba) noexcept
{
    return rgba & 0x000000FFU;
}

/// RGBA to G
constexpr uint8_t get_green(const color rgba) noexcept
{
    return (rgba >> 8) & 0x000000FFU;
}

/// RGBA to B
constexpr uint8_t get_blue(const color rgba) noexcept
{
    return (rgba >> 16) & 0x000000FFU;
}

/// wui *.json use BGRA format

/// BGRA to A
constexpr uint8_t get_alpha_bgra(const uint32_t bgra) noexcept
{
    return (bgra >> 24) & 0x000000FFU;
}

/// BGRA to R
constexpr uint8_t get_red_bgra(const uint32_t bgra) noexcept
{
    return (bgra >> 16) & 0x000000FFU;
}

/// BGRA to G
constexpr uint8_t get_green_bgra(const uint32_t bgra) noexcept
{
    return (bgra >> 8) & 0x000000FFU;
}

/// BGRA to B
constexpr uint8_t get_blue_bgra(const uint32_t bgra) noexcept
{
    return bgra & 0x000000FFU;
}

/// RGB[alpha]
constexpr color set_alpha(const color rgba, const uint8_t alpha) noexcept
{
    return (rgba & 0x00FFFFFFU) | (static_cast<uint32_t>(alpha) << 24);
}

/// RGB[A * c]
/// c: [0, 1]
constexpr inline color change_alpha(const color rgba, const float c = 0.8f) noexcept
{
    return (rgba & 0x00FFFFFFU) | ((static_cast<uint32_t>(get_alpha(rgba) * c) << 24) & 0xFF000000U);
}

}
