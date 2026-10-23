//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/graphic/primitive_container.hpp>

#include <boost/nowide/convert.hpp>

#ifdef _WIN32
#include <strsafe.h>
#elif __linux__
#include <xcb/xcb_image.h>
#include <cairo.h>
#include <cairo-xcb.h>
#include <cmath>

//#define _CAIRO_RESIZE_FONT
#endif

namespace wui
{

primitive_container::~primitive_container()
{
    release();
}

#ifdef _WIN32

void primitive_container::init()
{
    err.reset();
}

void primitive_container::release()
{
    for (auto &p : pens)
    {
        DeleteObject(p.second);
    }
    pens.clear();

    for (auto &b : brushes)
    {
        DeleteObject(b.second);
    }
    brushes.clear();

    for (auto &f : fonts)
    {
        DeleteObject(f.second);
    }
    fonts.clear();

    for (auto &b : bitmaps)
    {
        DeleteObject(b.second);
    }
    bitmaps.clear();
}

HPEN primitive_container::get_pen(const int32_t style, const int32_t width, const color color_)
{
    auto it = pens.find({ { style, width }, color_ });
    if (it != pens.end())
    {
        return it->second;
    }
    auto pen = CreatePen(style, width, get_rgb0(color_));

    pens[{ { style, width }, color_ }] = pen;

    return pen;
}

HBRUSH primitive_container::get_brush(const color color_)
{
    if (is_alpha(color_)) // TODO: alpha
    {
        return (HBRUSH)GetStockObject(NULL_BRUSH);
    }
    auto it = brushes.find(color_);
    if (it != brushes.end())
    {
        return it->second;
    }

    auto brush = CreateSolidBrush(get_rgb0(color_));

    brushes[color_] = brush;

    return brush;
}

// TODO: font_size = -ScaleDPI(font_.size, dpi_); // -?
// fscale = (float) g_dpi / USER_DEFAULT_SCREEN_DPI;
HFONT primitive_container::get_font(const font& font_)
{
    auto it = fonts.find({ { font_.name, font_.size }, font_.decorations_ });
    if (it != fonts.end())
    {
        return it->second;
    }

    LOGFONTW log_font = {
       -font_.size,
        0,
        0,
        0,
        (font_.decorations_ & decorations::bold) ? FW_MEDIUM : FW_DONTCARE,
        0 != (font_.decorations_ & decorations::italic),
        0 != (font_.decorations_ & decorations::underline),
        0 != (font_.decorations_ & decorations::strike_out),
        ANSI_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        0
    };
    std::wstring font_name = std::move(boost::nowide::widen(font_.name));
    StringCchCopyW(log_font.lfFaceName, LF_FACESIZE, font_name.c_str());
    HFONT font__ = CreateFontIndirectW(&log_font);

    fonts[{ {font_.name, font_.size }, font_.decorations_ }] = font__;

    return font__;
}

HBITMAP primitive_container::get_bitmap(const int32_t width, const int32_t height,
    uint8_t *buffer, const HDC hdc)
{
    auto it = bitmaps.find({ width, height });
    if (it != bitmaps.end())
    {
        auto bitmap = it->second;

        BITMAPINFO bmpInfo;

        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFO) - sizeof(RGBQUAD);
        bmpInfo.bmiHeader.biWidth = width;
        bmpInfo.bmiHeader.biHeight = 0 - (LONG)height;
        bmpInfo.bmiHeader.biPlanes = 1;
        bmpInfo.bmiHeader.biBitCount = 32;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage = 0;
        bmpInfo.bmiHeader.biXPelsPerMeter = 0;
        bmpInfo.bmiHeader.biYPelsPerMeter = 0;
        bmpInfo.bmiHeader.biClrUsed = 0;
        bmpInfo.bmiHeader.biClrImportant = 0;

        SetDIBits(hdc, bitmap, 0, height, buffer, &bmpInfo, DIB_RGB_COLORS);

        return bitmap;
    }

    auto bitmap = CreateBitmap(width, height, 1, 32, buffer);

    bitmaps[{ width, height }] = bitmap;

    return bitmap;
}

#elif __linux__

void primitive_container::init()
{
    err.reset();
}

void primitive_container::release()
{
    if (context_.connection)
    {
        for (auto &g : gcs)
        {
            xcb_free_gc(context_.connection, g.second);
        }
        gcs.clear();
    }

    for (auto &f : fonts)
    {
    	cairo_destroy(f.second);
    }
    fonts.clear();
}

xcb_gcontext_t primitive_container::get_gc(const color color_)
{
    if (!context_.connection)
    {
        err.set(error_type::no_handle, "primitive_container::get_gc(color)", "no context_.connection");
        return -1;
    }

    auto it = gcs.find(color_);
    if (it != gcs.end())
    {
        return it->second;
    }

    auto gc = xcb_generate_id(context_.connection);

    uint32_t mask = XCB_GC_FOREGROUND;
    uint32_t value[] = { conv_to_bgr0(color_) };
    auto gc_create_cookie = xcb_create_gc(context_.connection, gc, context_.wnd, mask, value);

    gcs[color_] = gc;

    return gc;
}

_cairo *primitive_container::get_font(const font& font_, _cairo_surface *surface)
{
    if (!surface)
    {
        return nullptr;
    }

    auto it = fonts.find({ {font_.name, font_.size }, font_.decorations_ });
    if (it != fonts.end())
    {
        return it->second;
    }

    auto cr = cairo_create(surface);
    if (!cr)
    {
        return nullptr;
    }

    cairo_select_font_face(cr, font_.name.c_str(),
        !(font_.decorations_ & decorations::italic) ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_SLANT_ITALIC,
        !(font_.decorations_ & decorations::bold) ? CAIRO_FONT_WEIGHT_NORMAL : CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_.size);

#if 1
    // NB: Текст становится таким же четким как win32 (10, 11 windows)
    cairo_font_options_t* font_options = cairo_font_options_create();
    cairo_get_font_options(cr, font_options);

    cairo_font_options_set_antialias(font_options, font_.size < 9 ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_SUBPIXEL);


    // ВАЖНО ДЛЯ DPI:  хинтинг метрик
    /*

    Что делает Хинтинг метрик (Hint Metrics)

    Хинтинг — это процесс подгонки контуров и размеров букв под физическую пиксельную
    сетку монитора, чтобы они оставались читаемыми на низких разрешениях.
    1. При CAIRO_HINT_METRICS_ON (Поведение по умолчанию):
        • Cairo округляет ширину каждой буквы и межсимвольные интервалы до целых
            физических пикселей.
        • Плюс: Вертикальные линии букв (например, у «Ш», «Н», «I») всегда
            попадают четко в физические пиксели. Буквы получаются максимально контрастными, а размытие по горизонтали сводится к нулю.
        • Минус: При дробных масштабах (1.25, 1.5) округление до целого
            пикселя работает грубо. Буквы начинают «плясать»: в одном слове две
            одинаковые буквы «о» могут иметь разную ширину (одна 7 пикселей, другая 8),
            а межсимвольные интервалы визуально схлопываются или расширяются.
            Текст выглядит «дерганым».

        на высоких DPI может делать шрифт избыточно ломаным и неестественным.

    2. При CAIRO_HINT_METRICS_OFF:
        • Cairo рассчитывает ширину букв и интервалы с субпиксельной точностью
            (дробные физические пиксели, например, ширина буквы равна 10.43 пикселя).
        • Плюс: Форма слова и межсимвольные расстояния масштабируются идеально
            линейно и плавно при любом DPI scale (хоть 1.0, хоть 1.37).
            Буквы больше не слипаются и не прыгают.
        • Минус: Так как границы букв теперь могут начинаться на 0.43 физического пикселя,
            Cairo включает горизонтальное субпиксельное сглаживание (Anti-aliasing).
            Из-за этого края букв могут казаться более мягкими («размытыми») на низких DPI
            мониторах по сравнению с жестким хинтингом.

    */
    // Выключаем жесткую привязку букв к сетке физических пикселей (спасает дробный масштаб)
    // CAIRO_HINT_METRICS_OFF	Убирает эффект "слипания" или разной толщины букв в словах при масштабе 1.25 / 1.5.
    cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_OFF);

    // SLIGHT (слабый хинтинг) выравнивает буквы по вертикальной сетке пикселей
    // (убирая размытие), но позволяет межсимвольным интервалам и ширине букв
    // слегка варьироваться, что спасает текст от «слипания» при дробном Fractional Scaling.

    // Настройка Хинтинга (Hinting — подгонка контура букв под пиксели)
    // CAIRO_HINT_STYLE_FULL делает текст максимально контрастным и резким,
    // что отлично для мелких шрифтов.

    // Если хинтинг в системе не задан, выставляем безопасный SLIGHT
    if (cairo_font_options_get_hint_style(font_options) == CAIRO_HINT_STYLE_DEFAULT)
    {
        // scale <= 1 and 1, 2, 4 ... CAIRO_HINT_STYLE_FULL
        // (static_cast<int32_t>(scale) == scale) && (scale_int & (scale_int - 1)) == 0
        cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_FULL);
        // DPI scale дробные размеры
        //cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_SLIGHT);
    }

    cairo_set_font_options(cr, font_options);
    cairo_font_options_destroy(font_options);
#endif


#ifdef _CAIRO_RESIZE_FONT
    // linux: graphic::get_font_max_h(font ) { ...; return font_extents.ascent + font_extents.descent;}
    // win32: graphic::get_font_max_h(font font_) { return font_.size; }

    cairo_font_extents_t font_extents;
    cairo_font_extents(cr, &font_extents);
    double h = font_extents.ascent + font_extents.descent;
    if (h > font_.size)
    {
        h = font_.size * (font_.size / h);
        cairo_set_font_size(cr, std::ceil(h)); // font_.size set
    }
#endif

    fonts[{ {font_.name, font_.size }, font_.decorations_ }] = cr;

    return cr;
}

#endif

wui::error primitive_container::get_error()
{
    return err;
}

}
