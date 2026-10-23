//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <InputSheet/InputSheet.h>

#include <Resource.h>

#include <wui/common/about.hpp>
#include <wui/locale/locale.hpp>
#include <wui/system/tools.hpp>
#include <wui/system/uri_tools.hpp>

InputSheet::InputSheet()
    : parentWindow_(),
    inputText(std::make_shared<wui::text>("", wui::hori_alignment::left, wui::vert_alignment::top, "h1_text")),
    input0(std::make_shared<wui::input>("Type some text here", wui::input_view::multiline))
{
    input0->set_change_callback([this]() { inputText->set_text(input0->text()); });
    input0->set_return_callback([this]() {
            inputText->set_text(input0->text() + "\n\n* You press 'RETURN'...");
        });
}

void InputSheet::Run(std::weak_ptr<wui::window> parentWindow__)
{
    parentWindow_ = parentWindow__;

    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        auto parentPos = parentWindow->position();
        auto width = parentPos.width(), height = parentPos.height();

        parentWindow->add_control(inputText, {});
        parentWindow->add_control(input0, {});
        //inputText->update_theme();
        //input0->update_theme();

        UpdateSize(width, height);

        inputText->redraw();
        input0->redraw();
    }
}

void InputSheet::End()
{
    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        parentWindow->remove_control(inputText);
        parentWindow->remove_control(input0);
    }
}

void InputSheet::UpdateSize(int32_t width, int32_t height)
{
    inputText->set_position({ 40, 100, width - 40, height - 240 });
    input0->set_position({ 40, height - 200, width - 40, height - 40 });
}
