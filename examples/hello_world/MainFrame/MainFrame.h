//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/window/window.hpp>

#include <wui/locale/locale.hpp>
#include <wui/control/image.hpp>
#include <wui/control/text.hpp>
#include <wui/control/input.hpp>
#include <wui/control/button.hpp>
#include <wui/control/message.hpp>

#include <Resource.h>

class MainFrame
{
public:
    MainFrame();

    bool Run();

private:

    void ReceiveEvents(const wui::event& ev);

    void UpdateControlsPosition();
    void UpdateButtonPosition(const int32_t button_h);

    void OnOK();

    static constexpr int32_t WND_WIDTH = 340, WND_HEIGHT = 340;

    std::shared_ptr<wui::window> window = std::make_shared<wui::window>();

    std::shared_ptr<wui::image> logoImage = std::make_shared<wui::image>(IMG_LOGO);

    std::shared_ptr<wui::text> whatsYourNameText = std::make_shared<wui::text>(
        wui::locale("main_frame", "whats_your_name_text"),
        wui::hori_alignment::center, wui::vert_alignment::center,
        "h1_text");

    std::shared_ptr<wui::input> userNameInput = std::make_shared<wui::input>(std::move(wui::config::get_string("User", "Name", "")));

    std::shared_ptr<wui::button> okButton = std::make_shared<wui::button>(
        wui::locale("main_frame", "ok_button"), [this]() { OnOK(); });

    std::shared_ptr<wui::message> messageBox = std::make_shared<wui::message>(window);

    bool user_approve_close{ false };
    bool hello_again{ false };
};

