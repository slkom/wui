//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/graphic/graphic.hpp>
#include <wui/system/tools.hpp>
#include <wui/common/error.hpp>
#include <wui/window/window.hpp>

#include <boost/nowide/convert.hpp>

#include <unordered_map>

#ifdef __linux__
#include <xcb/xcb_image.h>
//extern "C" {
//#include <xcb/present.h>
//}
#include <cairo.h>
#include <cairo-xcb.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <cstring>
#elif _WIN32
#include <gdiplus.h>
#endif

#ifndef NDEBUG
#include <assert.h>
#endif

namespace wui
{

#ifdef __linux__
static xcb_visualtype_t* default_visual_type(system_context& context_)
{
    auto depth_iter = xcb_screen_allowed_depths_iterator(context_.screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter))
    {
        auto visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter))
        {
            if (context_.screen->root_visual == visual_iter.data->visual_id)
            {
                return visual_iter.data;
            }
        }
    }

    return nullptr;
}
#endif

graphic::graphic(system_context& context__) : context_(context__), pc(context_)
{}

graphic::~graphic()
{
    release();
}

#if !defined(_WIN32)

/// over-allocation
bool graphic::resize(const int32_t width, const int32_t height, const bool clear_)
{
    // TODO: выделять можно порциями, однако, в данном случае операции не затратные?
    if (inited() && width == width_ && height == height_)
    {
        if (clear_)
        {
            clear({ 0, 0, width, height });
        }
        return true; // skip
    }

    if (width <= 0 || height <= 0)
    {
        return inited(); // skip
    }

    if (!target_surface || target_surface == screen_surface)
    {
        if (!screen_surface)
        {
            return init(width, height, background_color, clear_);
        }
        cairo_surface_flush(screen_surface);
        cairo_xcb_surface_set_size(screen_surface, width, height);
    }
    else
    {
#ifndef NDEBUG
        assert(!screen_surface_use);
#endif
    }

    pc.release();
    if (surface)
    {
        cairo_surface_destroy(surface);
    }

    surface = cairo_surface_create_similar(target_surface, CAIRO_CONTENT_COLOR, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
    {
        surface = nullptr;
        err.set(error_type::no_handle, "graphic::init()", "Can't create the cairo similar surface");
        return false;
    }
    width_ = width;
    height_ = height;
    if (clear_)
    {
        clear({ 0, 0, width, height });
    }
    return true;
}
#endif


bool graphic::init(const int32_t width, const int32_t height, const color background_color_, const bool clear_)
{
    if (width <= 0 || height <= 0)
    {
      err.set(error_type::invalid_value, "graphic::init()", "Graphics buffer dimensions must be positive");
      return false;
    }

#ifdef _WIN32
    if (inited())
    {
        err.set(error_type::already_started, "graphic::init()", "the object 'graphic' are already initialized");
        return false;
    }

    auto wnd_dc = GetDC(context_.hwnd);

    mem_dc = CreateCompatibleDC(wnd_dc);
    if (!mem_dc)
    {
        ReleaseDC(context_.hwnd, wnd_dc);
        err.set(error_type::no_handle, "graphic::init()", "CreateCompatibleDC return null");
        return false;
    }

    mem_bitmap = CreateCompatibleBitmap(wnd_dc, width, height);
    if (!mem_bitmap)
    {
        DeleteDC(mem_dc);
        mem_dc = NULL;
        ReleaseDC(context_.hwnd, wnd_dc);
        err.set(error_type::no_handle, "graphic::init()", "CreateCompatibleBitmap return null");
        return false;
    }

    mem_bitmap_old = SelectObject(mem_dc, mem_bitmap);

    SetMapMode(mem_dc, MM_TEXT);

    ReleaseDC(context_.hwnd, wnd_dc);

#elif __linux__

    if (!context_.connection)
    {
        err.set(error_type::no_handle, "graphic::init()");
        return false;
    }

    pc.release();

    if (surface)
    {
        cairo_surface_destroy(surface);
        surface = nullptr;
    }

    if (!target_surface || target_surface == screen_surface)
    {
        if (!screen_surface)
        {
            // cairo_xcb_surface_create(., wnd, ...) :
            // such argument wnd does not allocate memory for the surface.
            screen_surface = cairo_xcb_surface_create(context_.connection, context_.wnd,
                default_visual_type(context_), width, height);
            if (cairo_surface_status(screen_surface) != CAIRO_STATUS_SUCCESS)
            {
                screen_surface = nullptr;
                target_surface = nullptr;
                err.set(error_type::no_handle, "graphic::init()", "Can't create the cairo screen surface");
                return false;
            }
            screen_surface_use = true;
        }
        else
        {
            cairo_surface_flush(screen_surface); // ?
            // This call only updates the internal cairo boundaries in the metadata.
            cairo_xcb_surface_set_size(screen_surface, width, height);
        }

        target_surface = screen_surface;
    }
    else
    {
#ifndef NDEBUG
        assert(!screen_surface_use);
#endif
        if (screen_surface)
        {
            cairo_surface_destroy(screen_surface);
            screen_surface = nullptr;
        }
    }

    surface = cairo_surface_create_similar(target_surface, CAIRO_CONTENT_COLOR, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
    {
        surface = nullptr;
        err.set(error_type::no_handle, "graphic::init()", "Can't create the cairo similar surface");
        return false;
    }
#endif

    width_ = width;
    height_ = height;
    background_color = background_color_;

    pc.init();
    if (clear_)
    {
        clear({ 0, 0, width, height });
    }
    return true;
}

void graphic::release()
{
    pc.release();

    if (!inited())
        return;
    width_ = height_ = 0;
#ifdef _WIN32
    SelectObject(mem_dc, mem_bitmap_old);
    mem_bitmap_old = NULL;
    DeleteObject(mem_bitmap);
    mem_bitmap = NULL;
    DeleteDC(mem_dc);
    mem_dc = NULL;
#elif __linux__

    cairo_surface_destroy(surface);
    surface = nullptr;

    if (dirty_region)
    {
        cairo_region_destroy(dirty_region);
        dirty_region = nullptr;
    }

    screen_surface_use = false;
    target_surface = nullptr;
    if (screen_surface)
    {
        cairo_surface_destroy(screen_surface);
        screen_surface = nullptr;
    }
#endif
}

void graphic::set_background_color(color background_color_, const bool clear_)
{
    background_color = background_color_;

    if (clear_)
    {
        clear({ 0, 0, width_, height_ });
    }
}

void graphic::clear(const rect position)
{
    if (!inited())
    {
        return;
    }

#ifdef _WIN32
    RECT filling_rect = !position.is_hide() ?
        RECT{ position.left, position.top, position.right, position.bottom } :
        RECT{ 0, 0, width_, height_ };
    FillRect(mem_dc, &filling_rect, pc.get_brush(background_color));
#elif __linux__
    auto cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgb(cr,
        get_red(background_color) / 255.0,
        get_green(background_color) / 255.0,
        get_blue(background_color) / 255.0);

    if (position.is_hide() || (width_ == position.width() && height_ == position.height()))
    {
        //cairo_rectangle(cr, 0, 0, width_, height_);
        cairo_paint(cr);
    }
    else
    {
        cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
        cairo_fill(cr);
    }
    cairo_destroy(cr);
#endif
}

#if __linux__
void graphic::dirty_region_union(const rect rc)
{
    if (rc.is_hide())
    {
        return;
    }
    cairo_rectangle_int_t r{ rc.left, rc.top, rc.width(), rc.height() };
    if (nullptr == dirty_region)
    {
        dirty_region = cairo_region_create_rectangle(&r);
        return;
    }
    cairo_region_union_rectangle(dirty_region, &r);
}

rect graphic::get_dirty_region_extents()
{
    bool reg_empty = false;
    if (!dirty_region
        || CAIRO_STATUS_SUCCESS != cairo_region_status(dirty_region)
        || (reg_empty = cairo_region_is_empty(dirty_region)))
    {
        return reg_empty ? rect{} : rect{ 0, 0, width_, height_ }; // null or empty (safe empty dirty_region, not destroy)
    }
    cairo_rectangle_int_t rc;
    cairo_region_get_extents(dirty_region, &rc);
    return { rc.x, rc.y, rc.x + rc.width, rc.y + rc.height};
}
#endif

void graphic::flush(const rect updated_size)
{
    if (!inited())
    {
        return;
    }

#ifdef _WIN32
    auto wnd_dc = GetDC(context_.hwnd);

    if (wnd_dc)
    {
        BitBlt(wnd_dc,
            updated_size.left,
            updated_size.top,
            updated_size.width(),
            updated_size.height(),
            mem_dc,
            updated_size.left,
            updated_size.top,
            SRCCOPY);
        ReleaseDC(context_.hwnd, wnd_dc);
    }
#elif __linux__
#ifndef NDEBUG
    assert(!updated_size.is_hide());
#endif
    cairo_t* cr_target = cairo_create(target_surface);
    cairo_set_source_surface(cr_target, surface, 0, 0);
    cairo_set_operator(cr_target, CAIRO_OPERATOR_SOURCE);

    if (width_ != updated_size.width() || height_ != updated_size.height())
    {
        cairo_rectangle(cr_target, updated_size.left, updated_size.top, updated_size.width(),
            updated_size.height());
        cairo_fill(cr_target);
    }
    else
    {
        cairo_paint(cr_target);
    }

    cairo_destroy(cr_target);

    if (dirty_region)
    {
        cairo_region_destroy(dirty_region);
        dirty_region = nullptr;
    }

    cairo_surface_flush(target_surface);
    xcb_flush(context_.connection);
#endif // __linux__
}

#ifndef _WIN32
void graphic::surface_flush()
{
    /*
        This function cairo_surface_flush() must be called before switching from drawing on the surface with
        cairo to drawing on it directly with native APIs, or accessing its memory outside
        of Cairo.
    */
    cairo_surface_flush(surface);
}
#endif


void graphic::draw_pixel(const int32_t x, const int32_t y, const color color_)
{
#ifdef _WIN32
    // color alpha not supported

    SetPixel(mem_dc, x, y, get_rgb0(color_));

#elif __linux__
    // slow
    auto cr = cairo_create(surface);
    cairo_set_source_rgb(cr,
        get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0);
    cairo_rectangle(cr, x, y, 1, 1);
    cairo_fill(cr);
    cairo_destroy(cr);
#endif
}

void graphic::draw_line(const rect position, const color color_, const int32_t width,
    const bool antialias_none)
{
#ifdef _WIN32

#ifndef _ONLY_GDIPLUS
    auto old_pen = (HPEN)SelectObject(mem_dc, pc.get_pen(PS_SOLID, width, color_));

    MoveToEx(mem_dc, position.left, position.top, (LPPOINT)NULL);
    LineTo(mem_dc, position.right, position.bottom);

    SelectObject(mem_dc, old_pen);
#else
    Gdiplus::Pen pen(
        Gdiplus::Color(
            get_alpha(color_),
            get_red(color_),
            get_green(color_),
            get_blue(color_)),
        static_cast<Gdiplus::REAL>(width));

    Gdiplus::Graphics g(mem_dc);
    if(!antialias_none)
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    g.TranslateTransform(-0.5f, -0.5f);
    g.DrawLine(&pen, position.left, position.top, position.right, position.bottom);
#endif
#elif __linux__
    auto cr = cairo_create(surface);
    cairo_set_source_rgb(cr,
        get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0);

    if (antialias_none)
    {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    }
    if (width & 1)
        cairo_translate(cr, 0.5, 0.5);
    cairo_set_line_width(cr, width);
    cairo_move_to(cr, position.left, position.top);
    cairo_line_to(cr, position.right, position.bottom);
    cairo_stroke(cr);
    cairo_destroy(cr);
#endif
}

void graphic::draw_lines(const rect *rcs, uint32_t size, const color color_, const int32_t width,
    const bool antialias_none)
{
#ifdef _WIN32
    Gdiplus::Pen pen(
        Gdiplus::Color(
        get_alpha(color_),
        get_red(color_),
        get_green(color_),
        get_blue(color_)),
        static_cast<Gdiplus::REAL>(width));

    Gdiplus::Graphics g(mem_dc);
    if (!antialias_none)
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    for (uint32_t i = 0; i < size; ++i)
    {
        g.DrawLine(&pen,
            static_cast<Gdiplus::REAL>(rcs[i].left),
            static_cast<Gdiplus::REAL>(rcs[i].top),
            static_cast<Gdiplus::REAL>(rcs[i].right),
            static_cast<Gdiplus::REAL>(rcs[i].bottom));
    }
#elif __linux__
    auto cr = cairo_create(surface);
    cairo_set_source_rgb(cr,
        get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0);

    if (antialias_none)
    {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    }

    if (width & 1) cairo_translate(cr, 0.5, 0.5);
    cairo_set_line_width(cr, width);

    for (uint32_t i = 0; i < size; ++i)
    {
        cairo_move_to(cr, rcs[i].left, rcs[i].top);
        cairo_line_to(cr, rcs[i].right, rcs[i].bottom);
    }

    cairo_stroke(cr);
    cairo_destroy(cr);
#endif
}

// returns recommended distance between the baselines of two adjacent lines of text
int32_t graphic::font_line_height(const font &font_, int32_t* max_height)
{
    if (!inited())
    {
        if (max_height)
            *max_height = font_.size;
        return font_.size;
    }

#ifdef _WIN32
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    TEXTMETRIC tm;
    if (!GetTextMetrics(mem_dc, &tm))
    {
        SelectObject(mem_dc, old_font);
        if (max_height)
            *max_height = font_.size;
        return font_.size;
    }

    SelectObject(mem_dc, old_font);
    if (max_height)
        *max_height = tm.tmHeight;
    return tm.tmHeight + tm.tmExternalLeading;
#elif __linux__
    auto cr = pc.get_font(font_, surface);
    if (!cr)
    {
        if (max_height)
            *max_height = font_.size;
        err.set(error_type::no_handle, "graphic::font_line_height()", "No cairo font context");
        return font_.size;
    }
    cairo_font_extents_t fe;
    cairo_font_extents(cr, &fe);
    if (max_height)
        *max_height = static_cast<int32_t>(std::ceil(fe.ascent + fe.descent));
    return static_cast<int32_t>(std::ceil(fe.height));
#endif
}

int32_t graphic::font_max_height(const font &font_)
{
    if (!inited())
    {
        return font_.size;
    }

#ifdef _WIN32
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    TEXTMETRIC tm;
    if (!GetTextMetrics(mem_dc, &tm))
    {
        SelectObject(mem_dc, old_font);
        return font_.size;
    }

    SelectObject(mem_dc, old_font);
    return tm.tmHeight;
#elif __linux__
    auto cr = pc.get_font(font_, surface);
    if (!cr)
    {
        err.set(error_type::no_handle, "graphic::font_max_height()", "No cairo font context");
        return font_.size;
    }
    cairo_font_extents_t fe;
    cairo_font_extents(cr, &fe);
    return static_cast<int32_t>(std::ceil(fe.ascent + fe.descent));;
#endif
}

#ifdef _WIN32
int32_t graphic::font_line_height_gdiplus(const font &font_)
{
    if (!inited())
    {
        return font_.size;
    }
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    Gdiplus::Graphics g(mem_dc);
    Gdiplus::Font font(mem_dc);
    const Gdiplus::REAL lineSpacing = font.GetHeight(&g); // font.GetHeight(dpi)
    SelectObject(mem_dc, old_font);
    return static_cast<int32_t>(std::ceil(lineSpacing));
}
#endif

rect graphic::measure_text(std::string_view text_, const font &font_)
{
    if (text_.empty() || !inited())
    {
        return {0, 0, 0, font_.size};
    }

#ifndef NDEBUG
    assert(font_.unamed()); // check bad call (font not configured)
#endif

#ifdef _WIN32
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));

    SIZE sz{ };
    auto wide_str = std::move(boost::nowide::widen(text_));
    GetTextExtentPoint32W(mem_dc, wide_str.c_str(), static_cast<int>(wide_str.size()), &sz);

    SelectObject(mem_dc, old_font);
    return {0, 0, sz.cx, sz.cy };
#elif __linux__

    auto cr = pc.get_font(font_, surface);
    if (!cr)
    {
        err.set(error_type::no_handle, "graphic::measure_text()", "No cairo font context");
        return { 0, 0, 0, font_.size };
    }

    std::string s;
    s.reserve(text_.size() + 2);
    s = text_;

    cairo_text_extents_t te;
    cairo_text_extents(cr, s.c_str(), &te);
    if(CAIRO_STATUS_SUCCESS != cairo_status(cr))
    {
        return { 0, 0, 0, font_.size };
    }

    cairo_font_extents_t fe;
    cairo_font_extents(cr, &fe);

    return { 0, 0,
        static_cast<int32_t>(std::ceil(te.x_advance)),
        static_cast<int32_t>(std::ceil(fe.height)) };
#endif
}

#ifdef _WIN32
rect graphic::measure_text_gdiplus(std::string_view text_, const font &font_)
{
    if (text_.empty() || !inited())
    {
        return {0, 0, 0, font_.size};
    }
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));

    Gdiplus::Font font(mem_dc);

    Gdiplus::RectF layoutRect{};
    auto wide_str = std::move(boost::nowide::widen(text_));
    Gdiplus::RectF boundingBox;
    Gdiplus::Graphics g(mem_dc);

    Gdiplus::StringFormat stringFormat;
    stringFormat.SetFormatFlags(
        Gdiplus::StringFormatFlagsNoFitBlackBox |
        Gdiplus::StringFormatFlagsNoClip);

    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    const Gdiplus::Status status = g.MeasureString(
        wide_str.c_str(),
        static_cast<INT>(wide_str.size()),
        &font,
        layoutRect,
        &stringFormat,
        &boundingBox
    );

    SelectObject(mem_dc, old_font);
    if (Gdiplus::Ok == status)
    {
        return { 0, 0, static_cast<int32_t>(std::ceil(boundingBox.GetRight())),
            static_cast<int32_t>(std::ceil(boundingBox.GetBottom())) };
    }
    return { 0, 0, 0, font_.size };
}
#endif

void graphic::draw_text(const rect position, std::string_view text_, const color color_,
    const font& font_)
{
#ifdef _WIN32

#   if 1
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));

    SetTextColor(mem_dc, get_rgb0(color_));
    const auto mode_old = SetBkMode(mem_dc, TRANSPARENT);

    auto wide_str = std::move(boost::nowide::widen(text_));

    if (position.is_hide())
    {
        TextOutW(mem_dc, position.left, position.top, wide_str.c_str(), static_cast<int>(wide_str.size()));
    }
    else
    {
        const RECT rc = { position.left, position.top, position.right, position.bottom };
        ExtTextOutW(mem_dc,
            position.left, position.top,
            ETO_CLIPPED,
            &rc, wide_str.c_str(),
            static_cast<UINT>(wide_str.size()), NULL);
    }

    SelectObject(mem_dc, old_font);
    SetBkMode(mem_dc, mode_old);

#   else

    //TODO: для DPI - рисование текста исключительно GDI++
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    Gdiplus::Graphics g(mem_dc);
    if (!position.is_hide())
    {
        Gdiplus::Region region(Gdiplus::Rect(position.left, position.top, position.width(), position.height()));
        g.SetClip(&region, Gdiplus::CombineModeReplace);
    }
    const Gdiplus::Font font(mem_dc);
    if (is_alpha(color_))
        g.SetCompositingQuality(Gdiplus::CompositingQualityGammaCorrected);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    const Gdiplus::SolidBrush br(
        Gdiplus::Color(
        get_alpha(color_),
        get_red(color_),
        get_green(color_),
        get_blue(color_)));

    Gdiplus::StringFormat stringFormat;
    stringFormat.SetFormatFlags(
        Gdiplus::StringFormatFlagsNoFitBlackBox |
        Gdiplus::StringFormatFlagsNoClip
    );

    auto wide_str = std::move(boost::nowide::widen(line.str));
    const Gdiplus::PointF pt(static_cast<Gdiplus::REAL>(position.left),
        static_cast<Gdiplus::REAL>(position.top));
    g.DrawString(wide_str.c_str(),
        static_cast<INT>(wide_str.size()),
        &font,
        pt,
        &stringFormat,
        &br);
    SelectObject(mem_dc, old_font);
#   endif

#elif __linux__

    auto cr = pc.get_font(font_, surface);
    if (!cr)
    {
        err.set(error_type::no_handle, "graphic::draw_text()", "No cairo font context");
        return;
    }

    const bool clip = !position.is_hide();
    if (clip)
    {
        cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr,
        get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0
    );

    const auto top = position.top + font_.size;
    cairo_move_to(cr, position.left, top);

    std::string text__(text_); /// Workaround to prevent crashes
    text__ += '\0';

    cairo_show_text(cr, text__.c_str());

    if (font_.decorations_ & (decorations::underline | decorations::strike_out))
    {
        double y_offset{};
        cairo_font_extents_t fe;
        cairo_font_extents(cr, &fe);
        cairo_set_line_width(cr, fe.height > 15.0 ? fe.height / 15.0 : 1.0);

        if (font_.decorations_ & decorations::underline)
        {
            y_offset = fe.descent / 2.0;
        }
        else
        {
            cairo_text_extents_t tex;
            cairo_text_extents(cr, "x\0", &tex);
            y_offset = tex.y_bearing / 2.0;
        }

        cairo_text_extents_t te;
        cairo_text_extents(cr, text__.c_str(), &te);

        cairo_move_to(cr, position.left + te.x_bearing, top + y_offset);
        cairo_line_to(cr, position.left + te.x_bearing + te.x_advance, top + y_offset);
        cairo_stroke(cr);
    }
    if (clip)
    {
        cairo_reset_clip(cr);
    }

#endif
}

#ifdef _WIN32

void graphic::draw_text_clip_rgb(const rect position, const text_lines_t& lines,
    const color color_, const font &font_, const bool clip_)
{
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    SetTextColor(mem_dc, get_rgb0(color_));
    const auto mode_old = SetBkMode(mem_dc, TRANSPARENT);

    const RECT rc = { position.left, position.top, position.right, position.bottom };
    const RECT* ptr_rc = clip_ && !position.is_hide() ? &rc : nullptr;
    for (auto& line : lines)
    {
        auto wide_str = std::move(boost::nowide::widen(line.str));
        ExtTextOutW(mem_dc,
            position.left + line.rc.left, position.top + line.rc.top,
            ETO_CLIPPED,
            ptr_rc, wide_str.c_str(),
            static_cast<UINT>(wide_str.size()), NULL);
    }
    SelectObject(mem_dc, old_font);
    SetBkMode(mem_dc, mode_old);
}

// supported alpha
void graphic::draw_text_clip(const rect position, const text_lines_t& lines,
    const color color_, const font &font_, const bool clip_)
{
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font_));
    Gdiplus::Graphics g(mem_dc);
    if (clip_)
    {
        Gdiplus::Region region(Gdiplus::Rect(position.left, position.top, position.width(), position.height()));
        g.SetClip(&region, Gdiplus::CombineModeReplace);
    }

    const Gdiplus::Font font(mem_dc);
    if (is_alpha(color_))
        g.SetCompositingQuality(Gdiplus::CompositingQualityGammaCorrected);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    const Gdiplus::SolidBrush br(
        Gdiplus::Color(
            get_alpha(color_),
            get_red(color_),
            get_green(color_),
            get_blue(color_)));

    Gdiplus::StringFormat stringFormat;
    stringFormat.SetFormatFlags(
        Gdiplus::StringFormatFlagsNoFitBlackBox |
        Gdiplus::StringFormatFlagsNoClip);

    for (auto& line : lines)
    {
        auto wide_str = std::move(boost::nowide::widen(line.str));
        const Gdiplus::PointF pt(static_cast<Gdiplus::REAL>(position.left + line.rc.left),
            static_cast<Gdiplus::REAL>(position.top + line.rc.top));
        g.DrawString(wide_str.c_str(),
            static_cast<INT>(wide_str.size()),
            &font,
            pt,
            &stringFormat,
            &br);
    }

    SelectObject(mem_dc, old_font);
}

#elif __linux__

// supported alpha
void graphic::draw_text_clip(const rect position, const text_lines_t & lines,
    const color color_, const font &font_, const bool clip_)
{
    auto cr = pc.get_font(font_, surface);
    if (!cr) {
        err.set(error_type::no_handle, "graphic::draw_text()", "No cairo font context");
        return;
    }

    cairo_set_source_rgba(cr,
        get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0,
        get_alpha(color_) / 255.0);

    if (clip_)
    {
        cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
        cairo_clip(cr);
    }

    double y_offset{};
    if (font_.decorations_ & (decorations::underline | decorations::strike_out))
    {
        cairo_font_extents_t fe;
        cairo_font_extents(cr, &fe);
        cairo_set_line_width(cr, fe.height > 15.0 ? fe.height / 15.0 : 1.0);

        if (font_.decorations_ & decorations::underline)
        {
            y_offset = fe.descent / 2.0;
        }
        else
        {
            cairo_text_extents_t tex;
            cairo_text_extents(cr, "x\0", &tex);
            y_offset = tex.y_bearing / 2.0;
        }
    }
    std::string text;
    for (auto& line : lines)
    {
        const auto left = position.left + line.rc.left;
        const auto top = position.top + line.rc.top + font_.size;// *5.0 / 6.0;
        cairo_move_to(cr, left, top);
        text = line.str; /// Workaround to prevent crashes
        text += '\0';

        cairo_show_text(cr, text.c_str());

        if (font_.decorations_ & (decorations::underline | decorations::strike_out))
        {
            cairo_text_extents_t te;
            cairo_text_extents(cr, text.c_str(), &te);

            cairo_move_to(cr, left + te.x_bearing, top + y_offset);
            cairo_line_to(cr, left + te.x_bearing + te.x_advance, top + y_offset);
            cairo_stroke(cr);
        }
    }

    if (clip_)
    {
        cairo_reset_clip(cr);
    }
}
#endif

void graphic::fill_path(const rect position, const point2i* pts, const int n,
    const color color_, const bool antialias_none)
{
#ifndef NDEBUG
    // assert(n > 0);
#endif
#ifdef _WIN32

    const Gdiplus::SolidBrush br(
        Gdiplus::Color(
            get_alpha(color_),
            get_red(color_),
            get_green(color_),
            get_blue(color_)));

    Gdiplus::Graphics g(mem_dc);

    if (is_alpha(color_))
        g.SetCompositingQuality(Gdiplus::CompositingQualityGammaCorrected);

    if (!antialias_none)
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    if (n > 1) // Make the path
    {
        g.TranslateTransform(position.left - 0.5f,
            position.top - 0.5f);

        Gdiplus::GraphicsPath path;
        for (int32_t i = 0; i < n; ++i)
        {
            const Gdiplus::Point p(pts[i].x, pts[i].y);
            path.AddLines(&p, 1);
        }
        path.CloseFigure();

        g.FillPath(&br, &path);
        return;
    }

    g.FillRectangle(&br, position.left + pts[0].x, position.top + pts[0].y, 1, 1);

#elif __linux__

    auto cr = cairo_create(surface);
    cairo_set_source_rgba(cr, get_red(color_) / 255.0,
        get_green(color_) / 255.0,
        get_blue(color_) / 255.0,
        get_alpha(color_) / 255.0);

    if (antialias_none)
    {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    }

    if (n > 1)
    {
        cairo_new_sub_path(cr);
        cairo_translate(cr, position.left, position.top);
        for (int i = 0; i < n; ++i)
        {
            cairo_line_to(cr, pts[i].x, pts[i].y);
        }
        cairo_close_path(cr);
    }
    else
    {
        cairo_rectangle(cr, position.left + pts[0].x, position.top + pts[0].y, 1, 1);
    }

    cairo_fill(cr);
    cairo_destroy(cr);
#endif
}

void graphic::draw_rect(const rect position, const color fill_color)
{
#ifndef NDEBUG
    assert(position.left <= position.right);
    assert(position.top <= position.bottom);
#endif

#ifdef _WIN32

#ifndef _ONLY_GDIPLUS
    const RECT position_rect = { position.left, position.top, position.right, position.bottom };
    FillRect(mem_dc, &position_rect, pc.get_brush(fill_color));
#else
    const Gdiplus::SolidBrush br(
        Gdiplus::Color(
            get_alpha(fill_color),
            get_red(fill_color),
            get_green(fill_color),
            get_blue(fill_color)));

    Gdiplus::Graphics g(mem_dc);
    if (is_alpha(fill_color))
        g.SetCompositingQuality(Gdiplus::CompositingQualityGammaCorrected);
    Gdiplus::GraphicsPath path;
    Gdiplus::Rect r{ position.left, position.top, position.width(), position.height() };
    path.AddRectangle(r);
    g.FillPath(&br, &path);

#endif

#elif __linux__

# ifndef NDEBUG
    assert(surface);
# endif

    auto cr = cairo_create(surface);

    cairo_set_source_rgba(cr,
        get_red(fill_color) / 255.0,
        get_green(fill_color) / 255.0,
        get_blue(fill_color) / 255.0,
        get_alpha(fill_color) / 255.0
        );
    cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
    cairo_fill(cr);

    cairo_destroy(cr);
#endif
}

#ifdef _WIN32
static void DrawRoundBox(HDC dc, const rect pos_, const int32_t radius_,
    int32_t borderWidth, const color background, const color border)
{
    const rect pos{ pos_.left, pos_.top,
        pos_.right, pos_.bottom };

    Gdiplus::Graphics g(dc);

    if(is_alpha(background) || is_alpha(border))
        g.SetCompositingQuality(Gdiplus::CompositingQualityGammaCorrected);

    Gdiplus::GraphicsPath path;
    // Make the path
    if (radius_ > 0)
    {
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        path.AddArc(pos.left, pos.top, radius_ * 2, radius_ * 2, 180, 90);
        path.AddArc(pos.right - radius_ * 2, pos.top, radius_ * 2, radius_ * 2, 270, 90);
        path.AddArc(pos.right - radius_ * 2, pos.bottom - radius_ * 2, radius_ * 2, radius_ * 2, 0, 90);
        path.AddArc(pos.left, pos.bottom - radius_ * 2, radius_ * 2, radius_ * 2, 90, 90);
        path.CloseFigure();
    }
    else
    {
        Gdiplus::RectF r{
            static_cast<Gdiplus::REAL>(pos.left),  static_cast<Gdiplus::REAL>(pos.top),
            pos.right - pos.left - 0.5f, pos.bottom - pos.top - 0.5f};

        path.AddRectangle(r);
    }

    // Fill (only if the color is set)
    if (0 != get_alpha(background))
    {
        const Gdiplus::SolidBrush br(
            Gdiplus::Color(
                get_alpha(background),
                get_red(background),
                get_green(background),
                get_blue(background)));

        g.FillPath(&br, &path);
    }

    // Border outline
    if (borderWidth > 0 && 0 != get_alpha(border))
    {
        Gdiplus::Pen pen(
            Gdiplus::Color(
                get_alpha(border),
                get_red(border),
                get_green(border),
                get_blue(border)),
            static_cast<Gdiplus::REAL>(borderWidth));

        g.DrawPath(&pen, &path);
    }
}
#endif

void graphic::draw_rect(const rect position, const color border_color,
    const color fill_color, const int32_t border_width, int32_t rnd)
{
    rnd = clamp_corner_radius(position, rnd);
#ifdef _WIN32
    DrawRoundBox(mem_dc, position, rnd, border_width, fill_color, border_color);
#elif __linux__
    auto cr = cairo_create(surface);

    cairo_new_sub_path(cr);
    const double l = position.left,
        t = position.top,
        width = position.width(),
        height = position.height();
    if (rnd <= 0)
    {
        cairo_rectangle(cr, l + 0.5, t + 0.5, width - 0.5, height - 0.5);
    }
    else
    {
        constexpr double degrees = M_PI / 180.0;

        cairo_arc(cr, l + width - rnd, t + rnd, rnd, -90 * degrees, 0 * degrees);
        cairo_arc(cr, l + width - rnd, t + height - rnd, rnd, 0 * degrees, 90 * degrees);
        cairo_arc(cr, l + rnd, t + height - rnd, rnd, 90 * degrees, 180 * degrees);
        cairo_arc(cr, l + rnd, t + rnd, rnd, 180 * degrees, 270 * degrees);
        cairo_close_path(cr);
    }


    if (0 != get_alpha(fill_color))
    {
        if(0 == border_width && 0 == rnd)
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        cairo_set_source_rgba(cr,
            get_red(fill_color) / 255.0,
            get_green(fill_color) / 255.0,
            get_blue(fill_color) / 255.0,
            get_alpha(fill_color) / 255.0
        );
        cairo_fill_preserve(cr);
    }

    if (border_width > 0 && 0 != get_alpha(border_color))
    {
        if(0 == rnd)
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

        cairo_set_source_rgba(cr,
            get_red(border_color) / 255.0,
            get_green(border_color) / 255.0,
            get_blue(border_color) / 255.0,
            get_alpha(border_color) / 255.0
        );

        cairo_set_line_width(cr, border_width);
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
#endif
}

void graphic::draw_buffer(const rect position, uint8_t *buffer,
    const int32_t left_shift, const int32_t top_shift, const int32_t stride_data)
{
#ifdef _WIN32

    if (!mem_dc)
        return;

    auto source_bitmap = pc.get_bitmap(position.width(), position.height(), buffer, mem_dc);
    auto source_dc = CreateCompatibleDC(mem_dc);
    SelectObject(source_dc, source_bitmap);

    BitBlt(mem_dc,
        position.left,
        position.top,
        position.width(),
        position.height(),
        source_dc,
        left_shift,
        top_shift,
        SRCCOPY);

    DeleteDC(source_dc);

#elif __linux__

    if (!context_.connection)
        return;

    //TODO: Not tested!

    // faster than image : load_image_from_data() ?

    // CAIRO_FORMAT_ARGB32 => 0xAARRGGBB
    const int32_t width = position.width(), height = position.height();
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, position.width());
    cairo_surface_t* image_surface;
    if (0 == stride_data || stride_data >= stride)
    {
        image_surface = cairo_image_surface_create_for_data(
            buffer, // read directly from buffer
            CAIRO_FORMAT_ARGB32,
            width, height,
            stride_data ? stride_data : stride
        );

        if (cairo_surface_status(image_surface) != CAIRO_STATUS_SUCCESS)
        {
            err.set(error_type::no_handle, "graphic::draw_buffer()", "the object 'image_surface' are already not create");
            return;
        }
    }
    else
    {
        //image_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        image_surface = cairo_surface_create_similar_image(surface, CAIRO_FORMAT_ARGB32, width, height);
        if (cairo_surface_status(image_surface) != CAIRO_STATUS_SUCCESS)
        {
            err.set(error_type::no_handle, "graphic::draw_buffer()", "the object 'image_surface' are already not create");
            return;
        }

        uint8_t* data = cairo_image_surface_get_data(image_surface);

# ifndef NDEBUG
        if (nullptr == data)
        {
            cairo_surface_destroy(image_surface);
            return;
        }
# endif

        const ptrdiff_t stride = cairo_image_surface_get_stride(image_surface);
        for (uint32_t y = 0; y < height; ++y)
        {
            std::memcpy(data + y * stride, buffer + y * stride_data, width * 4);
        }

        cairo_surface_mark_dirty(image_surface);
        cairo_surface_flush(surface);
    }

    auto cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, position.left, position.top, width, height);

    cairo_clip(cr);
    cairo_set_source_surface(cr, image_surface, position.left - left_shift, position.top - top_shift);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(image_surface);

#endif
}

void graphic::copy_area(const rect position, graphic &gr_src, const int32_t left_shift, const int32_t top_shift)
{
#ifdef _WIN32
    if (gr_src.mem_dc)
    {
        BitBlt(mem_dc,
            position.left,
            position.top,
            position.width(),
            position.height(),
            gr_src.mem_dc,
            left_shift,
            top_shift,
            SRCCOPY);
    }
#elif __linux__
    auto cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, position.left, position.top, position.width(), position.height());
    cairo_clip(cr);

    cairo_set_source_surface(cr, gr_src.surface, position.left - left_shift, position.top - top_shift);
    cairo_paint(cr);

    cairo_destroy(cr);
#endif
}

#if __linux__

/// workarounds
void graphic::draw_surface(cairo_surface_t* surface_, const rect position__,
    const bool grayscale)
{
    auto cr = cairo_create(surface);

    const auto surface_width = cairo_image_surface_get_width(surface_);
    const auto surface_height = cairo_image_surface_get_height(surface_);

    double x_scale_factor = 1.0, y_scale_factor = 1.0;

    if (surface_width != 0 && surface_height != 0)
    {
        x_scale_factor = static_cast<double>(position__.width()) / surface_width;
        y_scale_factor = static_cast<double>(position__.height()) / surface_height;

        cairo_scale(cr, x_scale_factor, y_scale_factor);
    }

    if (grayscale)
    {
        cairo_set_operator(cr, CAIRO_OPERATOR_HSL_LUMINOSITY);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.4);
    }

    cairo_set_source_surface(cr,
        surface_,
        position__.left / x_scale_factor,
        position__.top / y_scale_factor);

    cairo_paint(cr);

    cairo_destroy(cr);
}

#endif

error graphic::get_error()
{
    return err;
}

/// Text measurer //////////////////////////

namespace
{
graphic* tm_graphic = nullptr;
}
// TODO: для совместимости с стилем кодирования, когда
// различные вызовы требуют инициализации окна, создают временное невидимое окно,
// для измерений текста так же возможно использовать внешние библиотеки

void graphic::set_text_measurer(graphic* gr) noexcept
{
    tm_graphic = gr;
}

graphic* graphic::get_text_measurer() noexcept
{
    return tm_graphic;
}

bool graphic::text_measurer_inited() noexcept
{
    return nullptr != tm_graphic;
}
bool graphic::is_me_text_measurer(const graphic& gr) noexcept
{
#ifdef _WIN32
    return tm_graphic && gr.context_.hwnd == tm_graphic->context_.hwnd;
#elif __linux__
    return tm_graphic && gr.context_.wnd == tm_graphic->context_.wnd;
#endif
}

int32_t font_max_height(const font &font_, graphic* gr)
{
    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return font_.size;
    }
    return gr->font_max_height(font_);
}

/// recommended distance between the baselines of two adjacent lines of text
int32_t font_line_height(const font &font_, graphic* gr, int32_t* max_height)
{
    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        if(max_height) *max_height = font_.size;
        return font_.size;
    }
    return gr->font_line_height(font_, max_height);
}

#ifdef _WIN32
int32_t font_line_height_gdiplus(const font &font_, graphic* gr)
{
    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return font_.size;
    }
    return gr->font_line_height_gdiplus(font_);
}
#endif

namespace
{
std::unordered_map<std::string, std::pair<int32_t, int32_t>> tm_cache;
}

static size_t font_hash(const font &font_)
{
    std::hash<std::string> hasher;

    return hasher(font_.name + std::to_string(font_.size));
}

rect measure_text(std::string_view text, const font &font_, graphic* gr)
{
    if (text.empty())
    {
        return { 0, 0, 0, font_.size };
    }

    std::hash<std::string> hasher;
    // Compute the hash value
    auto stringHash = hasher(std::string(text));
    auto fontHash = font_hash(font_);
    auto hash = std::to_string(stringHash) + "_" + std::to_string(fontHash);

    // Find in cache if finded - return
    auto it = tm_cache.find(hash);
    if (it != tm_cache.end())
    {
        return { 0, 0, it->second.first, it->second.second };
    }

    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return { 0, 0, 0, font_.size };
    }

    // Measure and cache
    const auto pos = gr->measure_text(text, font_);
    tm_cache[hash] = { pos.width(), pos.height() };
    return pos;
}

rect measure_text_direct(std::string_view text, const font &font_, graphic* gr)
{
    if (text.empty())
    {
        return { 0, 0, 0, font_.size };
    }
    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return { 0, 0, 0, font_.size };
    }
    return gr->measure_text(text, font_);
}

#ifdef _WIN32

namespace
{
//NB: width, height gdi+ отличаются от сохраненных в tm_cache
std::unordered_map<std::string, std::pair<int32_t, int32_t>> tm_cache_gdi;
}

rect measure_text_gdiplus(std::string_view text, const font &font_, graphic* gr)
{
    if (text.empty())
    {
        return { 0, 0, 0, font_.size };
    }

    std::hash<std::string> hasher;

    // Compute the hash value
    auto stringHash = hasher(std::string(text));
    auto fontHash = font_hash(font_);
    auto hash = std::to_string(stringHash) + "_" + std::to_string(fontHash);

    // Find in cache if finded - return
    auto it = tm_cache_gdi.find(hash);
    if (it != tm_cache_gdi.end())
    {
        return { 0, 0, it->second.first, it->second.second };
    }

    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return { 0, 0, 0, font_.size };
    }

    // Measure and cache
    auto pos = gr->measure_text_gdiplus(text, font_);
    tm_cache_gdi[hash] = { pos.width(), pos.height() };
    return pos;
}

rect measure_text_gdiplus_direct(std::string_view text, const font &font_, graphic* gr)
{
    if (text.empty())
    {
        return { 0, 0, 0, font_.size };
    }

    gr = (gr && gr->inited()) ? gr : (tm_graphic && tm_graphic->inited()) ? tm_graphic : nullptr;
    if (!gr)
    {
        return { 0, 0, 0, font_.size };
    }
    return gr->measure_text_gdiplus(text, font_);
}
#endif

}
