//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <cstdint>
#include <algorithm>

#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif

namespace wui
{

struct rect
{
    int32_t left, top, right, bottom;

    /// <summary>
    /// operator `==`
    /// </summary>
    /// <param name="lr"></param>
    /// <returns>true, these rectangles are equal</returns>
    [[nodiscard]] inline bool operator==(const rect lr) const noexcept
    {
        return lr.left == left && lr.top == top && lr.right == right && lr.bottom == bottom;
    }

    [[nodiscard]] inline bool operator!=(const rect r) const noexcept
    {
        return !(*this == r);
    }

    /// <summary>
    /// operator '>'
    /// </summary>
    /// <param name="lr"></param>
    /// <returns>true, if the sides of this rectangle are larger</returns>
    [[nodiscard]] inline bool operator>(const rect lr) const noexcept
    {
        return width() > lr.width() && height() > lr.height();
    }

    /// <summary>
    /// operator '>='
    /// </summary>
    /// <param name="lr"></param>
    /// <returns>true, if the sides of this rectangle are greater than or equal</returns>
    [[nodiscard]] inline bool operator>=(const rect lr) const noexcept
    {
        return width() >= lr.width() && height() >= lr.height();
    }

    /// <summary>
    /// clear the rectangle
    /// </summary>
    inline void clear() noexcept
    {
        *this = rect{};
    }

    /// <summary>
    /// in, check area
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <returns>true, this rectangle is inside a rectangle</returns>
    [[nodiscard]] inline bool in(const int32_t x, const int32_t y) const noexcept
    {
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    /// <summary>
    /// in, check area
    /// </summary>
    /// <param name="outer"></param>
    /// <returns>true, is rectangle in to rectangle (is intersect)</returns>
    [[nodiscard]] inline bool in(const rect outer) const noexcept
    {
        return !((outer.right <= left || right <= outer.left || outer.bottom <= top || bottom <= outer.top));
    }

    /// <summary>
    /// is_null()
    /// </summary>
    /// <returns>true, is zero rectangle </returns>
    [[nodiscard]] inline bool is_null() const noexcept
    {
        return 0 == left && 0 == top && 0 == right && 0 == bottom;
    }

    /// <summary>
    /// is_hide, width <= 0 || height <= 0
    /// </summary>
    /// <returns> true is hide or bad area </returns>
    [[nodiscard]] inline bool is_hide() const noexcept
    {
        return left >= right || top >= bottom;
    }

    /// <summary>
    /// empty, zero width & height
    /// </summary>
    /// <returns> bool </returns>
    [[nodiscard]] inline bool empty() const noexcept
    {
        return right == left || bottom == top;
    }

    /// <summary>
    /// width
    /// </summary>
    /// <returns> int32_t </returns>
    [[nodiscard]] inline int32_t width() const noexcept
    {
        return right - left;
    }

    /// <summary>
    /// height
    /// </summary>
    /// <returns> int32_t </returns>
    [[nodiscard]] inline int32_t height() const noexcept
    {
        return bottom - top;
    }

    /// <summary>
    /// move position
    /// </summary>
    /// <param name="val"></param>
    inline void move(const int32_t val) noexcept
    {
        left += val;
        top += val;
        right += val;
        bottom += val;
    }

    /// <summary>
    /// move position
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    inline void move(const int32_t x, const int32_t y) noexcept
    {
        left += x;
        top += y;
        right += x;
        bottom += y;
    }

    /// <summary>
    /// put : set left, top
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    inline void put(const int32_t left_, const int32_t top_) noexcept
    {
        right = left_ + width();
        bottom = top_ + height();
        left = left_;
        top = top_;
    }

    /// <summary>
    /// normalize rect, eq: { rect t = *this; t.put(0, 0); return t; }
    /// </summary>
    /// <returns>rect</returns>
    [[nodiscard]] inline rect normalize() const noexcept
    {
        return { 0, 0, width(), height() };
    }

    /// <summary>
    /// resize rectangle
    /// </summary>
    /// <param name="val"></param>
    inline void resize(const int32_t val) noexcept
    {
        right = left + val;
        bottom = top + val;
    }

    /// <summary>
    /// resize rectangle
    /// </summary>
    /// <param name="width_"></param>
    /// <param name="height_"></param>
    inline void resize(const int32_t width_, const int32_t height_) noexcept
    {
        right = left + width_;
        bottom = top + height_;
    }

    /// <summary>
    /// resize width rectangle
    /// </summary>
    /// <param name="width_"></param>
    inline void resize_width(const int32_t width_) noexcept
    {
        right = left + width_;
    }

    /// <summary>
    /// resize height rectangle
    /// </summary>
    /// <param name="height_"></param>
    inline void resize_height(const int32_t height_) noexcept
    {
        bottom = top + height_;
    }

    /// <summary>
    /// widen rectangle on both sides
    /// </summary>
    /// <param name="val"></param>
    inline void widen(const int32_t val) noexcept
    {
        left -= val;
        top -= val;
        right += val;
        bottom += val;
    }

    /// <summary>
    /// widen rectangle on both sides
    /// </summary>
    /// <param name="dx"></param>
    /// <param name="dy"></param>
    inline void widen(const int32_t dx, const int32_t dy) noexcept
    {
        left -= dx;
        top -= dy;
        right += dx;
        bottom += dy;
    }

    /// <summary>
    /// intersect (min) two rectangle, static
    /// </summary>
    /// <param name="r"></param>
    /// <returns>rect</returns>
    [[nodiscard]] inline rect intersect(const rect r) const noexcept
    {
        rect t;
        intersect(t, *this, r);
        return t;
    }

    /// <summary>
    /// intersect (min) two rectangle, static
    /// </summary>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    /// <returns>rect</returns>
    [[nodiscard]] static inline rect intersect(const rect r1, const rect r2) noexcept
    {
        rect r;
        intersect(r, r1, r2);
        return r;
    }

    /// <summary>
    /// intersect (min) two rectangle, static
    /// </summary>
    /// <param name="r_out"></param>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    static inline void intersect(rect& r_out, const rect r1, const rect r2) noexcept
    {
        r_out.left = std::max(r1.left, r2.left);
        r_out.right = std::min(r1.right, r2.right);
        r_out.top = std::max(r1.top, r2.top);
        r_out.bottom = std::min(r1.bottom, r2.bottom);
    }

    /// <summary>
    /// union (max) two rectangle
    /// </summary>
    /// <param name="r1"></param>
    /// <returns>rect</returns>
    [[nodiscard]] inline rect union_(const rect r) const noexcept
    {
        rect t;
        union_(t, *this, r);
        return t;
    }

    /// <summary>
    /// union (max) two rectangle
    /// </summary>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    /// <returns>rect</returns>
    [[nodiscard]] static inline rect union_(const rect r1, const rect r2) noexcept
    {
        rect r;
        union_(r, r1, r2);
        return r;
    }

    /// <summary>
    /// max : union two rectangle
    /// </summary>
    /// <param name="r_out"></param>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    static inline void union_(rect& r_out, const rect r1, const rect r2) noexcept
    {
        r_out.left = std::min(r1.left, r2.left);
        r_out.right = std::max(r1.right, r2.right);
        r_out.top = std::min(r1.top, r2.top);
        r_out.bottom = std::max(r1.bottom, r2.bottom);
    }
};

}
