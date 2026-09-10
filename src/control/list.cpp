//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/list.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <iostream>

namespace wui
{

list::list(std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    showed_(true), enabled_(true), focused_(false),
    mouse_on_control(false), mouse_on_slider(false),
    mode(list_mode::simple),
    item_count(0), selected_item_(-1), active_item_(-1), // TODO: int64_t
    title_height(0),
    scroll_area(0),
    vert_scroll(std::make_shared<scroll>(0, 0, orientation::vertical,
        std::bind(&list::on_scroll, this, std::placeholders::_1,
            std::placeholders::_2), scroll::tc, theme__))
{
    update_theme();
}

list::~list()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

int32_t list::get_font_size() const
{
    return theme_font(tcn, tv_font, theme_).size;
}

bool list::update_mem_gr()
{
    if (!mem_gr)
    {
        return false;
    }

    auto width = position_.width() - 2 * (theme_data_.border_width + theme_data_.item_indent);
    auto height = position_.height() - title_height - 2 * theme_data_.border_width
        - theme_data_.item_indent;

    auto current = mem_gr->max_size();

    if (current.width() < width || current.height() < height)
    {
        mem_gr->release();
        return mem_gr->init({ 0, 0, width, height }, theme_data_.background);
    }

    mem_gr->clear();
    return true;
}

void list::draw(graphic &gr, const rect&)
{
    if (!showed_ || position_.is_hide() || !update_mem_gr())
    {
        return;
    }

    const auto control_pos = position();
    if (theme_data_.round)
    {
        /// Draw the frame (gr)
        gr.draw_rect(control_pos, make_color(0, 0, 0, 0),
            theme_data_.background, 0, theme_data_.round);
    }

    draw_items(*mem_gr);
    if (title_height)
    {
        /// Draw the titles (gr)
        draw_titles(gr);
    }

    // Copying the offscreen buffer to the parent context
    gr.copy_area(control_pos, *mem_gr,
        -theme_data_.border_width - theme_data_.item_indent ,
        -title_height - theme_data_.border_width);

    if ((mouse_on_control || focused_) && has_scrollbar())
    {
        vert_scroll->draw(gr, {});
    }

    if (theme_data_.border_width)
    {
        const auto border_color = focused_ ? theme_data_.focused_border :
            (!mouse_on_control ? theme_data_.border : theme_data_.hover_border);
        gr.draw_rect(control_pos, border_color, make_color(0, 0, 0, 0),
            theme_data_.border_width, theme_data_.round);
    }
}

void list::receive_control_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (ev.type & event_type::mouse)
    {
        if (has_scrollbar())
        {
            if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))
            {
                event sev = ev;
                if (!mouse_on_slider)
                {
                    mouse_on_slider = true;

                    sev.mouse_event_.type = wui::mouse_event_type::enter;
                    vert_scroll->receive_control_events(sev);

                    return;
                }
                vert_scroll->receive_control_events(sev);
                return;
            }
            else
            {
                if (mouse_on_slider)
                {
                    mouse_on_slider = false;

                    event sev = ev;
                    sev.mouse_event_.type = wui::mouse_event_type::leave;
                    vert_scroll->receive_control_events(sev);

                    redraw();
                }
            }
        }

        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
                if (has_scrollbar())
                {
                    vert_scroll->show();
                }
                mouse_on_control = true;
                redraw();
            break;
            case mouse_event_type::leave:
                mouse_on_control = false;
                mouse_on_slider = false;
                active_item_ = -1;
                redraw();
            break;
            case mouse_event_type::left_down:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    int32_t pos = 0, n = 0;
                    for (auto &c : columns_)
                    {
                        if (ev.mouse_event_.x >= pos && ev.mouse_event_.x < pos + c.width)
                        {
                            if (column_click_callback)
                            {
                                column_click_callback(n);
                            }
                            break;
                        }

                        pos += c.width;
                        ++n;
                    }
                    return;
                }
            break;
            case mouse_event_type::left_up:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    return;
                }
                update_selected_item(ev.mouse_event_.y);

                if (item_click_callback)
                {
                    item_click_callback(click_button::left, selected_item_, ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::right_up:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    return;
                }

                update_selected_item(ev.mouse_event_.y);

                if (item_click_callback)
                {
                    item_click_callback(click_button::right, selected_item_, ev.mouse_event_.x, ev.mouse_event_.y);
                }
            break;
            case mouse_event_type::left_double:
                if (ev.mouse_event_.y - position().top <= title_height)
                {
                    return;
                }

                update_selected_item(ev.mouse_event_.y);

                if (selected_item_ != -1 && item_activate_callback)
                {
                    item_activate_callback(selected_item_);
                }
            break;
            case mouse_event_type::move:
            {
                if (mode == list_mode::simple || mode == list_mode::simple_topmost)
                {
                    if(!has_scrollbar()
                        || vert_scroll->get_scroll_view() != scroll_view::full)
                        update_active_item(ev.mouse_event_.y);
                }
                else if (mode == list_mode::auto_select)
                {
                    update_selected_item(ev.mouse_event_.y);
                }
                set_cursor(parent_, cursor::default_);
            }
            break;
            case mouse_event_type::wheel:
                if (mode == list_mode::simple || mode == list_mode::simple_topmost)
                {
                    update_active_item(ev.mouse_event_.y);
                }
                else if (mode == list_mode::auto_select)
                {
                    update_selected_item(ev.mouse_event_.y);
                }

                if (ev.mouse_event_.wheel_delta > 0)
                {
                    vert_scroll->scroll_up();
                }
                else
                {
                    vert_scroll->scroll_down();
                }
            break;
            default:
            break;
        }
    }
    else if (ev.type & event_type::keyboard)
    {
        switch (ev.keyboard_event_.type)
        {
            case keyboard_event_type::down:
                switch (ev.keyboard_event_.key[0])
                {
                    case vk_esc:
                        if (item_count > 0 && active_item_ >= 0)
                        {
                            active_item_ = -1;
                            redraw();
                        }
                        break;
                    case vk_home: case vk_nhome:
                        if (mode != list_mode::auto_select && !keyboard_auto_select)
                        {
                            if (item_count > 0 && active_item_ != 0)
                            {
                                vert_scroll->set_scroll_pos(0);

                                active_item_ = 0;
                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ != 0)
                            {
                                vert_scroll->set_scroll_pos(0);

                                selected_item_ = 0;
                                active_item_ = -1;
                                redraw();

                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                    case vk_end: case vk_nend:
                        if (mode != list_mode::auto_select && !keyboard_auto_select)
                        {
                            if (item_count > 0 && active_item_ != item_count - 1)
                            {
                                vert_scroll->set_scroll_pos(scroll_area);

                                active_item_ = item_count - 1;
                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ != item_count - 1)
                            {
                                vert_scroll->set_scroll_pos(scroll_area);

                                selected_item_ = item_count - 1;
                                active_item_ = -1;
                                redraw();
                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                    case vk_up: case vk_nup:
                        if (mode != list_mode::auto_select
                            && ((!keyboard_auto_select && vk_alt != ev.keyboard_event_.modifier)
                                ||(keyboard_auto_select && vk_alt == ev.keyboard_event_.modifier))
                            )
                        {
                            if (item_count > 0 &&
                                (active_item_ > 0 || active_item_ < 0))
                            {
                                if (active_item_ < 0)
                                {
                                    active_item_ = selected_item_ > 0 ?
                                        selected_item_ : 1;
                                }

                                --active_item_;

                                auto selected_item_top = get_item_top(active_item_);
                                if (selected_item_top < vert_scroll->get_scroll_pos())
                                {
                                    vert_scroll->set_scroll_pos(active_item_);
                                }

                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ > 0)
                            {
                                --selected_item_;

                                auto selected_item_top = get_item_top(selected_item_);
                                if (selected_item_top < vert_scroll->get_scroll_pos())
                                {
                                    vert_scroll->set_scroll_pos(selected_item_top);
                                }

                                active_item_ = -1;
                                redraw();

                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                    case vk_down: case vk_ndown:
                        if (mode != list_mode::auto_select
                            && ((!keyboard_auto_select && vk_alt != ev.keyboard_event_.modifier)
                                || (keyboard_auto_select && vk_alt == ev.keyboard_event_.modifier))
                            )
                        {
                            if (item_count > 0 && active_item_ < item_count - 1)
                            {
                                if (active_item_ < 0)
                                {
                                    active_item_ = (selected_item_ >= 0 ?
                                        (selected_item_ < item_count - 1 ?
                                            selected_item_ : item_count - 2): 0);
                                }

                                ++active_item_;

                                auto selected_item_bottom = get_item_top(active_item_)
                                    + get_item_height(active_item_) + title_height;
                                if (selected_item_bottom > position_.height())
                                {
                                    vert_scroll->set_scroll_pos(selected_item_bottom - position_.height());
                                }

                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ < item_count - 1)
                            {
                                ++selected_item_;

                                auto selected_item_bottom = get_item_top(selected_item_)
                                    + get_item_height(selected_item_) + title_height;
                                if (selected_item_bottom > position_.height())
                                {
                                    vert_scroll->set_scroll_pos(selected_item_bottom - position_.height());
                                }

                                active_item_ = -1;
                                redraw();

                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                    case vk_page_up: case vk_npage_up:
                        if (mode != list_mode::auto_select && !keyboard_auto_select)
                        {
                            if (item_count > 0 && active_item_ != 0)
                            {
                                constexpr int32_t diff_scroll = 10;

                                if (active_item_ < 0)
                                {
                                    active_item_ = selected_item_ > 0 ?
                                        selected_item_ : 1;
                                }

                                if (active_item_ > diff_scroll)
                                {
                                    active_item_ -= diff_scroll;
                                }
                                else
                                {
                                    active_item_ = 0;
                                }

                                vert_scroll->set_scroll_pos(get_item_top(active_item_));

                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ != 0)
                            {
                                constexpr int32_t diff_scroll = 10;

                                if (selected_item_ > diff_scroll)
                                {
                                    selected_item_ -= diff_scroll;
                                }
                                else
                                {
                                    selected_item_ = 0;
                                }

                                vert_scroll->set_scroll_pos(get_item_top(selected_item_));

                                active_item_ = -1;
                                redraw();

                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                    case vk_page_down: case vk_npage_down:
                        if (mode != list_mode::auto_select && !keyboard_auto_select)
                        {
                            if (item_count > 0 && active_item_ < item_count - 1)
                            {
                                int32_t diff_scroll = 10;
                                if (diff_scroll > item_count)
                                {
                                    diff_scroll = item_count;
                                }

                                if (active_item_ < 0)
                                {
                                    active_item_ = selected_item_ > 0 ?
                                        selected_item_ : 0;
                                }

                                if (active_item_ < item_count - diff_scroll)
                                {
                                    active_item_ += diff_scroll;
                                }
                                else
                                {
                                    active_item_ = item_count - 1;
                                }

                                vert_scroll->set_scroll_pos(get_item_top(active_item_)
                                    + get_item_height(active_item_) - position_.height() + title_height);

                                redraw();
                            }
                        }
                        else
                        {
                            if (item_count > 0 && selected_item_ < item_count - 1)
                            {
                                int32_t diff_scroll = 10;
                                if (diff_scroll > item_count)
                                {
                                    diff_scroll = item_count;
                                }

                                if (selected_item_ < 0)
                                {
                                    selected_item_ = 0;
                                }

                                if (selected_item_ < item_count - diff_scroll)
                                {
                                    selected_item_ += diff_scroll;
                                }
                                else
                                {
                                    selected_item_ = item_count - 1;
                                }

                                active_item_ = -1;
                                vert_scroll->set_scroll_pos(get_item_top(selected_item_)
                                    + get_item_height(selected_item_) - position_.height() + title_height);

                                redraw();

                                if (item_change_callback)
                                {
                                    lock_changes_item_count = true;
                                    item_change_callback(selected_item_);
                                    lock_changes_item_count = false;
                                }
                            }
                        }
                    break;
                }
            break;
            case keyboard_event_type::up:
                if (ev.keyboard_event_.key[0] == vk_up || ev.keyboard_event_.key[0] == vk_down)
                {
                    //end_work();
                }
            break;
            default: break;
        }
    }
    else if (ev.type & event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;

                if (has_scrollbar())
                {
                    vert_scroll->show();
                }

                redraw();
            break;
            case internal_event_type::remove_focus:
                focused_ = false;
                active_item_ = -1;

                vert_scroll->hide();

                redraw();
            break;
            case internal_event_type::execute_focused:
                if (active_item_ >= 0 && active_item_ < item_count
                    && selected_item_ != active_item_)
                {
                    selected_item_ = active_item_;
                    redraw();

                    if (item_change_callback)
                    {
                        item_change_callback(selected_item_);
                    }
                }

                if (selected_item_ != -1 && item_activate_callback)
                {
                    item_activate_callback(selected_item_);
                }
            break;
        }
    }
}

int32_t list::check_items_height(const int32_t height)
{
    int32_t h = 0;
    for (int32_t i = 0; i < item_count; ++i)
    {
        const auto item_height = get_item_height(i);
        if (item_height != -1)
        {
            h += item_height;
            if (h >= height)
            {
                return h - item_height;
            }
        }
    }
    return height;
}

rect list::get_preferred_size(const int32_t height)
{
    int32_t w = 0;
    for (auto& cc: columns_)
    {
        w += cc.width + column_ident_;
    }
    if (w)
        w -= column_ident_;
    const auto h = check_items_height(height - title_height - theme_data_.border_width)
        + title_height + 2 * theme_data_.border_width + theme_data_.item_indent;
    return { 0, 0,
        w + 2 * (theme_data_.border_width + theme_data_.item_indent), h };
}

void list::move(const int32_t dx, const int32_t dy)
{
    position_.move(dx, dy);
    set_position(position_);
}

void list::update_scroll(const bool changed)
{
    const auto border_width = theme_data_.border_width / 2;
    vert_scroll->set_position(
        {
            position_.right - border_width - scroll::full_scrollbar_size,
            position_.top + title_height + border_width,
            position_.right - border_width,
            position_.bottom - border_width - theme_data_.item_indent
        });

    if (changed)
    {
        update_scroll_area();
    }
}

void list::set_position(const rect& position__)
{
    const bool changed = position__.height() != position_.height();
    position_ = position__;
    update_scroll(changed);
}

rect list::position() const
{
    return get_control_position(position_, parent_);
}

void list::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;

    my_control_sid = window->subscribe(std::bind(&list::receive_control_events, this, std::placeholders::_1),
        wui::event_type::internal | wui::event_type::mouse | wui::event_type::keyboard,
        shared_from_this());

    window->add_control(vert_scroll, { 0 });

    /// Create memory dc for inner content
    if (mem_gr) mem_gr.reset();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        mem_gr = std::make_unique<graphic>(parent__->context());
    }
}

std::weak_ptr<window> list::parent() const
{
    return parent_;
}

void list::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(vert_scroll);

        parent__->unsubscribe(my_control_sid);
        my_control_sid.clear();
    }

    parent_.reset();

    mem_gr.reset();
}

void list::set_topmost(bool yes)
{
    mode = yes ? list_mode::simple_topmost : list_mode::simple;
}

bool list::topmost() const
{
    return mode == list_mode::auto_select || mode == list_mode::simple_topmost;
}

bool list::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool list::focusing() const
{
    return enabled_ && showed_;
}

error list::get_error() const
{
    return {};
}

void list::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void list::update_theme_data()
{
    theme_data_.background = theme_color(tcn, tv_background, theme_);
    theme_data_.border = theme_color(tcn, tv_border, theme_);
    theme_data_.hover_border = theme_color(tcn, tv_hover_border, theme_);
    theme_data_.focused_border = theme_color(tcn, tv_focused_border, theme_);
    theme_data_.title = theme_color(tcn, tv_title, theme_);
    theme_data_.title_column = theme_color(tcn, tv_title_column, theme_);
    theme_data_.title_text = theme_color(tcn, tv_title_text, theme_);
    theme_data_.selected_item = theme_color(tcn, tv_selected_item, theme_);
    theme_data_.active_item = theme_color(tcn, tv_active_item, theme_);

    theme_data_.border_width = theme_dimension(tcn, tv_border_width, theme_);
    theme_data_.round = theme_dimension(tcn, tv_round, theme_);

    theme_data_.border_item = theme_dimension(tcn, tv_border_item, theme_);

    theme_data_.item_indent = theme_dimension(tcn, tv_item_indent, theme_);

    if (theme_data_.round
        && (theme_data_.item_indent < theme_data_.round - theme_data_.border_width - 1))
    {
        theme_data_.item_indent = theme_data_.round - theme_data_.border_width - 1;
    }

    theme_data_.font_ = std::move(theme_font(tcn, tv_font, theme_));
}

void list::update_theme(std::shared_ptr<i_theme> theme__)
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

    const auto title_height_old = title_height;
    calc_title_height();

    if (mem_gr)
    {
        mem_gr->set_background_color(theme_data_.background);
    }

    if ((title_height_old != title_height
        || border_width_old != theme_data_.border_width
        || font_size_old != theme_data_.font_.size
        || item_indent_old != theme_data_.item_indent
        )
        && !position_.is_hide())
    {
        update_scroll(true);
    }
}

void list::show()
{
    if (!showed_)
    {
        showed_ = true;
        if (has_scrollbar())
        {
            vert_scroll->show();
        }
        redraw();
    }
}

void list::hide()
{
    if (showed_)
    {
        showed_ = false;

        vert_scroll->hide();

        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_data_.border_width);
            parent__->redraw(pos, true);
        }
    }
}

bool list::showed() const
{
    return showed_;
}

void list::enable()
{
    if (!enabled_)
    {
        enabled_ = true;
        redraw();
    }
}

void list::disable()
{
    if (enabled_)
    {
        enabled_ = false;
        redraw();
    }
}

bool list::enabled() const
{
    return enabled_;
}

void list::update_columns(const std::vector<column> &columns__)
{
    columns_ = columns__;
    calc_title_height();
    if (!position_.is_hide())
    {
        update_scroll(true); //при добавлении columns после set_position,
        redraw();
    }
}

const std::vector<list::column> &list::columns()
{
    return columns_;
}

void list::set_mode(list_mode mode_) noexcept
{
    mode = mode_;
}

// TODO: int64_t
void list::select_item(int32_t n_item)
{
    if (n_item < 0 || n_item >= item_count)
    {
        return;
    }

    selected_item_ = n_item;

    redraw();

    if (item_change_callback)
    {
        item_change_callback(selected_item_);
    }
}

// TODO: int64_t n_column
void list::set_column_width(int32_t n_column, int32_t width)
{
    if (n_column >= static_cast<int32_t>(columns_.size()))
    {
        columns_[n_column].width = width;
        redraw();
    }
}

int32_t list::get_item_height(int32_t nItem) const
{
    int32_t height = -1;
    if (item_height_callback && nItem >= 0)
    {
        item_height_callback(nItem, height);
    }
    return height <= 0 ? -1 :
        (height < theme_data_.font_.size + 2 * theme_data_.border_item
            ? theme_data_.font_.size + 2 * theme_data_.border_item : height);
}

void list::set_item_count(int32_t count)
{
    if (count < 0 || lock_changes_item_count)
    {
        return;
    }

    item_count = count;

    if (-1 != selected_item_ && selected_item_ >= count)
    {
        selected_item_ = -1;
        if (item_change_callback)
        {
            item_change_callback(selected_item_);
        }
    }

    update_scroll_area();

    if (!position_.is_hide())
    {
        redraw();
    }
}

void list::make_selected_visible()
{
    const auto area = position_.height();
    if (area <= 0 || selected_item_ < 0)
    {
        return;
    }

    const auto scroll_pos = vert_scroll->get_scroll_pos();

    const auto selected_top = get_item_top(selected_item_);
    const auto selected_bottom = selected_top + get_item_height(selected_item_);

    const auto visible_top = scroll_pos;
    const auto visible_bottom = scroll_pos + area;

    if (selected_top < visible_top || selected_bottom > visible_bottom)
    {
        vert_scroll->set_scroll_pos(selected_top);
    }
}

void list::scroll_to_start()
{
    vert_scroll->set_scroll_pos(0);
    redraw();
}

void list::scroll_to_end()
{
    vert_scroll->set_scroll_pos(scroll_area);
    redraw();
}

int32_t list::get_item_top(int32_t n_item) const
{
    if (n_item < 0)
    {
        return 0;
    }

    int32_t top = 0;

    for (int32_t i = 0; i < n_item; ++i)
    {
        const auto height = get_item_height(i);
        if (height != -1)
        {
            top += height;
        }
    }

    return top;
}

void list::set_draw_callback(std::function<void(graphic&, const int32_t nItem,
    const rect&, const item_state state)> draw_callback_) noexcept
{
    draw_callback = draw_callback_;
}

void list::set_item_height_callback(std::function<void(int32_t, int32_t&)> item_height_callback_) noexcept
{
    item_height_callback = item_height_callback_;
}

void list::set_item_click_callback(std::function<void(click_button, int32_t, int32_t, int32_t)> item_click_callback_) noexcept
{
    item_click_callback = item_click_callback_;
}

void list::set_item_change_callback(std::function<void(int32_t)> item_change_callback_) noexcept
{
    item_change_callback = item_change_callback_;
}

void list::set_item_activate_callback(std::function<void(int32_t)> item_activate_callback_) noexcept
{
    item_activate_callback = item_activate_callback_;
}

void list::set_column_click_callback(std::function<void(int32_t)> column_click_callback_) noexcept
{
    column_click_callback = column_click_callback_;
}

void list::set_scroll_callback(std::function<void(scroll_state, int32_t)> scroll_callback_) noexcept
{
    scroll_callback = scroll_callback_;
}

void list::on_scroll(scroll_state ss, int32_t v)
{
    redraw();

    if (scroll_callback)
    {
        scroll_callback(ss, v);
    }
}

void list::redraw()
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

void list::redraw_item(int32_t item)
{
    if (showed_ && item >= 0)
    {
        const auto control_pos = position();

        const auto scroll_pos = vert_scroll->get_scroll_pos();

        const auto top = control_pos.top + get_item_top(item) + title_height - scroll_pos;

        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto height = get_item_height(item);
            if (height != -1)
            {
                parent__->redraw({ control_pos.left, top, control_pos.right, top + height});
            }
        }
    }
}

void list::calc_title_height()
{
    if (columns_.empty())
    {
        title_height = 0;
        return;
    }

    title_height = theme_data_.font_.size + text_indent * 2;
}

int32_t list::get_left_position_text(int32_t n_col) const noexcept
{
    if (n_col <= 0 || n_col >= columns_.size())
        return text_indent;
    int32_t width = columns_[0].width;
    for (int n = 1; n < n_col; ++n)
    {
        width += columns_[n].width;
    }
    return text_indent + column_ident_ * n_col + width;
}

void list::draw_titles(graphic &gr_)
{
    // check rect`s width
    const auto control_pos = position();
    rect rc{
        control_pos.left,
        control_pos.top + theme_data_.border_width,
        control_pos.left + position_.width(),
        control_pos.top + title_height + theme_data_.border_width
    };
    gr_.draw_rect(rc, make_color(0, 0, 0, 0), theme_data_.title,
        0, theme_data_.round);

    if (theme_data_.round)
    {
        // TODO: tmp, remove? make extended draw_rect()
        rc.top = rc.bottom - theme_data_.round;
        gr_.draw_rect(rc, make_color(0, 0, 0, 0), theme_data_.title, 0, 0);
    }

    const auto top = control_pos.top + theme_data_.border_width + 1;
    const auto bottom = control_pos.top + theme_data_.border_width
        + title_height - 1;// theme_data_.item_indent;

    const auto right = control_pos.left + position_.width()
        - theme_data_.border_width - theme_data_.item_indent;

    int32_t left = control_pos.left
        + theme_data_.border_width + theme_data_.item_indent;//

    for (size_t i = 0; i < columns_.size(); ++i)
    {
        const auto& c = columns_[i];
        auto r = left + c.width;
        if (r >= right)
        {
            r = right;
        }
        const auto w = r - left;
        if (w <= 0)
        {
            break;
        }

        // TODO: add move div
        gr_.draw_rect({ r - 1, top, r + 1, bottom },
            theme_data_.title_column);

        if (w + text_indent > 0)
        {
            gr_.draw_text({ left + text_indent, control_pos.top + text_indent, 0, 0 },
                c.caption, theme_data_.title_text, theme_data_.font_);
        }

        left += w + column_ident_;
    }
}

void list::draw_items(graphic &gr_)
{
    if (!draw_callback || position_.height() == 0)
    {
        return;
    }

    int32_t first_item = -1, item_bottom = 0;
    const auto scroll_pos = vert_scroll->get_scroll_pos(); // + title_height;
    while (scroll_pos >= item_bottom && first_item < item_count)
    {
        ++first_item;
        item_bottom = get_item_top(first_item) + get_item_height(first_item);
    }

    int32_t last_item = -1, item_top = 0;
    while (position_.height() > item_top && last_item < item_count)
    {
        ++last_item;
        item_top = get_item_top(last_item) - scroll_pos;
    }

    if (last_item < first_item || last_item == first_item)
    {
        return;
    }

    if (last_item > item_count)
    {
        last_item = item_count;
    }

    const auto control_pos = position();
    const int32_t top_ = -vert_scroll->get_scroll_pos(); // +1
    const int32_t right = position_.width()
        - 2 * (theme_data_.border_width - theme_data_.item_indent);

    for (auto item = first_item; item < last_item; ++item)
    {
        item_state state = item_state::normal;
        if (item == selected_item_)
        {
            state = item_state::selected;
        }
        else
        {
            if (item == active_item_)
            {
                state = item_state::active;
            }
        }
        const auto item_height = get_item_height(item);
        const auto top = get_item_top(item) + top_;
        const rect item_rect{ 0, top, right, top + item_height };
        draw_callback(gr_, item, item_rect, state);
    }
}

void list::update_selected_item(int32_t y)
{
    const auto scroll_pos = vert_scroll->get_scroll_pos();

    const auto pos = (y - position().top - title_height) + scroll_pos;

    int32_t item = -1, start_pos = 0, end_pos = 0;
    while (item != item_count)
    {
        if (start_pos <= pos && end_pos > pos)
        {
            break;
        }
        else
        {
            ++item;
        }
        start_pos = get_item_top(item);

        const auto height = get_item_height(item);
        end_pos = height != -1 ? start_pos + height : 0;
    }

    if (-1 != item && item != selected_item_)
    {
        const auto old_selected = selected_item_;

        selected_item_ = item < item_count ? item : -1;

        if (old_selected >= 0)
        {
            redraw_item(old_selected);
        }

        if (selected_item_ >= 0)
        {
            redraw_item(selected_item_);
        }
    }

    // если item != selected_item_ необходимо вызвать item_change_callback()
    // пример simple.cpp: диалог по кнопке 'Edit'
    if (-1 != item && selected_item_ >= 0 && item_change_callback)
    {
        item_change_callback(selected_item_);
    }
}

void list::update_active_item(int32_t y)
{
    const auto prev_active_item_ = active_item_;

    const auto scroll_pos = vert_scroll->get_scroll_pos();

    const auto pos = (y - position().top - title_height) + scroll_pos;

    active_item_ = -1;

    int32_t start_pos = 0, end_pos = 0;
    while (active_item_ != item_count)
    {
        if (start_pos <= pos && end_pos > pos)
        {
            break;
        }
        else
        {
            ++active_item_;
        }
        start_pos = get_item_top(active_item_);

        const auto height = get_item_height(active_item_);
        end_pos = height != -1 ? start_pos + height : 0;
    }

    if (prev_active_item_ != active_item_)
    {
        redraw();
    }
}

void list::update_scroll_area()
{
    scroll_area = title_height// + theme_data_.border_width
        + get_item_top(item_count)// + theme_data_.item_indent
        - position_.height();
    if (scroll_area < 0)
    {
        scroll_area = 0;
    }

    vert_scroll->set_area(scroll_area);

    if (vert_scroll->get_scroll_pos() > scroll_area)
    {
        vert_scroll->set_scroll_pos(scroll_area);
    }
}

}
