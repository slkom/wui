//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/button.hpp>

#include <wui/window/window.hpp>

#include <wui/control/image.hpp>
#include <wui/control/tooltip.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif

namespace wui
{

button::button(std::string_view caption_, std::function<void(void)> click_callback_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view::text),
    caption(caption_),
    caption_org(caption_),
    image_size(0),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    showed_(true), enabled_(true), topmost_(false), active_(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false)
{
}

static std::shared_ptr<image> get_button_image(button_view button_view_, std::shared_ptr<i_theme> theme_)
{
    switch (button_view_)
    {
        case button_view::switcher:
            return std::make_shared<image>(theme_image(button::ti_switcher_on, theme_));
        break;
        case button_view::radio:
            return std::make_shared<image>(theme_image(button::ti_radio_on, theme_));
        break;
        default:
            return nullptr;
        break;
    }
}

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    caption_org(caption_),
    image_(get_button_image(button_view__, theme__)),
    image_size(0),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    showed_(true), enabled_(true), topmost_(false), active_(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false)
{
    if (image_ && !image_->get_error().is_ok())
    {
        update_err("button::constructor[image from theme standart buttons]", image_->get_error());
    }
}

#ifdef _WIN32
button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, int32_t image_resource_index_, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    caption_org(caption_),
    image_(std::make_shared<image>(image_resource_index_, theme__)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    showed_(true), enabled_(true), topmost_(false), active_(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false)
{
    if (image_ && !image_->get_error().is_ok())
    {
        update_err("button::constructor[image from resource]", image_->get_error());
    }
}
#endif

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, std::string_view imageFileName_, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    caption_org(caption_),
    image_(std::make_shared<image>(imageFileName_, theme__)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    showed_(true), enabled_(true), topmost_(false), active_(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false)
{
    if (image_ && !image_->get_error().is_ok())
    {
        update_err("button::constructor[image from file]", image_->get_error());
    }
}

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, const std::vector<uint8_t> &image_data, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    caption_org(caption_),
    image_(std::make_shared<image>(image_data)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    showed_(true), enabled_(true), topmost_(false), active_(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false)
{
    if (image_ && !image_->get_error().is_ok())
    {
        update_err("button::constructor[image from data]", image_->get_error());
    }
}

button::~button()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

int32_t button::get_font_size() const
{
    return theme_font(tcn, tv_font, theme_).size;
}

rect button::get_preferred_size()
{
    // дает одинаковый height для нескольких кнопок
    const int32_t font_size = theme_font(tcn, tv_font, theme_).size;
    rect text_rect{}; // NB: text_rec_ не устанавливаем
    if (!caption_org.empty())
    {
        auto font_ = std::move(theme_font(tcn, tv_font, theme_));
        auto parent__ = parent_.lock();
        text_rect = measure_text(caption_org, font_, parent__ ? &parent__->get_graphic() : nullptr);
    }

    rect pref_rect{};

    switch (button_view_)
    {
        case button_view::text:
            pref_rect.right = text_rect.right + _text_width_space;
            pref_rect.bottom = font_size + _text_height_space;
            break;
        case button_view::anchor:
            pref_rect.right = text_rect.right + _ident_left + _text_width_space;
            pref_rect.bottom = font_size + _text_height_space;
            break;
        case button_view::sheet:
            pref_rect.right = text_rect.right + _text_width_space;
            pref_rect.bottom = font_size + 2 * _sheet_bottom_space;
            break;
        case button_view::image:
        case button_view::image_menu:
            if (image_)
            {
                pref_rect.right = image_size;
                pref_rect.bottom = image_size;
            }
            else
            {
                pref_rect.right = 32;
                pref_rect.bottom = 32;
            }
            break;
        case button_view::image_right_text:
        {
            const auto image_size__ = image_ ? image_size : 0;
            pref_rect.right = image_size__ + text_rect.right + _text_width_space + _text_width_space / 2;
            pref_rect.bottom = std::max(image_size__, font_size) + _text_height_space;
        }
        break;
        case button_view::image_bottom_text:
        {
            const auto image_size__ = image_ ? image_size : 0;
            pref_rect.right = std::max(image_size__, text_rect.width()) + _text_width_space;
            pref_rect.bottom = image_size__ + font_size + _text_height_space + _text_height_space / 2;
        }
        break;
        case button_view::switcher: case button_view::radio:
        {
            const auto image_width = image_ ? image_->width() : 0;
            pref_rect.right = image_width + text_rect.right + _ident_left + _text_width_space;
            const auto image_height = image_ ? image_->height() : 0;
            pref_rect.bottom = std::max(image_height, font_size) + _text_height_space;
        }
        break;
    }
    if (button_view::anchor != button_view_)
    {
        const auto val = 2 * theme_dimension(tcn, tv_border_width, theme_);
        pref_rect.right += val;
        pref_rect.bottom += val;
    }
    return pref_rect;
}

void button::draw(graphic &gr, const rect&)
{
    if (!showed_ || position_.is_hide())
    {
        return;
    }

    auto font_ = std::move(theme_font(tcn, tv_font, theme_));

    if (!caption_org.empty() && 0 == text_rect_.width()
        && button_view_ != button_view::image && button_view_ != button_view::image_menu)
    {
        text_rect_ = measure_text(caption_org, font_, &gr);
    }

    int32_t text_top{}, text_left{}, image_left{}, image_top{};
    rect control_pos{};

    const auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    switch (button_view_)
    {
        case button_view::text: case button_view::anchor:
        {
            const auto val = 2 * border_width;
            auto ident = val;
            if (button_view_ == button_view::anchor)
            {
                ident += _ident_left;
            }
            if (text_rect_.right + ident + _text_width_space > position_.width())
            {
                position_.right = position_.left + ident + text_rect_.right + _text_width_space;
            }
            if (font_.size + val + _text_height_space > position_.height())
            {
                position_.bottom = position_.top + val + font_.size + _text_height_space;
            }

            control_pos = position();
            text_left = control_pos.left;
            if (button_view_ != button_view::anchor)
            {
                text_left += (control_pos.width() - text_rect_.right) / 2;
            }
            else
            {
                text_left += ident;
            }
            text_top = control_pos.top + (control_pos.height() - font_.size) / 2;
        }
        break;
        case button_view::sheet:
        {
            control_pos = position();
            text_left = control_pos.left;

            // NB: текст на одной линии
            // дает одинаковый вид для нескольких sheet
            int32_t shift_y = (control_pos.height() - font_.size) / 2 - 2 * _sheet_bottom_space;
            if (shift_y < 0) shift_y = 0;
            text_top = control_pos.top + shift_y;

            caption = caption_org;
            truncate_line(caption, &gr, font_, control_pos.right - text_left);
        }
        break;
        case button_view::image:
        case button_view::image_menu:
            if (image_)
            {
                const auto val = 2 * border_width;
                if (image_size + val > position_.width())
                {
                    position_.right = position_.left + image_size + val;
                }
                if (image_size + val > position_.height())
                {
                    position_.bottom = position_.top + image_size + val;
                }

                control_pos = position();
                image_left = control_pos.left + (control_pos.width() - image_size) / 2;
                image_top = control_pos.top + (control_pos.height() - image_size) / 2;
            }
            break;
        case button_view::image_right_text:
        {
            const auto val = 2 * border_width;
            const auto image_size__ = image_ ? image_size : 0;
            if (image_size__ + val + text_rect_.right + _text_width_space + _text_width_space / 2 > position_.width())
            {
                position_.right = position_.left + image_size__ + val + text_rect_.right + _text_width_space + _text_width_space / 2;
            }
            const auto h = val + std::max(image_size__, font_.size) + _text_height_space;
            if (h > position_.height())
            {
                position_.bottom = position_.top + h;
            }

            control_pos = position();
            image_left = control_pos.left + (val + _text_width_space) / 2;
            image_top = control_pos.top + (control_pos.height() - image_size__) / 2;
            text_left = image_left + image_size__ + _text_width_space / 2;
            text_top = control_pos.top + (control_pos.height() - text_rect_.bottom) / 2;
        }
        break;
        case button_view::image_bottom_text:
        {
            const auto val = 2 * border_width;
            const auto image_size__ = image_ ? image_size : 0;
            const auto w = val + std::max(image_size__, text_rect_.width()) + _text_width_space;
            if (w > position_.width())
            {
                position_.right = position_.left + w;
            }
            if (image_size__ + val + font_.size + _text_height_space + _text_height_space / 2 > position_.height())
            {
                position_.bottom = position_.top + image_size__ + val + font_.size + _text_height_space + _text_height_space / 2;
            }

            control_pos = position();
            image_left = control_pos.left + ((control_pos.width() - image_size__) / 2);
            image_top = control_pos.top + border_width + _text_height_space / 2;
            text_top = image_top + image_size__ + _text_height_space / 2;
            text_left = control_pos.left + (control_pos.width() - text_rect_.right) / 2;
        }
        break;
        case button_view::switcher: case button_view::radio:
        {
            const auto val = 2 * border_width;
            const auto image_width = image_ ? image_->width() : 0;
            if (image_width + val + text_rect_.right + _ident_left + _text_width_space > position_.width())
            {
                position_.right = position_.left + image_width + val + text_rect_.right + _ident_left + _text_width_space;
            }
            const auto image_height = image_ ? image_->height() : 0;
            const auto h = std::max(image_height, font_.size) + val + _text_height_space;
            if (h > position_.height())
            {
                position_.bottom = position_.top + h;
            }

            control_pos = position();
            image_left = control_pos.left + _ident_left + border_width;
            text_left = image_left + image_width + _text_width_space / 2;
            image_top = control_pos.top + (control_pos.height() - image_height) / 2;
            // NB: текст на одной линии
            // дает одинаковый вид для нескольких radio
            text_top = control_pos.top + (control_pos.height() - font_.size) / 2;

            caption = caption_org;
            truncate_line(caption, &gr, font_, control_pos.right - val / 2 - text_left - _text_width_space / 2);
        }
        break;
        default:
            return;
    }

    const bool draw_rect_ = (button_view_ != button_view::anchor && button_view_ != button_view::switcher
        && button_view_ != button_view::radio && button_view_ != button_view::sheet);
    const auto round = theme_dimension(tcn, tv_round, theme_);
    if (draw_rect_)
    {
        //auto border_color = focused_
        //    ? theme_color(tcn, tv_focused_border, theme_)
        //    : (!active_ ? theme_color(tcn, tv_border, theme_) : theme_color(tcn, tv_hover_border, theme_));

        color border_color, fill_color;
        if (enabled_)
        {
            if (active_)
            {
                border_color = theme_color(tcn, tv_hover_border, theme_);
            }
            else
            {
                if (focused_ || button_view_ != button_view::image_menu)
                {
                    border_color = theme_color(tcn, tv_focused_border, theme_);
                }
                else
                {
                    border_color = button_view_ != button_view::image_menu ? theme_color(tcn, tv_border, theme_) :
                        theme_color(window::tc, window::tv_background, theme_);
                }
            }

            fill_color = (active_ || turned_ ? theme_color(tcn, tv_active, theme_) :
                theme_color(tcn, tv_calm, theme_));
        }
        else
        {
            fill_color = theme_color(tcn, tv_disabled, theme_);
            border_color = button_view_ != button_view::image_menu ? fill_color :
                theme_color(window::tc, window::tv_background, theme_);
        }

        gr.draw_rect(control_pos, border_color, fill_color, border_width, round);
    }
    else
    {
        // linux: background not redraw, clear
        const auto background = theme_color(window::tc, window::tv_background, theme_);
        const auto border_color = focused_ && button_view_ != button_view::sheet ?
            theme_color(tcn, tv_disabled, theme_) : make_color(0, 0, 0, 0);
        gr.draw_rect(control_pos, border_color, background, border_width, round);
    }

    if (image_ && button_view_ != button_view::text && button_view_ != button_view::anchor)
    {
        image_->set_position( { image_left,
            image_top,
            image_left + (button_view_ != button_view::switcher
                && button_view_ != button_view::radio ? image_size : image_->width()),
            image_top + (button_view_ != button_view::switcher
                && button_view_ != button_view::radio ? image_size : image_->height()) });
        image_->draw(gr, { 0 });
    }

    if (!caption.empty() && button_view_ != button_view::image && button_view_ != button_view::image_menu)
    {
        auto color_ = make_color(0, 0, 0, 0);

        if (button_view_ == button_view::anchor)
        {
            color_ = theme_color(tcn, tv_anchor, theme_);
            font_.decorations_ = decorations::underline;
        }

        if (!enabled_
            && (button_view::anchor == button_view_
                || button_view::sheet == button_view_
                || button_view::radio == button_view_
                || button_view::switcher == button_view_
                ))
        {
            color_ = theme_color(tcn, tv_disabled, theme_);
        }

        if (make_color(0, 0, 0, 0) == color_)
        {
            color_ = theme_color(tcn, tv_text, theme_);
        }

        gr.draw_text({ text_left, text_top }, caption, color_, font_);
    }

    if (button_view_ == button_view::sheet && (turned_  || focused_) && enabled_)
    {
        gr.draw_rect({ control_pos.left, control_pos.bottom - 3,
            control_pos.left + text_rect_.width(), control_pos.bottom - 1 },
            turned_ ? theme_color(tcn, tv_sheet, theme_) : theme_color(tcn, tv_disabled, theme_));
    }
}

void button::receive_event(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (ev.type & event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
            {
                active_ = true;
                set_cursor(parent_, button_view_ != button_view::anchor ? cursor::default_ : cursor::hand);
                redraw();

                if (!caption.empty() && (button_view_ == button_view::image
                    || button_view_ == button_view::image_menu))
                {
                    tooltip_->show_on_control(*this, 5);
                }
            }
            break;
            case mouse_event_type::leave:
            {
                pushed = false;

                if (!caption.empty() && (button_view_ == button_view::image
                    || button_view_ == button_view::image_menu))
                {
                    tooltip_->hide();
                }

                active_ = false;
                set_cursor(parent_, cursor::default_);
                redraw();
            }
            break;
            case mouse_event_type::left_down:
                pushed = true;
                if (click_callback_down)
                {
                    click_callback_down();
                }
                redraw();
                break;
            case mouse_event_type::left_up:
                if (pushed)
                {
                    active_ = false;
                    tooltip_->hide();

                    if (button_view_ == button_view::switcher || button_view_ == button_view::radio)
                    {
                        turn(!turned_);
                    }

                    if (click_callback)
                    {
                        click_callback();
                    }

                    pushed = false;
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
                if (focusing_)
                {
                    focused_ = true;

                    redraw();
                }
            break;
            case internal_event_type::remove_focus:
                focused_ = false;
                redraw();
            break;
            case internal_event_type::execute_focused:
                if (button_view_ == button_view::switcher || button_view_ == button_view::radio)
                {
                    turn(!turned_);
                }
                if (click_callback)
                {
                    click_callback();
                }
                else
                {
                    if (click_callback_down)
                    {
                        click_callback_down();
                    }
                }
                redraw();
            break;
        }
    }
}

void button::set_position(const rect& position__)
{
    position_ = position__;
    position_org = position__;
}

rect button::position() const
{
    return get_control_position(position_, parent_);
}

void button::move(const int32_t dx, const int32_t dy)
{
    position_.move(dx, dy);
    position_org = position_;
}

void button::set_parent(std::shared_ptr<window> window_)
{
    active_ = false;
    focused_ = false;

    parent_ = window_;
    window_->add_control(tooltip_, tooltip_->position());
    my_subscriber_id = window_->subscribe(std::bind(&button::receive_event, this, std::placeholders::_1),
        wui::event_type::internal | wui::event_type::mouse,
        shared_from_this());
}

std::weak_ptr<window> button::parent() const
{
    return parent_;
}

void button::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(tooltip_);
        parent__->unsubscribe(my_subscriber_id);
    }
    parent_.reset();
}

void button::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool button::topmost() const
{
    return topmost_;
}

bool button::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool button::focusing() const
{
    return enabled_ && showed_ && focusing_;
}

error button::get_error() const
{
    return err;
}

void button::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void button::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    tooltip_->update_theme(theme_);

    position_ = position_org;
    text_rect_.clear();

    if (button_view_ == button_view::switcher)
    {
        image_->change_image_raw(turned_ ? ti_switcher_on : ti_switcher_off);
        if (!image_->get_error().is_ok())
        {
            update_err("button::update_theme[switcher]", image_->get_error());
        }
    }
    if (button_view_ == button_view::radio)
    {
        image_->change_image_raw(turned_ ? ti_radio_on : ti_radio_off);
        if (!image_->get_error().is_ok())
        {
            update_err("button::update_theme[radio]", image_->get_error());
        }
    }
    else if (image_)
    {
        image_->update_theme(theme_);
    }

    redraw();
}

void button::show()
{
    if (!showed_)
    {
        showed_ = true;
        redraw();
    }
}

void button::hide()
{
    showed_ = false;
    tooltip_->hide();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto pos = position();
        pos.widen(theme_dimension(tcn, tv_border_width, theme_));
        parent__->redraw(pos, true);
    }
}

bool button::showed() const
{
    return showed_;
}

void button::enable()
{
    if (!enabled_)
    {
        enabled_ = true;
        if (image_) image_->enable();
        redraw();
    }
}

void button::disable()
{
    if (enabled_)
    {
        enabled_ = false;
        if (image_) image_->disable();
        redraw();
    }
}

bool button::enabled() const
{
    return enabled_;
}

void button::set_caption(std::string_view caption_)
{
    caption = caption_;
    caption_org = caption_;
    tooltip_->set_text(caption_);

    text_rect_.clear();

    redraw();
}

void button::set_button_view(button_view button_view__)
{
    button_view_ = button_view__;

    redraw();
}

#ifdef _WIN32
void button::set_image(int32_t resource_index)
{
    if (image_)
    {
        image_->change_image(resource_index);
    }
    else
    {
        image_ = std::make_shared<image>(resource_index);
        image_->redraw();
    }

    if (!image_->get_error().is_ok())
    {
        update_err("button::set_image[from resource]", image_->get_error());
    }
    redraw();
}
#endif

void button::set_image(std::string_view file_name)
{
    if (image_)
    {
        image_->change_image(file_name);
    }
    else
    {
        image_ = std::make_shared<image>(file_name);
        image_->redraw();
    }

    if (!image_->get_error().is_ok())
    {
        update_err("button::set_image[from file]", image_->get_error());
    }
    redraw();
}

void button::set_image(const std::vector<uint8_t> &image_data)
{
    if (image_)
    {
        image_->change_image(image_data);
    }
    else
    {
        image_ = std::make_shared<image>(image_data);
        image_->redraw();
    }

    if (!image_->get_error().is_ok())
    {
        update_err("button::set_image[from data]", image_->get_error());
    }
    redraw();
}

void button::enable_focusing()
{
    focusing_ = true;
}

void button::disable_focusing()
{
    focusing_ = false;
}

void button::turn(bool on)
{
    turned_ = on;
    switch (button_view_)
    {
        case button_view::switcher:
            image_->change_image_raw(turned_ ? ti_switcher_on : ti_switcher_off);
            if (!image_->get_error().is_ok())
            {
                update_err("button::turn", image_->get_error());
            }
        break;
        case button_view::radio:
            image_->change_image_raw(turned_ ? ti_radio_on : ti_radio_off);
            if (!image_->get_error().is_ok())
            {
                update_err("button::turn", image_->get_error());
            }
        break;
        default:
        break;
    }
    redraw();
}

bool button::turned() const
{
    return turned_;
}

void button::set_callback(std::function<void(void)> click_callback_)
{
    click_callback = click_callback_;
}

void button::set_callback_down(std::function<void(void)> click_callback_)
{
    click_callback_down = click_callback_;
}

void button::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_border_width, theme_));
            parent__->redraw(pos, true);
        }
    }
}

void button::update_err(std::string_view place, const error &input_err)
{
    err.set(input_err.get_type(), std::string(place) + "::" + input_err.get_component(),
        input_err.get_message());
}

}
