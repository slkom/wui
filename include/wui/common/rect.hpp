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
    inline bool operator==(const rect &lr) const noexcept
    {
        return lr.left == left && lr.top == top && lr.right == right && lr.bottom == bottom;
    }

    /// <summary>
    /// operator `>`
    /// </summary>
    /// <param name="lr"></param>
    /// <returns>true, the sides of this rectangle are larger</returns>
    inline bool operator>(const rect &lr) const noexcept
    {
        return width() > lr.width() && height() > lr.height();
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
    inline bool in(const int32_t x, const int32_t y) const noexcept
    {
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    /// <summary>
    /// in, check area
    /// </summary>
    /// <param name="outer"></param>
    /// <returns>true, is rectangle in to rectangle</returns>
    inline bool in(const rect &outer) const noexcept
    {
        return !((outer.right <= left || right <= outer.left || outer.bottom <= top || bottom <= outer.top));
    }

    /// <summary>
    /// is_null()
    /// </summary>
    /// <returns>true, is zero rectangle </returns>
    inline bool is_null() const noexcept
    {
        return 0 == left && 0 == top && 0 == right && 0 == bottom;
    }

    /// <summary>
    /// is_hide, width <= 0 || height <= 0
    /// </summary>
    /// <returns> true is hide or bad area </returns>
    inline bool is_hide() const noexcept
    {
        return left >= right || top >= bottom;
    }

    /// <summary>
    /// empty, zero width & height
    /// </summary>
    /// <returns> bool </returns>
    inline bool empty() const noexcept
    {
        return right == left || bottom == top;
    }

    /// <summary>
    /// width
    /// </summary>
    /// <returns> int32_t </returns>
    inline int32_t width() const noexcept
    {
        return right - left;
    }

    /// <summary>
    /// height
    /// </summary>
    /// <returns> int32_t </returns>
    inline int32_t height() const noexcept
    {
        return bottom - top;
    }

    /// <summary>
    /// move
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
    /// min : intersect two rectangle, static
    /// </summary>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    /// <returns></returns>
    static inline rect min(const rect& r1, const rect& r2) noexcept
    {
        rect r;
        min(r, r1, r2);
        return r;
    }

    /// <summary>
    /// min : intersect two rectangle, static
    /// </summary>
    /// <param name="r_out"></param>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    static inline void min(rect& r_out, const rect& r1, const rect& r2) noexcept
    {
        r_out.left = std::max(r1.left, r2.left);
        r_out.right = std::min(r1.right, r2.right);
        r_out.top = std::max(r1.top, r2.top);
        r_out.bottom = std::min(r1.bottom, r2.bottom);
    }

    /// <summary>
    /// max : union two rectangle
    /// </summary>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    /// <returns></returns>
    static inline rect max(const rect &r1, const rect& r2) noexcept
    {
        rect r;
        max(r, r1, r2);
        return r;
    }

    /// <summary>
    /// max : union two rectangle
    /// </summary>
    /// <param name="r_out"></param>
    /// <param name="r1"></param>
    /// <param name="r2"></param>
    static inline void max(rect &r_out, const rect &r1, const rect& r2) noexcept
    {
        r_out.left = std::min(r1.left, r2.left);
        r_out.right = std::max(r1.right, r2.right);
        r_out.top = std::min(r1.top, r2.top);
        r_out.bottom = std::max(r1.bottom, r2.bottom);
    }
};

}
