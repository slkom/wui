//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/config/config.hpp>

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>

#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>

//#include <wui/framework/framework.hpp>

#include <wui/system/tools.hpp>

#include <MainFrame/MainFrame.h>

#include <iostream>

MainFrame::MainFrame()
{
    window->subscribe([this](const wui::event& ev) { ReceiveEvents(ev); },
        wui::event_type::internal | wui::event_type::system | wui::event_type::keyboard);

    window->set_min_size(WND_WIDTH, WND_HEIGHT);
}

bool MainFrame::Run()
{
    window->set_control_callback([&](wui::window_control control, std::string& tooltip_text, bool& continue_)
        {
            switch (control)
            {
                case wui::window_control::theme:
                {
                    wui::error err;

                    auto nextTheme = std::move(wui::get_next_app_theme());
                    wui::set_default_theme_from_name(nextTheme, err);
                    if (!err.is_ok())
                    {
                        std::cerr << err.str() << std::endl;
                        return;
                    }

                    wui::config::set_string("User", "Theme", nextTheme);

                    window->update_theme();

                    tooltip_text = wui::locale("window", nextTheme == "dark" ?
                        wui::window::cl_light_theme : wui::window::cl_dark_theme);
                    UpdateControlsPosition();
                    okButton->redraw();
                }
                break;
                case wui::window_control::lang:
                {
                    wui::error err;

                    auto nextLocale = wui::get_next_app_locale();
                    wui::set_locale_from_type(nextLocale, err);
                    if (!err.is_ok())
                    {
                        std::cerr << err.str() << std::endl;
                        return;
                    }

                    wui::config::set_int("User", "Locale", static_cast<int32_t>(nextLocale));

                    tooltip_text = wui::locale("window", "switch_lang");

                    window->set_caption(wui::locale("main_frame", "caption"));
                    whatsYourNameText->set_text(wui::locale("main_frame", "whats_your_name_text"));
                    okButton->set_caption(wui::locale("main_frame", "ok_button"));
                    UpdateControlsPosition();
                    okButton->redraw();
                }
                break;
                case wui::window_control::close:
                    if (!user_approve_close)
                    {
                        continue_ = false;
                        messageBox->set_docked();
                        messageBox->show(wui::locale("main_frame", "confirm_close_text"),
                            wui::locale("main_frame", "cross_message_caption"),
                            wui::message_icon::information, wui::message_button::yes_no,
                            [this](wui::message_result r)
                            {
                                if (r == wui::message_result::yes)
                                {
                                    wui::config::set_int("MainFrame", "Width", window->position().width());
                                    wui::config::set_int("MainFrame", "Height", window->position().height());
                                    user_approve_close = true;
                                    window->close();
                                }
                            });
                    }
                    break;
            }
        });

    const auto width = wui::config::get_int("MainFrame", "Width", WND_WIDTH);
    const auto height = wui::config::get_int("MainFrame", "Height", WND_HEIGHT);

#if 1
    // This example shows a dialog, then we show the main window.
    window->hide();
    const auto ret = window->init(wui::locale("main_frame", "caption"), { -1, -1, width, height },
        wui::window_style::frame | wui::window_style::switch_theme_button |
        wui::window_style::switch_lang_button | wui::window_style::border_all,
        [this]() { }); // init root window
    if (!ret)
        return false;
    OnOK();
    messageBox->event_loop();
    window->show();
    return ret;
#else
    return window->init(wui::locale("main_frame", "caption"), { -1, -1, width, height },
        wui::window_style::frame | wui::window_style::switch_theme_button |
        wui::window_style::switch_lang_button | wui::window_style::border_all,
        [this]() { });
#endif
}

void MainFrame::ReceiveEvents(const wui::event& ev)
{
    if (ev.type & wui::event_type::internal)
    {
        // The main window is already initialized (context and graphics).
        // Font size and str length calculations are available (get_preferred_size(), measure_text(), ...).
        switch (ev.internal_event_.type)
        {
            case wui::internal_event_type::window_created:
                // do not create temporary ui control objects
                window->add_control(logoImage, { 0 });
                window->add_control(whatsYourNameText, { 0 });
                window->add_control(userNameInput, { 0 });
                window->add_control(okButton, { 0 });

                window->set_default_push_control(okButton);
                break;
            case wui::internal_event_type::size_changed:
            case wui::internal_event_type::window_expanded:
                UpdateControlsPosition();
                break;
            case wui::internal_event_type::window_minimized: // iconic
                break;
        }
    }
}

constexpr int32_t _space = 30;

// Смена языка, темы может менять длину текста. Если текст заведомо внутри
// заданной позиции, get_preferred_size() можно не использовать,
// позиция может быть фиксированная, если не ожидается проблем с длинной и
// шириной текста (изменение языка, размера шрифта).
void MainFrame::UpdateButtonPosition(const int32_t button_h)
{
    const auto width = window->position().width(), height = window->position().height();
    const int32_t center = width / 2;
    const wui::rect rc_button = okButton->get_preferred_size();
    okButton->set_position({ center - rc_button.width() / 2 + 10,
         height - _space - rc_button.height(),
         center + rc_button.width() / 2 + 10,
         height - _space
        });
}

void MainFrame::UpdateControlsPosition()
{
    constexpr int32_t img_height = 40;
    const auto width = window->position().width(), height = window->position().height();
    const auto top = std::max(window->caption_height() + 4, _space);

    const wui::rect rc_t = whatsYourNameText->get_preferred_size();
    wui::rect pos{ _space, top, width - _space, top + rc_t.height() };
    whatsYourNameText->set_position(pos);

    const auto input_h = userNameInput->lines_max_height() + 4;
    wui::line_up_top_bottom(pos, input_h, _space);
    userNameInput->set_position(pos);
    wui::line_up_top_bottom(pos, img_height * 2, _space);

    const int32_t center = width / 2;

    pos.left = center - img_height, pos.right = center + img_height;

    logoImage->set_position(pos);
    UpdateButtonPosition(input_h);
}

void MainFrame::OnOK()
{
    wui::config::set_string("User", "Name", userNameInput->text());
    messageBox->set_docked(false);
    messageBox->show(
        wui::locale("main_frame", hello_again ? "hello_again_text" : "hello_text") +
        (userNameInput->text().empty() ? "%username%" : userNameInput->text()) + "! " + wui::locale("main_frame", "hello_text2"),
        wui::locale("main_frame", "ok_message_caption"), wui::message_icon::information,
        wui::message_button::ok, [this](wui::message_result)
        {
            hello_again = true;
        });
}