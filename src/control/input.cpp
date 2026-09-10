//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/input.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/system/clipboard_tools.hpp>

#include <wui/locale/locale.hpp>

#include <wui/common/dbgtrace.hpp>

#include <boost/nowide/convert.hpp>
#include <utf8/utf8.h>

#include <regex>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif

namespace wui
{

input::input(std::string_view text__, input_view input_view__,
    input_content input_content__, int32_t symbols_limit_,
    std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : input_view_(input_view__),
    input_content_(input_content__),
    symbols_limit(symbols_limit_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    timer_(std::bind(&input::redraw_cursor, this)),
    menu_(std::make_shared<menu>(menu::tc, theme_)),
    vert_scroll(std::make_shared<scroll>(0, 0, orientation::vertical,
        std::bind(&input::on_vert_scroll, this, std::placeholders::_1, std::placeholders::_2), scroll::tc, theme__)),
    hor_scroll(std::make_shared<scroll>(0, 0, orientation::horizontal,
        std::bind(&input::on_hor_scroll, this, std::placeholders::_1, std::placeholders::_2), scroll::tc, theme__)),
    showed_(true), enabled_(true), topmost_(false),
    active(false), focused_(false),
    cursor_visible(false),
    selecting(false),
    auto_scroll_timer_(std::make_shared<timer>([this]() { on_auto_scroll(); }))
{
    focus_mode_ = check_focus_mode();
    update_theme();
    update_lines(text__);
    reset_state();

    menu_->set_items({
            { 0, menu_item_state::normal, locale(tc, cl_cut).data(), "Ctrl+X", nullptr, {}, [this](int32_t) { buffer_cut(); } },
            { 1, menu_item_state::normal, locale(tc, cl_copy).data(), "Ctrl+C", nullptr, {}, [this](int32_t) { buffer_copy(); } },
            { 2, menu_item_state::normal, locale(tc, cl_paste).data(), "Ctrl+V", nullptr, {}, [this](int32_t) { buffer_paste(); } }
        });
}

input::~input()
{
    stop_auto_scroll();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

static int32_t get_text_width(std::string text, const font& font_)
{
    if (text.empty())
        return 0;
    const auto text_rect = measure_text(text, font_);
    return text_rect.right;
}

// Auxiliary function: comparing cursor positions (row, col)
inline bool cursor_less(const size_t row1, const size_t col1, const size_t row2, const size_t col2)
{
    return row1 < row2 || (row1 == row2 && col1 < col2);
}

// Auxiliary functions for working with byte indexes (as in single-line mode)
// char_pos: utf8 char pos
static size_t get_byte_pos_for_char_pos(const std::string& s, const size_t char_pos)
{
    if (s.empty() || 0 == char_pos)
    {
        return 0;
    }

    auto it = s.begin();
    const size_t actual_chars = utf8::distance(it, s.end());

    if (char_pos >= actual_chars)
    {
        return s.size(); // Returning the end of the string
    }
    utf8::advance(it, char_pos, s.end());
    return std::distance(s.begin(), it);
}

font input::get_font()
{
    auto font_ = std::move(theme_font(tcn, tv_font, theme_));
    if (input_view_ == input_view::password)
    {
#ifdef _WIN32
        font_.name = "Courier New";
#elif __linux__
        font_.name = "monospace";
#endif
    }
    return std::move(font_);
}

int32_t input::get_font_size() const
{
    return theme_data_.font_.size;
}

bool input::update_mem_gr()
{
    if (!mem_gr)
        return false;

    auto width = position_.width() - 2 * (theme_data_.border_width + theme_data_.item_indent);
    auto height = position_.height() - 2 * theme_data_.border_width - theme_data_.item_indent;

    auto current = mem_gr->max_size();

    if (current.width() < width || current.height() < height)
    {
        mem_gr->release();
        return mem_gr->init({ 0, 0, width, height }, theme_data_.background);
    }

    mem_gr->clear();
    return true;
}

void input::draw(graphic& gr, const rect& paint_rect [[maybe_unused]] )
{
    if (!showed_ || position_.is_hide() || !update_mem_gr())
    {
        return;
    }

    /// Draw the frame (gr)
    const auto control_pos = position();
    if (theme_data_.round)
    {
        gr.draw_rect(control_pos, make_color(0, 0, 0, 0),
            theme_data_.background, 0, theme_data_.round);
    }

    const int line_height = mem_gr->get_font_ideal_height(theme_data_.font_); // line space
    if (line_height > 0)
    {
        // We start from the scroll position
        const int32_t y = -(scroll_offset_y % line_height);
        const size_t start_line = scroll_offset_y / line_height;
        const int32_t content_height = control_pos.height()
            - (input_view_ == input_view::multiline ? scroll::full_scrollbar_size : 0);

        const int32_t visible_bottom = content_height + line_height;
        const size_t count = input_view_ == input_view::multiline ? lines_.size() : 1;
        for (size_t i = start_line; i < count; ++i)
        {
            int32_t actual_y = y + static_cast<int32_t>(i - start_line) * line_height;
            if (actual_y >= visible_bottom
                || actual_y < -line_height)
                break;

            if (input_view_ != input_view::multiline && count == 1)
            {
                actual_y = position_.height() > theme_data_.font_.size + 2 * theme_data_.border_width ?
                    (position_.height() - theme_data_.font_.size) / 2  - theme_data_.border_width: theme_data_.border_width;
            }

            // Highlighting the selection
            if (((i >= select_start_row && i <= select_end_row)
                || (i >= select_end_row && i <= select_start_row))
                && (select_start_col != select_end_col || select_start_row != select_end_row))
            {
                bool has_sel = false;
                ptrdiff_t sel_start = 0, sel_end = 0;
                size_t srow = select_start_row, scol = select_start_col,
                    erow = select_end_row, ecol = select_end_col;
                const auto sel_full = utf8::distance(lines_[i].begin(), lines_[i].end());
                if (cursor_less(erow, ecol, srow, scol))
                {
                    std::swap(srow, erow), std::swap(scol, ecol);
                }
                if (i > srow && i < erow)
                {
                    has_sel = true; sel_start = 0; sel_end = sel_full;
                }
                else if (i == srow && i == erow && scol != ecol)
                {
                    has_sel = true; sel_start = scol; sel_end = ecol;
                    if (sel_start > sel_end) std::swap(sel_start, sel_end);
                    if (sel_end > sel_full) sel_end = sel_full;
                }
                else if (i == srow && i < erow)
                {
                    has_sel = true; sel_start = scol; sel_end = sel_full;
                }
                else if (i == erow && i > srow)
                {
                    has_sel = true; sel_start = 0; sel_end = ecol;
                    if (sel_end > sel_full) sel_end = sel_full;
                }

                if (has_sel && sel_start < sel_end)
                {
                    const size_t start_byte = get_byte_pos_for_char_pos(lines_[i], sel_start);
                    const size_t end_byte = get_byte_pos_for_char_pos(lines_[i], sel_end);
                    const int32_t x1 = measure_text(lines_[i].substr(0, start_byte), theme_data_.font_, mem_gr.get()).right
                        - scroll_offset_x + horizontal_indent;
                    const int32_t x2 = measure_text(lines_[i].substr(0, end_byte), theme_data_.font_, mem_gr.get()).right
                        - scroll_offset_x + horizontal_indent;
                    mem_gr->draw_rect({ x1, actual_y, x2, actual_y + theme_data_.font_.size }, theme_data_.selection);
                }
            }

            if (input_view_ != input_view::password)
            {
                mem_gr->draw_text({ horizontal_indent - scroll_offset_x, actual_y },
                    lines_[i], theme_data_.text, theme_data_.font_);
            }
            else
            {
                std::string str; str.resize(lines_[i].size(), '*');
                mem_gr->draw_text({ horizontal_indent - scroll_offset_x, actual_y },
                    str, theme_data_.text, theme_data_.font_);
            }

            // Cursor
            if (cursor_visible && i == cursor_row)
            {
                const size_t max_col = utf8::distance(lines_[i].begin(), lines_[i].end());
                const size_t safe_cursor_col = std::min(cursor_col, max_col);
                const size_t cursor_byte = get_byte_pos_for_char_pos(lines_[i], safe_cursor_col);
                const int32_t cursor_x = measure_text(lines_[i].substr(0, cursor_byte),
                    theme_data_.font_, mem_gr.get()).right - scroll_offset_x + horizontal_indent;
                mem_gr->draw_line({ cursor_x, actual_y, cursor_x,
                    actual_y + theme_data_.font_.size }, theme_data_.cursor);
            }
        }
    }

    // Copying the offscreen buffer to the parent context
    gr.copy_area(control_pos, *mem_gr,
        -theme_data_.border_width - theme_data_.item_indent,
        -theme_data_.border_width);

    if (input_view_ == input_view::multiline)
    {
        // Rendering scrollbars
        if (vert_scroll->showed())
        {
            vert_scroll->draw(gr, {});
        }
        if (hor_scroll->showed())
        {
            hor_scroll->draw(gr, {});
        }
    }

    if (theme_data_.border_width)
    {
        const auto border_color = focused_ ? theme_data_.focused_border :
            (!active ? theme_data_.border : theme_data_.hover_border);
        gr.draw_rect(control_pos,
            border_color, make_color(0, 0, 0, 0), theme_data_.border_width, theme_data_.round);
    }
}

static bool is_number(std::string_view s, std::string_view line)
{
    //assert(!s.empty());

    char ch_punc = '.';
    std::locale lcl;
    if (std::has_facet<std::numpunct<char>>(lcl))
    {
        const auto& facet = std::use_facet<std::numpunct<char>>(lcl);
        ch_punc = facet.decimal_point();
    }

#ifndef NDEBUG
    char ch_punct_c = 0;
    auto* le = localeconv();
    if (le && le->decimal_point)
    {
        ch_punct_c = le->decimal_point[0];
    }
    if (ch_punc && ch_punct_c && ch_punc != ch_punct_c)
    {
        std::cerr << "numeric punctuation : 'C' different 'STD lib', use std::locale::global()" << std::endl;
    }
#endif

    const char chset[]{ ch_punc, '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E', '\0' };
    const char ch = s.data()[0];
    return s.find_first_not_of(chset) == std::string::npos
         && !std::any_of(line.cbegin(), line.cend(), [ch, ch_punc](const char c) {
            return (c == ch_punc || '-' == c || 'e' == c || 'E' == c)
                && std::toupper(ch) == std::toupper(c); })
        ;
}

// Auxiliary function for multiline
std::pair<size_t, size_t> input::calculate_mouse_cursor_position(const int x, const int y)
{
    // Protection against empty lines_
    if (lines_.empty())
    {
        return { 0, 0 };
    }

    // Fallback to minimum height
    const int line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space

    auto control_pos = position();

    // We take into account scrolling and borders
    const int rel_y = y - control_pos.top + theme_data_.border_width + scroll_offset_y;
    const size_t row = std::min(static_cast<size_t>(rel_y / line_height), lines_.size() - 1);

    const int rel_x = x - control_pos.left + theme_data_.border_width - horizontal_indent + scroll_offset_x;

    // We use character positions to measure text
    const auto& line = lines_[row];
    const size_t char_count = utf8::distance(line.begin(), line.end());

    size_t col = 0;
    for (; col <= char_count; ++col)
    {
        const size_t byte_pos = get_byte_pos_for_char_pos(line, col);
        const int w = measure_text(line.substr(0, byte_pos), theme_data_.font_).right;
        if (w > rel_x)
            break;
    }

    if (col > 0)
    {
        --col;
    }
    if (col >= char_count)
    {
        col = char_count;
    }

    return { row, col };
}

void input::remove_focus()
{
    focused_ = false;
    selecting = false;
    cursor_visible = false;
    stop_auto_scroll();
    timer_.stop();
    if (return_callback)
    {
        return_callback();
    }
    redraw();
}

void input::receive_control_events(const event& ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    // Scrollbar event handling for multiline
    if (input_view_ == input_view::multiline)
    {
        if ((ev.type & event_type::mouse)
            && (ev.mouse_event_.type == mouse_event_type::move
                || ev.mouse_event_.type == mouse_event_type::enter))
        {
            if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)
                || hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))
            {
                set_cursor(parent_, cursor::default_);
            }
            else
            {
                set_cursor(parent_, cursor::ibeam);
            }
            // Enter/leave emulation for scrollbars
            bool vert_hover = vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y);
            bool hor_hover = hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y);
            static bool prev_vert_hover = false;
            static bool prev_hor_hover = false;
            if (vert_hover && !prev_vert_hover)
            {
                event enter_ev = ev;
                enter_ev.mouse_event_.type = mouse_event_type::enter;
                vert_scroll->receive_control_events(enter_ev);
            }
            else if (!vert_hover && prev_vert_hover)
            {
                event leave_ev = ev;
                leave_ev.mouse_event_.type = mouse_event_type::leave;
                vert_scroll->receive_control_events(leave_ev);
            }
            if (hor_hover && !prev_hor_hover)
            {
                event enter_ev = ev;
                enter_ev.mouse_event_.type = mouse_event_type::enter;
                hor_scroll->receive_control_events(enter_ev);
            }
            else if (!hor_hover && prev_hor_hover)
            {
                event leave_ev = ev;
                leave_ev.mouse_event_.type = mouse_event_type::leave;
                hor_scroll->receive_control_events(leave_ev);
            }
            prev_vert_hover = vert_hover;
            prev_hor_hover = hor_hover;
        }

        if (!selecting)
        {
            // Checking whether the cursor is above the scrollbars for mouse events
            if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))
            {
                vert_scroll->receive_control_events(ev);
                return;
            }
            if (hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))
            {
                hor_scroll->receive_control_events(ev);
                return;
            }
        }
    }

    if (ev.type & event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
            {
                active = true;
                set_cursor(parent_, cursor::ibeam);
                stop_auto_scroll();
                redraw();
            }
            break;
            case mouse_event_type::leave:
            {
                active = false;
                redraw();
                if (selecting)
                {
                    auto control_pos = position();

                    if (ev.mouse_event_.x < control_pos.left && cursor_col > 0)
                    {
                        start_auto_hscroll(true); // left
                        return;
                    }

                    if (ev.mouse_event_.x > control_pos.right && cursor_row < lines_.size())
                    {
                        start_auto_hscroll(false); // right
                        return;
                    }

                    if (input_view_ == input_view::multiline)
                    {
                        if (ev.mouse_event_.y < control_pos.top && cursor_row > 0)
                        {
                            start_auto_scroll(true); // up
                            return;
                        }

                        if (ev.mouse_event_.y > control_pos.bottom && cursor_row + 1 < lines_.size())
                        {
                            start_auto_scroll(false); // down
                            return;
                        }
                    }
                    return;
                }

                set_cursor(parent_, cursor::default_);
            }
            break;
            case mouse_event_type::left_down:
            {
                auto [row, col] = calculate_mouse_cursor_position(ev.mouse_event_.x, ev.mouse_event_.y);
                cursor_row = row;
                cursor_col = col;
                select_start_row = select_end_row = cursor_row;
                select_start_col = select_end_col = cursor_col;
                selecting = true;
                scroll_to_cursor();
                redraw();
            }
            break;
            case mouse_event_type::left_up:
                selecting = false;
                stop_auto_scroll();
                menu_->hide();
            break;
            case mouse_event_type::right_up:
                if(!menu_->showed())
                {
                    bool has_selection = !(select_start_row == select_end_row && select_start_col == select_end_col);
                    menu_->update_item({ 0, has_selection && input_view_ != input_view::readonly && input_view_ != input_view::password ? menu_item_state::normal : menu_item_state::disabled,
                        locale(tc, cl_cut).data(), "Ctrl+X", nullptr, {}, [this](int32_t) { buffer_cut(); auto p = parent_.lock(); if (p) p->set_focused(shared_from_this()); } });
                    menu_->update_item({ 1, has_selection && input_view_ != input_view::password ? menu_item_state::normal : menu_item_state::disabled,
                        locale(tc, cl_copy).data(), "Ctrl+C", nullptr, {}, [this](int32_t) { buffer_copy(); auto p = parent_.lock(); if (p) p->set_focused(shared_from_this()); } });
                    menu_->update_item({ 2, input_view_ != input_view::readonly ? menu_item_state::normal : menu_item_state::disabled,
                        locale(tc, cl_paste).data(), "Ctrl+V", nullptr, {}, [this](int32_t) { buffer_paste(); auto p = parent_.lock(); if (p) p->set_focused(shared_from_this()); } });

                    menu_->show_on_control(shared_from_this(), 0, ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::move:
                if (selecting)
                {
                    // Обычная обработка для видимой области
                    auto [row, col] = calculate_mouse_cursor_position(ev.mouse_event_.x, ev.mouse_event_.y);
                    cursor_row = row;
                    cursor_col = col;
                    select_end_row = row;
                    select_end_col = col;
                    scroll_to_cursor();
                    redraw();
                }
                break;
            case mouse_event_type::left_double:
                select_current_word(ev.mouse_event_.x, ev.mouse_event_.y);
                break;
            case mouse_event_type::wheel:
                if (ev.mouse_event_.wheel_delta > 0)
                {
                    vert_scroll->scroll_up();
                }
                else
                {
                    vert_scroll->scroll_down();
                }
                break;
            default: break;
        }
    }
    else if (ev.type & event_type::keyboard)
    {
        switch (ev.keyboard_event_.type)
        {
            case keyboard_event_type::down:
            {
                timer_.stop();
                cursor_visible = true;
                const bool shift = (ev.keyboard_event_.modifier == vk_lshift || ev.keyboard_event_.modifier == vk_rshift);

                switch (ev.keyboard_event_.key[0])
                {
                    case vk_left:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            if (cursor_col > 0)
                            {
                                --cursor_col;
                            }
                            else if (cursor_row > 0)
                            {
                                --cursor_row;
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            }
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row;
                                select_start_col = old_col;
                                selecting = true;
                            }
                        }
                        else
                        {
                            if (cursor_col > 0)
                            {
                                --cursor_col;
                            }
                            else if (cursor_row > 0)
                            {
                                --cursor_row;
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            }
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_right:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            const size_t max_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            if (cursor_col < max_col)
                            {
                                ++cursor_col;
                            }
                            else if (cursor_row + 1 < lines_.size())
                            {
                                ++cursor_row;
                                cursor_col = 0;
                            }
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row; select_start_col = old_col; selecting = true;
                            }
                        }
                        else
                        {
                            const size_t max_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            if (cursor_col < max_col)
                            {
                                ++cursor_col;
                            }
                            else if (cursor_row + 1 < lines_.size())
                            {
                                ++cursor_row;
                                cursor_col = 0;
                            }
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_up:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            if (cursor_row > 0)
                            {
                                --cursor_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            }
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row;
                                select_start_col = old_col;
                                selecting = true;
                            }
                        }
                        else
                        {
                            if (cursor_row > 0)
                            {
                                --cursor_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            }
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_down:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            if (cursor_row + 1 < lines_.size())
                            {
                                ++cursor_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            }
                            else
                            {
                                // Если уже на последней строке, двигаем курсор в конец строки
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            }
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row;
                                select_start_col = old_col;
                                selecting = true;
                            }
                        }
                        else
                        {
                            if (cursor_row + 1 < lines_.size())
                            {
                                ++cursor_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            }
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_home:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            cursor_col = 0;
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row;
                                select_start_col = old_col;
                                selecting = true;
                            }
                        }
                        else
                        {
                            cursor_col = 0;
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_end:
                        if (shift)
                        {
                            const size_t old_row = cursor_row, old_col = cursor_col;
                            // We use a symbolic position for the selection to work correctly
                            cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            select_end_row = cursor_row;
                            select_end_col = cursor_col;
                            if (!selecting)
                            {
                                select_start_row = old_row;
                                select_start_col = old_col;
                                selecting = true;
                            }
                        }
                        else
                        {
                            cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                        }
                        scroll_to_cursor();
                        redraw();
                        break;
                    case vk_back:
                        if (clear_selected_text())
                        {
                            update_scroll_areas();
                            scroll_to_cursor();
                            if (change_callback)
                            {
                                change_callback();
                            }
                            redraw();
                            break;
                        }
                        if (cursor_col > 0)
                        {
                            const auto prev_position = cursor_col;
                            --cursor_col;
                            // We get byte positions for correct deletion of UTF-8 characters
                            const size_t prev_byte = get_byte_pos_for_char_pos(lines_[cursor_row], prev_position);
                            const size_t curr_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                            lines_[cursor_row].erase(curr_byte, prev_byte - curr_byte);
                        }
                        else if (cursor_row > 0)
                        {
                            --cursor_row;
                            cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                            lines_[cursor_row] += lines_[cursor_row + 1];
                            lines_.erase(lines_.begin() + cursor_row + 1);
                        }
                        invalidate_max_width_cache();
                        update_scroll_areas();
                        scroll_to_cursor();
                        if (change_callback)
                        {
                            change_callback();
                        }
                        redraw();
                        break;
                    case vk_del:
                    {
                        if (clear_selected_text())
                        {
                            update_scroll_areas();
                            scroll_to_cursor();
                            if (change_callback)
                            {
                                change_callback();
                            }
                            redraw();
                            break;
                        }
                        if (cursor_col < lines_[cursor_row].size())
                        {
                            // We get byte positions for correct deletion of UTF-8 characters
                            const size_t start_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                            const size_t end_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col + 1);
                            lines_[cursor_row].erase(start_byte, end_byte - start_byte);
                        }
                        else if (cursor_row + 1 < lines_.size())
                        {
                            lines_[cursor_row] += lines_[cursor_row + 1];
                            lines_.erase(lines_.begin() + cursor_row + 1);
                        }
                        invalidate_max_width_cache();
                        update_scroll_areas();
                        scroll_to_cursor();
                        if (change_callback)
                        {
                            change_callback();
                        }
                        redraw();
                    }
                    break;
                    case vk_return: case vk_rreturn:
                        if (input_view_ == input_view::multiline)
                        {
                            // We get byte positions for correct operation with UTF-8
                            const size_t cursor_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                            const std::string new_line = lines_[cursor_row].substr(cursor_byte);
                            lines_[cursor_row].erase(cursor_byte);
                            lines_.insert(lines_.begin() + cursor_row + 1, new_line);
                            ++cursor_row;
                            cursor_col = 0;
                            invalidate_max_width_cache();
                            update_scroll_areas();
                            scroll_to_cursor();
                            if (return_callback)
                            {
                                return_callback();
                            } else if (change_callback) // нужен ли?
                            {
                                change_callback();
                            }
                            redraw();
                        }
                        else
                        {
                            if (input_view_ != input_view::readonly)
                            {
                                remove_focus();
                            }
                        }
                        break;
                    case vk_page_up: case vk_npage_up:
                        if (cursor_row > 0)
                        {
                            const int32_t content_height = position().height() - theme_data_.border_width * 2
                                - scroll::full_scrollbar_size;

                            const int32_t line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space
                            const int32_t visible_lines = std::max(1, content_height / line_height);
                            const size_t new_row = cursor_row > (size_t)visible_lines ? cursor_row - visible_lines : 0;
                            cursor_row = new_row;
                            cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                            scroll_to_cursor();
                            redraw();
                        }
                        break;
                    case vk_page_down: case vk_npage_down:
                        if (cursor_row + 1 < lines_.size())
                        {
                            const int32_t content_height = position().height() - theme_data_.border_width * 2
                                - scroll::full_scrollbar_size;
                            const int32_t line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space
                            const int32_t visible_lines = std::max(1, content_height / line_height);
                            const size_t new_row = std::min(cursor_row + visible_lines, lines_.size() - 1);
                            cursor_row = new_row;
                            cursor_col = std::min(cursor_col,
                                static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                            selecting = false;
                            select_start_row = select_start_col
                                = select_end_row = select_end_col = 0;
                            scroll_to_cursor();
                            redraw();
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
            case keyboard_event_type::up:
                timer_.start(500);

                if (ev.keyboard_event_.key[0] == vk_lshift || ev.keyboard_event_.key[0] == vk_rshift)
                {
                    selecting = false;
                }
                break;
            case keyboard_event_type::key:
                switch(ev.keyboard_event_.key[0])
                {
                    case 0x3:       // ctrl + c
                        return buffer_copy();
                    case 0x18:      // ctrl + x
                        return buffer_cut();
                    case 0x16:      // ctrl + v
                        return buffer_paste();
                    case 0x1:       // ctrl + a
                        return select_all();
                    case 0x7f:      // ctrl + backspace
                        if (input_view_ != input_view::readonly)
                        {
                            return update_lines("");
                        }
                        break;
                    case vk_return:       // '\r' return
                        if (input_view_ == input_view::singleline)
                        {
                            //remove_focus();
                            return;
                        }
                        break;
                    default:
                        break;
                }

                if (input_view_ == input_view::readonly ||
                    ev.keyboard_event_.key[0] == vk_tab ||

                    (symbols_limit != -1 && static_cast<int32_t>(text().size()) >= symbols_limit))
                {
                    return;
                }

                // Разрешить служебные клавиши (backspace, delete) независимо от input_content
                if (ev.keyboard_event_.key[0] == vk_back)
                {
                    // TODO: сюда вероятно не заходим, судя по тестам

                    // Эта клавиша обрабатываются в keyboard_event_type::down, пропускаем здесь
                    return;
                }

                // TODO:  win32 : 0x0E - 0x0F unassigned [Ctrl+n, etc]
                //       linux?

                if (input_content_ == input_content::integer &&
                    !std::isdigit(static_cast<int>(static_cast<unsigned char>(ev.keyboard_event_.key[0]))))
                {
                    return;
                }

                if (input_content_ == input_content::numeric &&
                    !is_number(ev.keyboard_event_.key, lines_[cursor_row]))
                {
                    return;
                }

                if (input_content_ == input_content::hexadecimal &&
                    !std::isxdigit(static_cast<int>(static_cast<unsigned char>(ev.keyboard_event_.key[0]))))
                {
                    return;
                }

                if (clear_selected_text())
                {
                    if (change_callback)
                        change_callback();
                    redraw();
                }

                if (symbols_limit != -1 && text().size() < (size_t)symbols_limit)
                {
                    if (ev.keyboard_event_.key[0] == 13 || ev.keyboard_event_.key[0] == 10)
                    {
                        return;
                    }
                    const size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                    lines_[cursor_row].insert(insert_byte, ev.keyboard_event_.key, ev.keyboard_event_.key_size);
                    cursor_col += utf8::distance(ev.keyboard_event_.key, ev.keyboard_event_.key + ev.keyboard_event_.key_size);
                    invalidate_max_width_cache();
                    update_scroll_areas();
                    scroll_to_cursor();
                    if (change_callback)
                        change_callback();
                    redraw();
                }
                break;
        }
    }
    else if (ev.type & event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;

                redraw();

                timer_.start(500);
                break;
            case internal_event_type::remove_focus:
                remove_focus();
                break;
        }
    }
}

void input::receive_plain_events(const event& ev)
{
    if ((ev.type & event_type::mouse) && ev.mouse_event_.type == mouse_event_type::left_up)
    {
        selecting = false;
    }
}

void input::move(const int32_t dx, const int32_t dy)
{
    position_.move(dx, dy);
    set_position(position_);
}

void input::update_scroll()
{
    if (input_view_ == input_view::multiline)
    {
        const auto border_width = theme_data_.border_width / 2;

        vert_scroll->set_position({ position_.right - border_width - scroll::full_scrollbar_size,
            position_.top + theme_data_.item_indent,
            position_.right - border_width,
            position_.bottom - scroll::tiny_scrollbar_size - theme_data_.item_indent });

        hor_scroll->set_position({ position_.left + theme_data_.item_indent,
            position_.bottom - border_width - scroll::full_scrollbar_size,
            position_.right - theme_data_.item_indent,
            position_.bottom - border_width
            });
        update_scroll_areas();
        scroll_to_cursor();
    }
}

void input::set_position(const rect& position__)
{
    position_ = position__;
    if (input_view_ == input_view::multiline)
    {
        if (position_.is_hide())
        {
            hor_scroll->hide();
            vert_scroll->hide();
            return;
        }

        update_scroll();
    }
}

rect input::position() const
{
    return get_control_position(position_, parent_);
}

void input::set_parent(std::shared_ptr<window> window_)
{
    parent_ = window_;
    my_control_sid = window_->subscribe(std::bind(&input::receive_control_events, this, std::placeholders::_1),
        wui::event_type::internal | wui::event_type::mouse | wui::event_type::keyboard,
        shared_from_this());
    my_plain_sid = window_->subscribe(std::bind(&input::receive_plain_events, this, std::placeholders::_1), event_type::mouse);
    window_->add_control(menu_, {});

    if (input_view_ == input_view::multiline)
    {
        window_->add_control(vert_scroll, {});
        window_->add_control(hor_scroll, {});
    }

    /// Create memory dc for inner content
    if (mem_gr)
    {
        mem_gr.reset();
    }

    auto parent__ = parent_.lock();
    if (parent__)
    {
        mem_gr = std::make_unique<graphic>(parent__->context());
    }
}

std::weak_ptr<window> input::parent() const
{
    return parent_;
}

void input::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(menu_);
        if (input_view_ == input_view::multiline)
        {
            parent__->remove_control(vert_scroll);
            parent__->remove_control(hor_scroll);
        }
        parent__->unsubscribe(my_control_sid);
        parent__->unsubscribe(my_plain_sid);
    }
    parent_.reset();

    mem_gr.reset();
}

void input::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool input::topmost() const
{
    return topmost_;
}

bool input::focused() const
{
    return focused_;
}

bool input::focusing() const
{
    return enabled_ && showed_;
}

input_view input::get_input_view() const
{
    return input_view_;
}

// focus_mode focus_mode__
focus_mode input::check_focus_mode() const
{
    switch (input_view_)
    {
        case input_view::singleline:
            return focus_mode::free;
        case input_view::multiline:
            return focus_mode::fixed;
        default:
            break;
    }
    return focus_mode::default_;
}

focus_mode input::get_focus_mode() const
{
    return focus_mode_;
}

error input::get_error() const
{
    return {};
}

void input::update_theme_data()
{
    theme_data_.background = theme_color(tcn, tv_background, theme_);
    theme_data_.text = theme_color(tcn, tv_text, theme_);
    theme_data_.selection = theme_color(tcn, tv_selection, theme_);
    theme_data_.cursor = theme_color(tcn, tv_cursor, theme_);
    theme_data_.border = theme_color(tcn, tv_border, theme_);
    theme_data_.hover_border = theme_color(tcn, tv_hover_border, theme_);
    theme_data_.focused_border = theme_color(tcn, tv_focused_border, theme_);

    theme_data_.border_width = theme_dimension(tcn, tv_border_width, theme_);
    theme_data_.round = theme_dimension(tcn, tv_round, theme_);

    if (theme_data_.round
        && (theme_data_.item_indent < theme_data_.round - theme_data_.border_width - 1))
    {
        theme_data_.item_indent = theme_data_.round - theme_data_.border_width - 1;
    }

    theme_data_.font_ = std::move(get_font());
}

void input::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void input::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }

    theme_ = theme__;

    const auto border_width_old = theme_data_.border_width;
    const auto font_size_old = theme_data_.font_.size;
    const auto item_indent_old = theme_data_.item_indent;
    update_theme_data();

    if (mem_gr)
    {
        mem_gr->set_background_color(theme_data_.background);
    }

    if (input_view_ == input_view::multiline
        && (font_size_old != theme_data_.font_.size
        || border_width_old != theme_data_.border_width
        || item_indent_old != theme_data_.item_indent
        ) && !position_.is_hide())
    {
        update_scroll();
    }
}

void input::show()
{
    showed_ = true;
    update_scroll_visibility();
    redraw();
}

void input::hide()
{
    showed_ = false;
    hor_scroll->hide();
    vert_scroll->hide();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto pos = position();
        pos.widen(theme_data_.border_width);
        parent__->redraw(pos, true);
    }
}

bool input::showed() const
{
    return showed_;
}

void input::enable()
{
    enabled_ = true;
    redraw();
}

void input::disable()
{
    enabled_ = false;
    redraw();
}

bool input::enabled() const
{
    return enabled_;
}

void input::set_input_view(input_view input_view__)
{
    input_view_ = input_view__;
    theme_data_.font_ = std::move(get_font()); // update
    reset_state();
}

void input::set_input_content(input_content input_content__)
{
    input_content_ = input_content__;
}

void input::set_symbols_limit(int32_t symbols_limit_)
{
    symbols_limit = symbols_limit_;
}

void input::set_change_callback(std::function<void()> change_callback_)
{
    change_callback = change_callback_;
}

void input::set_return_callback(std::function<void()> return_callback_)
{
    return_callback = return_callback_;
}

const std::vector<std::string>& input::get_lines() const
{
    return lines_;
}

void input::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_data_.border_width);
            parent__->redraw(pos);
        }
    }
}

void input::redraw_cursor()
{
    cursor_visible = !cursor_visible;
    redraw();
}

void input::update_lines(std::string_view text)
{
    lines_.clear();
    if (text.empty())
    {
        lines_.push_back("");
        cursor_row = 0;
        cursor_col = 0;
        invalidate_max_width_cache();
        return;
    }
    std::istringstream iss;
    iss.str(text.data());
    std::string line;
    while (std::getline(iss, line))
    {
        lines_.push_back(line);
    }
    if (lines_.empty())
    {
        lines_.push_back("");
    }

    cursor_row = 0;
    cursor_col = 0;
    invalidate_max_width_cache();
    if (!position_.is_hide())
    {
        update_scroll_areas();
    }
}

// NB: use only calculate max symb
std::string input::text() const
{
    std::ostringstream oss;
    auto size = lines_.size();
    for (size_t i = 0; i < size; ++i)
    {
        oss << lines_[i];
        if (i + 1 < size) oss << '\n';
    }
    return oss.str();
}

void input::reset_state()
{
    cursor_row = cursor_col = 0;
    select_start_row = select_start_col = select_end_row = select_end_col = 0;
}

// Deleting selected text in multiline
bool input::clear_selected_text()
{
    if (select_start_row == select_end_row && select_start_col == select_end_col)
        return false;
    size_t srow = select_start_row, scol = select_start_col,
        erow = select_end_row, ecol = select_end_col;
    if (cursor_less(erow, ecol, srow, scol))
    {
        std::swap(srow, erow);
        std::swap(scol, ecol);
    }
    if (srow == erow)
    {
        // We get byte positions for correct deletion of UTF-8 characters
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        size_t end_byte = get_byte_pos_for_char_pos(lines_[srow], ecol);

        lines_[srow].erase(start_byte, end_byte - start_byte);

        cursor_row = srow;
        cursor_col = scol;
    }
    else
    {
        // The first line
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        lines_[srow].erase(start_byte);

        // The last line
        size_t end_byte = get_byte_pos_for_char_pos(lines_[erow], ecol);
        lines_[erow].erase(0, end_byte);

        // Combining the lines
        lines_[srow] += lines_[erow];
        lines_.erase(lines_.begin() + srow + 1, lines_.begin() + erow + 1);
        cursor_row = srow;
        cursor_col = scol;
        invalidate_max_width_cache();
    }
    select_start_row = select_start_col = select_end_row = select_end_col = 0;
    selecting = false;
    return true;
}

// Auxiliary functions for multiline
void input::select_all()
{
    if (lines_.empty())
        return;
    select_start_row = 0;
    select_start_col = 0;
    select_end_row = lines_.size() - 1;
    select_end_col = utf8::distance(lines_[select_end_row].begin(), lines_[select_end_row].end());
    scroll_to_cursor();
    redraw();
}

void input::select_current_word(int x, int y)
{
    auto [row, col] = calculate_mouse_cursor_position(x, y);
    cursor_row = row;
    cursor_col = col;

    select_start_row = select_end_row = row;
    select_start_col = select_end_col = col;

    const auto& line = lines_[row];
    const auto line_size = line.size();

    // We are looking for the beginning of a word (using character positions)
    while (select_start_col > 0)
    {
        const auto prev_char_pos = select_start_col - 1;
        const auto prev_byte_pos = get_byte_pos_for_char_pos(line, prev_char_pos);
        if (prev_byte_pos < line_size
            && (
                std::isspace(static_cast<int>(static_cast<unsigned char>(line[prev_byte_pos])))
                || std::ispunct(static_cast<int>(static_cast<unsigned char>(line[prev_byte_pos])))
                )
            )
        {
            break;
        }
        select_start_col = prev_char_pos;
    }

    const size_t actual_chars = utf8::distance(line.begin(), line.end());
    // Looking for the end of a word (using character positions)
    while (select_end_col < actual_chars)
    {
        const auto next_byte_pos = get_byte_pos_for_char_pos(line, select_end_col);
        if (next_byte_pos < line_size
            && (
                std::isspace(static_cast<int>(static_cast<unsigned char>(line[next_byte_pos])))
                || std::ispunct(static_cast<int>(static_cast<unsigned char>(line[next_byte_pos])))
                )
            )
        {
            break;
        }
        ++select_end_col;
    }

    selecting = false;
    scroll_to_cursor();
    redraw();
}

// Clipboard functions for multiline
void input::buffer_copy()
{
    if ((select_start_row == select_end_row && select_start_col == select_end_col)
        || input_view_ == input_view::password)
    {
        return;
    }

    auto parent__ = parent_.lock();
    if (!parent__)
    {
        return;
    }

    size_t srow = select_start_row, scol = select_start_col,
        erow = select_end_row, ecol = select_end_col;
    if (cursor_less(erow, ecol, srow, scol))
    {
        std::swap(srow, erow);
        std::swap(scol, ecol);
    }

    std::ostringstream oss;
    if (srow == erow)
    {
        // We get byte positions for correct copying of UTF-8 characters
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        size_t end_byte = get_byte_pos_for_char_pos(lines_[srow], ecol);
        oss << lines_[srow].substr(start_byte, end_byte - start_byte);
    }
    else
    {
        // The first line
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        oss << lines_[srow].substr(start_byte);

        // Middle lines
        for (size_t i = srow + 1; i < erow; ++i)
        {
            oss << '\n' << lines_[i];
        }

        // The last line
        if (erow > srow)
        {
            size_t end_byte = get_byte_pos_for_char_pos(lines_[erow], ecol);
            oss << '\n' << lines_[erow].substr(0, end_byte);
        }
    }

    clipboard_put(oss.str(), parent__->context());
}

void input::buffer_cut()
{
    if ((select_start_row == select_end_row && select_start_col == select_end_col)
        || input_view_ == input_view::readonly)
    {
        return;
    }

    buffer_copy();
    clear_selected_text();

    if (change_callback)
        change_callback();
    redraw();
}

void input::buffer_paste()
{
    auto parent__ = parent_.lock();
    if (!parent__)
    {
        return;
    }

    if (input_view_ == input_view::readonly
        || !is_text_in_clipboard(parent__->context()))
    {
        return;
    }

    // We check that cursor_row does not exceed the boundaries
    if (lines_.empty())
    {
        lines_.push_back("");
        cursor_row = 0;
        cursor_col = 0;
    }
    else if (cursor_row >= lines_.size())
    {
        cursor_row = lines_.size() - 1;
        cursor_col = lines_[cursor_row].size();
    }

    clear_selected_text();

    auto paste_string = clipboard_get_text(parent__->context());

    // Splitting the inserted text into lines
    std::istringstream iss(paste_string);
    std::vector<std::string> paste_lines;
    std::string line;
    while (std::getline(iss, line))
    {
        paste_lines.push_back(line);
    }
    if (paste_lines.empty()) paste_lines.push_back("");

    // Checking the character limit
    size_t total_chars = 0;
    for (const auto& l : lines_) total_chars += utf8::distance(l.begin(), l.end());
    for (const auto& l : paste_lines) total_chars += utf8::distance(l.begin(), l.end());

    if (symbols_limit != -1 && total_chars > (size_t)symbols_limit)
    {
        return; // We do not insert it if the limit is exceeded.
    }

    if (paste_lines.size() == 1)
    {
        size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
        lines_[cursor_row].insert(insert_byte, paste_lines[0]);
        cursor_col += utf8::distance(paste_lines[0].begin(), paste_lines[0].end());
    }
    else
    {
        // We insert several lines
        size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
        std::string tail = lines_[cursor_row].substr(insert_byte);
        lines_[cursor_row].erase(insert_byte);
        lines_[cursor_row] += paste_lines[0];
        std::vector<std::string> new_lines;
        new_lines.reserve(paste_lines.size() - 1);
        for (size_t i = 1; i < paste_lines.size() - 1; ++i)
        {
            new_lines.push_back(paste_lines[i]);
        }
        new_lines.push_back(paste_lines.back() + tail);
        lines_.insert(lines_.begin() + cursor_row + 1, new_lines.begin(), new_lines.end());
        cursor_row += paste_lines.size() - 1;
        // Cursor at the end of the inserted block
        cursor_col = utf8::distance(paste_lines.back().begin(), paste_lines.back().end());
        invalidate_max_width_cache();
    }

    update_scroll_areas();
    scroll_to_cursor();
    if (change_callback) change_callback();
    redraw();
}

// Methods for working with scrolling
void input::update_scroll_areas()
{
    const auto control_pos = position();
    //const int32_t line_height = theme_data_.font_.size;
    const int line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space

    // Calculate the maximum line width (using cache)
    const int32_t max_width = get_max_line_width();

    // Calculate the size of the text area
    const int32_t content_width = control_pos.width() - theme_data_.border_width * 2 - (input_view_ == input_view::multiline ?
        scroll::full_scrollbar_size : 0);
    const int32_t content_height = control_pos.height() - theme_data_.border_width * 2 - (input_view_ == input_view::multiline ?
        scroll::full_scrollbar_size : 0);

    // Count the vertical scrollbar
    const int32_t total_height = static_cast<int32_t>(lines_.size()) * line_height;
    const int32_t vert_area = std::max(0, total_height - content_height);
    vert_scroll->set_area(vert_area);

    // Count the horizontal scrollbar
    const int32_t hor_area = std::max(0, max_width - content_width);
    hor_scroll->set_area(hor_area);

    update_scroll_visibility();
}

void input::on_vert_scroll(scroll_state ss, int32_t v)
{
    if (ss == scroll_state::up_end || ss == scroll_state::down_end || ss == scroll_state::moving)
    {
        scroll_offset_y = v;
        redraw();
    }
}

void input::on_hor_scroll(scroll_state ss, int32_t v)
{
    if (ss == scroll_state::up_end || ss == scroll_state::down_end || ss == scroll_state::moving)
    {
        scroll_offset_x = v;
        redraw();
    }
}

void input::update_scroll_visibility()
{
    if (input_view_ != input_view::multiline)
    {
        return;
    }

    const auto control_pos = position();

    //const int32_t line_height = theme_data_.font_.size;
    const int line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space
    const int32_t total_height = static_cast<int>(lines_.size()) * line_height;
    const int32_t content_height = control_pos.height()
        - theme_data_.border_width * 2 - scroll::full_scrollbar_size;

    // Calculating the maximum row width (using cache)
    const int32_t max_width = get_max_line_width();
    const int32_t content_width = control_pos.width() - theme_data_.border_width * 2;

    // Showing/hiding the vertical scroll
    const bool need_vert_scroll = total_height > content_height && lines_.size() > 1;
    if (need_vert_scroll != vert_scroll->showed())
    {
        if (need_vert_scroll)
        {
            vert_scroll->show();
        }
        else
        {
            vert_scroll->hide();
        }
    }

    // Showing/hiding the horizontal scroll
    const bool need_hor_scroll = max_width > content_width;
    if (need_hor_scroll != hor_scroll->showed())
    {
        if (need_hor_scroll)
        {
            hor_scroll->show();
        }
        else
        {
            hor_scroll->hide();
        }
    }
}

// Cache management for performance
int input::get_max_line_width()
{
    if (!max_width_dirty_ && cached_max_width_ >= 0)
    {
        return cached_max_width_;
    }

    if (!graphic::text_measurer_inited())
    {
        return 0; // если текст задан до создания первого окна
    }

    int32_t max_width = 0;

    for (const auto& line : lines_)
    {
        const auto text_width = get_text_width(line, theme_data_.font_);
        max_width = std::max(max_width, text_width);
    }

    cached_max_width_ = max_width;// +horizontal_indent * 2;
    max_width_dirty_ = false;

    return cached_max_width_;
}

void input::invalidate_max_width_cache()
{
    max_width_dirty_ = true;
    cached_max_width_ = -1;
}

// Auto-scroll functions for mouse selection
void input::start_auto_scroll(bool up)
{
    if (auto_scroll_timer_ && auto_scroll_type_ == auto_scroll_type::idle)
    {
        auto_scroll_type_ = up ? auto_scroll_type::up : auto_scroll_type::down;
        auto_scroll_timer_->start(80); // 80ms interval (12.5 lines per sec)
    }
}

void input::start_auto_hscroll(bool left)
{
    if (auto_scroll_timer_ && auto_scroll_type_ == auto_scroll_type::idle)
    {
        auto_scroll_type_ = left ? auto_scroll_type::left : auto_scroll_type::right;
        auto_scroll_timer_->start(80); // 80ms interval (12.5 symbols per sec)
    }
}

void input::stop_auto_scroll()
{
    if (auto_scroll_timer_ && auto_scroll_type_ != auto_scroll_type::idle)
    {
        auto_scroll_type_ = auto_scroll_type::idle;
        auto_scroll_timer_->stop();
    }
}

void input::on_auto_scroll()
{
    if (auto_scroll_type_ != auto_scroll_type::idle && !selecting)
    {
        auto_scroll_type_ = auto_scroll_type::idle;
        return;
    }

    switch (auto_scroll_type_)
    {
        case auto_scroll_type::up:
            if (cursor_row > 0)
            {
                --cursor_row;
                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else
            {
                // If it is already on the first line, move the cursor to the beginning of the line
                cursor_col = 0;
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
                auto_scroll_type_ = auto_scroll_type::idle;
            }
            break;
        case auto_scroll_type::down:
            if (cursor_row + 1 < lines_.size())
            {
                ++cursor_row;
                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else
            {
                // If it is already on the last line, move the cursor to the end of the line
                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
                auto_scroll_type_ = auto_scroll_type::idle;
            }
            break;
        case auto_scroll_type::left:
            if (cursor_col > 0)
            {
                --cursor_col;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else
            {
                // If you are already in the first position, move the cursor up
                if (cursor_row > 0)
                {
                    --cursor_row;
                    cursor_col = 0;
                    select_end_row = cursor_row;
                    select_end_col = cursor_col;
                    scroll_to_cursor();
                    redraw();
                }
                auto_scroll_type_ = auto_scroll_type::idle;
            }
            break;
        case auto_scroll_type::right:
            if (cursor_col + 1 < lines_[cursor_row].size())
            {
                ++cursor_col;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else
            {
                // If you are already in the last position, move the cursor down
                if (cursor_row + 1 < lines_.size())
                {
                    ++cursor_row;
                    cursor_col = 0;
                    select_end_row = cursor_row;
                    select_end_col = cursor_col;
                    scroll_to_cursor();
                    redraw();
                }
                auto_scroll_type_ = auto_scroll_type::idle;
            }
            break;
    }
}

void input::scroll_to_cursor()
{
    if (lines_.empty() || cursor_row >= lines_.size())
    {
        return;
    }

    if (input_view_ != input_view::multiline)
    {
        cursor_row = 0;
    }

    const auto& line = lines_[cursor_row];

    const auto control_pos = position();
    const int line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space

    // We take into account the place for scrollbars
    const bool show_vert_scroll = vert_scroll->showed();
    const bool show_hor_scroll = hor_scroll->showed();

    const int32_t content_height = control_pos.height() - theme_data_.border_width * 2
        - (show_hor_scroll ? scroll::full_scrollbar_size : 0);
    const int32_t content_width = control_pos.width() - theme_data_.border_width * 2
        - (show_vert_scroll ? scroll::full_scrollbar_size : 0);

    const int32_t visible_left = scroll_offset_x;
    const int32_t visible_right = visible_left + content_width - 1;
    const int32_t cursor_extra = 8;

    // Calculating the cursor position in pixels
    int32_t cursor_y = static_cast<int32_t>(cursor_row) * line_height;
    int32_t cursor_x = 0;
    // Calculating the horizontal cursor position
    size_t max_col = line.empty() ? 0 : utf8::distance(line.begin(), line.end());
    size_t safe_cursor_col = std::min(cursor_col, max_col);
    size_t cursor_byte = line.empty() ? 0 : get_byte_pos_for_char_pos(line, safe_cursor_col);

    cursor_x = measure_text(line.substr(0, cursor_byte), theme_data_.font_).right;
    const int32_t line_width = measure_text(line, theme_data_.font_).right;

    if (safe_cursor_col == max_col)
    {
        const int32_t new_scroll = std::max(0, line_width + cursor_extra - content_width);
        hor_scroll->set_scroll_pos(new_scroll);
    }
    else
    {
        if (cursor_x < visible_left)
        {
            hor_scroll->set_scroll_pos(cursor_x);
        }
        else if (cursor_x + cursor_extra > visible_right)
        {
            hor_scroll->set_scroll_pos(cursor_x - content_width + cursor_extra);
        }
    }

    // Checking if you need to scroll vertically.
    if (input_view_ == input_view::multiline)
    {
        const int32_t visible_top = scroll_offset_y;
        const int32_t visible_bottom = visible_top + content_height;
        const int32_t total_height = static_cast<int>(lines_.size()) * line_height;
        const int32_t max_scroll = std::max(0, total_height - content_height);

        if (cursor_row == lines_.size() - 1)
        {
            vert_scroll->set_scroll_pos(max_scroll);
        }
        else if (cursor_y < visible_top)
        {
            vert_scroll->set_scroll_pos(cursor_y);
        }
        else if (cursor_y + line_height > visible_bottom)
        {
            int32_t new_scroll = cursor_y + line_height - content_height;
            if (new_scroll < 0) new_scroll = 0;
            if (new_scroll > max_scroll) new_scroll = max_scroll;
            vert_scroll->set_scroll_pos(new_scroll);
        }
    }
}

void input::scroll_to_end()
{
    if (lines_.empty() || input_view_ != input_view::multiline)
    {
        return;
    }

 //   const int32_t line_height = theme_data_.font_.size;
    const int32_t line_height = get_font_ideal_height(theme_data_.font_, mem_gr.get()); // line space

    vert_scroll->set_scroll_pos(line_height * static_cast<int32_t>(lines_.size()));
}

}
