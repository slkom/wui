//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/framework/framework.hpp>

#include <wui/config/config.hpp>

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>

#include <wui/locale/locale.hpp>

#include <MainFrame/MainFrame.h>

#include <Resource.h>

#include <iostream>


MainFrame::MainFrame()
    : window(std::make_shared<wui::window>()),


    mainSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "main_sheet_"),   [this](){ sheet = Sheet::Main;   UpdateSheets(); }, wui::button_view::sheet)),
    windowSheet  (std::make_shared<wui::button>(wui::locale("main_frame", "window_sheet_"), [this](){ sheet = Sheet::Window; UpdateSheets(); }, wui::button_view::sheet)),
    buttonSheet  (std::make_shared<wui::button>(wui::locale("main_frame", "button_sheet_"), [this](){ sheet = Sheet::Button; UpdateSheets(); }, wui::button_view::sheet)),
    inputSheet   (std::make_shared<wui::button>(wui::locale("main_frame", "input_sheet_"),  [this](){ sheet = Sheet::Input;  UpdateSheets(); }, wui::button_view::sheet)),
    listSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "list_sheet_"),   [this](){ sheet = Sheet::List;   UpdateSheets(); }, wui::button_view::sheet)),
    menuSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "menu_sheet_"),   [this](){ sheet = Sheet::Menu;   UpdateSheets(); }, wui::button_view::sheet)),
    panelSheet   (std::make_shared<wui::button>(wui::locale("main_frame", "other_sheet_"),  [this](){ sheet = Sheet::Others; UpdateSheets(); }, wui::button_view::sheet)),

    accountButton(std::make_shared<wui::button>(wui::locale("main_frame", "account_btn"), nullptr, wui::button_view::image, IMG_ACCOUNT, 32, wui::button::tc_tool)),
    menuButton   (std::make_shared<wui::button>(wui::locale("main_frame", "main_menu"), nullptr, wui::button_view::image_menu, IMG_MENU, 32, wui::button::tc_tool)),

    sheet(Sheet::Main)
{
    window->subscribe(std::bind(&MainFrame::ReceiveEvents,
        this,
        std::placeholders::_1),
        wui::event_type::internal | wui::event_type::system | wui::event_type::keyboard);

}

bool MainFrame::Run()
{
    window->set_control_callback([&](wui::window_control control,
        std::string &tooltip_text, [[maybe_unused]] bool &continue_) {
        switch (control)
        {
            case wui::window_control::theme:
            {
                auto nextTheme = std::move(wui::get_next_app_theme());
                wui::error err;
                wui::set_default_theme_from_name(nextTheme, err);
                if (!err.is_ok())
                {
                    std::cerr << err.str() << std::endl;
                    return;
                }

                wui::config::set_string("User", "Theme", nextTheme);

                window->update_theme();

                tooltip_text = wui::locale("window", nextTheme == "dark" ? wui::window::cl_light_theme : wui::window::cl_dark_theme);
            }
            break;
            case wui::window_control::lang:
            {
            }
            break;
        }

    });

    window->set_min_size(WND_WIDTH - WND_WIDTH / 6, WND_HEIGHT);

    return window->init(wui::locale("main_frame", "caption"), { -1, -1, WND_WIDTH, WND_HEIGHT },
        wui::window_style::frame | wui::window_style::switch_theme_button | wui::window_style::border_all,
        [this]() {});
}

void MainFrame::ReceiveEvents(const wui::event &ev)
{
    if (ev.type & wui::event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case wui::internal_event_type::window_created:
                window->add_control(mainSheet, { });
                window->add_control(windowSheet, { });
                window->add_control(buttonSheet, { });
                window->add_control(inputSheet, { });
                window->add_control(listSheet, { });
                window->add_control(menuSheet, { });
                window->add_control(panelSheet, { });
                window->add_control(accountButton, { });
                window->add_control(menuButton, { });

                UpdateSheetsSize();

                UpdateSheets();
            break;
            case wui::internal_event_type::size_changed:
            case wui::internal_event_type::window_expanded:
                UpdateSheetsSize();
            break;
            case wui::internal_event_type::window_normalized:
            break;
            case wui::internal_event_type::window_minimized: // iconic
            break;
        }
    }
}

void MainFrame::UpdateSheetsSize()
{
    constexpr int32_t ident = 10;
    constexpr int32_t space = 20;

    const auto width = window->position().width(), height = window->position().height();
    //const auto sheet_width = (width - 126) / 7;

    const auto sheets_top = window->caption_height() + 2;

    const int32_t sheets_height = mainSheet->font_max_height() + 8;
    mainSheetImpl.set_sheets_height(sheets_height);

    auto rc = mainSheet->get_preferred_size();
    auto sheet_width = rc.width();
    auto l = ident;
    mainSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = windowSheet->get_preferred_size();
    sheet_width = rc.width();
    windowSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = buttonSheet->get_preferred_size();
    sheet_width = rc.width();
    buttonSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = inputSheet->get_preferred_size();
    sheet_width = rc.width();
    inputSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = listSheet->get_preferred_size();
    sheet_width = rc.width();
    listSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = menuSheet->get_preferred_size();
    sheet_width = rc.width();
    menuSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });
    l += sheet_width + space;
    rc = panelSheet->get_preferred_size();
    sheet_width = rc.width();
    panelSheet->set_position({ l, sheets_top, l + sheet_width, sheets_top + sheets_height });

    const int32_t button_size = 36;

    accountButton->set_position({ width - button_size * 2 - 4, 30, width - button_size - 4, 30 + button_size });
    menuButton->set_position({ width - button_size - 2, 30, width - 2, 30 + button_size });

    switch (sheet)
    {
        case Sheet::Main:
            mainSheetImpl.UpdateSize(width, height);
            break;
        case Sheet::Window:
            break;
        case Sheet::Button:
            buttonSheetImpl.UpdateSize(width, height);
            break;
        case Sheet::Input:
            inputSheetImpl.UpdateSize(width, height);
            break;
        case Sheet::List:
            break;
        case Sheet::Menu:
            break;
        case Sheet::Others:
            break;
    }
}

void MainFrame::UpdateSheets()
{
    mainSheet  ->turn(false);
    windowSheet->turn(false);
    buttonSheet->turn(false);
    inputSheet ->turn(false);
    listSheet  ->turn(false);
    menuSheet  ->turn(false);
    panelSheet ->turn(false);

    switch (sheet)
    {
        case Sheet::Main:
            mainSheet->turn(true);

            buttonSheetImpl.End();
            inputSheetImpl.End();

            mainSheetImpl.Run(window);
            break;
        case Sheet::Window:
            windowSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Button:
            buttonSheet->turn(true);

            mainSheetImpl.End();
            inputSheetImpl.End();

            buttonSheetImpl.Run(window);
            break;
        case Sheet::Input:
            inputSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();

            inputSheetImpl.Run(window);
        break;
        case Sheet::List:
            listSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Menu:
            menuSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Others:
            panelSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
    }
}

