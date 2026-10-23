//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <ButtonSheet/ButtonSheet.h>

#include <Resource.h>

#include <wui/common/about.hpp>
#include <wui/locale/locale.hpp>
#include <wui/system/tools.hpp>
#include <wui/system/uri_tools.hpp>

#include <iostream>

ButtonSheet::ButtonSheet()
    : parentWindow_(),
    buttonsText(std::make_shared<wui::text>("Click on the buttons\n\t\tbelow the text", wui::hori_alignment::left, wui::vert_alignment::top, "h1_text")),
    simpleButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "simple0"), [this]() { buttonsText->set_text("First button pushed"); }, wui::button_view::text)),
    simpleButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "simple1"), [this]() { buttonsText->set_text("Second button pushed"); }, wui::button_view::text)),
    imageButton(std::make_shared<wui::button>("", [this]() { buttonsText->set_text("Image button pushed"); }, wui::button_view::image, IMG_LOGO, 32)),
    imageRightTextButton(std::make_shared<wui::button>(wui::locale("button_sheet", "right_text"), [this]() { buttonsText->set_text("Image with right text button pushed"); }, wui::button_view::image_right_text, IMG_LOGO, 32)),
    imageBottomTextButton(std::make_shared<wui::button>(wui::locale("button_sheet", "bottom_text"), [this]() { buttonsText->set_text("Image with bottom text button pushed"); }, wui::button_view::image_bottom_text, IMG_LOGO, 32, "rounded__green_button")),
    switcherButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "switcher_text"), [this]() { buttonsText->set_text("Switcher button pushed"); }, wui::button_view::switcher)),
    switcherButton2(std::make_shared<wui::button>("enable-disable buttons", [this]()
        {
            SetDisableButtons(switcherButton2->turned());
        }, wui::button_view::switcher)),
    radioButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "radio0_text"), [this]() { buttonsText->set_text("Radio first pushed"); radioButton1->turn(!radioButton0->turned()); }, wui::button_view::radio)),
    radioButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "radio1_text"), [this]() { buttonsText->set_text("Radio second pushed"); radioButton0->turn(!radioButton1->turned()); }, wui::button_view::radio)),
    anchorButton(std::make_shared<wui::button>(wui::locale("button_sheet", "anchor_text"), [this]() { buttonsText->set_text("Anchor pushed"); }, wui::button_view::anchor)),
    sheetButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "sheet0_text"), [this]() { buttonsText->set_text("First sheet pushed"); sheetButton0->turn(true); sheetButton1->turn(false); }, wui::button_view::sheet)),
    sheetButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "sheet1_text"), [this]() { buttonsText->set_text("Second sheet pushed"); sheetButton0->turn(false); sheetButton1->turn(true); }, wui::button_view::sheet))
{
    radioButton0->turn(true);
    sheetButton0->turn(true);

    if (!imageButton->get_error().is_ok())
    {
        std::cerr << imageButton->get_error().str() << std::endl;
    }
}

void ButtonSheet::SetDisableButtons(bool disable)
{
    if (disable)
    {
        sheetButton0->disable();
        sheetButton1->disable();
        buttonsText->disable();
        simpleButton0->disable();
        simpleButton1->disable();
        imageButton->disable();
        imageRightTextButton->disable();
        imageBottomTextButton->disable();
        switcherButton1->disable();
        radioButton0->disable();
        radioButton1->disable();
        anchorButton->disable();
        return;
    }
    sheetButton0->enable();
    sheetButton1->enable();
    buttonsText->enable();
    simpleButton0->enable();
    simpleButton1->enable();
    imageButton->enable();
    imageRightTextButton->enable();
    imageBottomTextButton->enable();
    switcherButton1->enable();
    radioButton0->enable();
    radioButton1->enable();
    anchorButton->enable();
}

void ButtonSheet::Run(std::weak_ptr<wui::window> parentWindow__)
{
    parentWindow_ = parentWindow__;

    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        parentWindow->add_control(buttonsText, {});
        parentWindow->add_control(switcherButton2, {});
        parentWindow->add_control(simpleButton0, {});
        parentWindow->add_control(simpleButton1, {});
        parentWindow->add_control(imageButton, {});
        parentWindow->add_control(imageRightTextButton, {});
        parentWindow->add_control(imageBottomTextButton, {});
        parentWindow->add_control(switcherButton1, {});
        parentWindow->add_control(radioButton0, {});
        parentWindow->add_control(radioButton1, {});
        parentWindow->add_control(anchorButton, {});
        parentWindow->add_control(sheetButton0, {});
        parentWindow->add_control(sheetButton1, {});

        const auto parentPos = parentWindow->position();
        UpdateSize(parentPos.width(), parentPos.height());

        // UpdateSize() не перерисовывает controls
#if 1
        sheetButton0->redraw();
        sheetButton1->redraw();
        buttonsText->redraw();
        simpleButton0->redraw();
        simpleButton1->redraw();
        imageButton->redraw();
        imageRightTextButton->redraw();
        imageBottomTextButton->redraw();
        switcherButton1->redraw();
        switcherButton2->redraw();
        radioButton0->redraw();
        radioButton1->redraw();
        anchorButton->redraw();
#else
        parentWindow->redraw(); // clear = true for all sheet, text etc
#endif
    }
}

void ButtonSheet::End()
{
    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        parentWindow->remove_control(sheetButton1);
        parentWindow->remove_control(sheetButton0);
        parentWindow->remove_control(anchorButton);
        parentWindow->remove_control(radioButton1);
        parentWindow->remove_control(radioButton0);
        parentWindow->remove_control(switcherButton1);
        parentWindow->remove_control(switcherButton2);
        parentWindow->remove_control(imageBottomTextButton);
        parentWindow->remove_control(imageRightTextButton);
        parentWindow->remove_control(imageButton);
        parentWindow->remove_control(simpleButton1);
        parentWindow->remove_control(simpleButton0);
        parentWindow->remove_control(buttonsText);
    }
}

void ButtonSheet::UpdateSize(int32_t width , int32_t height)
{
    constexpr int32_t space = 15;

    int32_t top = 80;
    const int32_t text_height = buttonsText->get_preferred_size().height(); // -10 test clip on/off
    buttonsText->set_position({ 10, top, width - 10, top + text_height });
    top += text_height + space;

    wui::rect r = switcherButton2->get_preferred_size();
    switcherButton2->set_position({ 10, top, 10 + r.width(), top + r.height() });
    top += r.height() + space;

    r = simpleButton0->get_preferred_size();
    simpleButton0->set_position({ 10, top, 10 + r.width(), top + r.height() });
    wui::rect r1 = simpleButton1->get_preferred_size();
    simpleButton1->set_position({ 20 + r.width(), top, 20 + r.width() + r1.width(), top + r1.height() });
    top += r1.height() + space;

    r = imageButton->get_preferred_size();
    r1 = imageRightTextButton->get_preferred_size();
    const auto h = std::max(r.height(), r1.height());
    const auto h_image = std::max(r.width(), h);
    imageButton->set_position({ 10, top, 10 + h_image, top + h });
    imageRightTextButton->set_position({ 20 + h_image, top, 20 + h_image + r1.width(), top + h });
    top += h + space;

    r = imageBottomTextButton->get_preferred_size();
//    imageBottomTextButton->set_position({ 10, top, 10 + r.width(), top + r.height() });
    imageBottomTextButton->set_position({ 10, top, width - 10, top + r.height() });
    top += r.height() + space;

    r = switcherButton1->get_preferred_size();
    switcherButton1->set_position({ 10, top, 10 + r.width(), top + r.height() });
    top += r.height() + space;

    r = radioButton0->get_preferred_size();
    radioButton0->set_position({ 10, top, 10 + r.width(), top + r.height() });
    r1 = radioButton1->get_preferred_size();
    radioButton1->set_position({ 10 + 20 + r.width(), top, 10 + 20 + r.width() + r1.width(), top + r1.height() });
    top += r.height() + space;

    r = anchorButton->get_preferred_size();
    anchorButton->set_position({ 10, top, 10 + r.width(), top + r.height() });
    top += r.height() + 20;

    r = sheetButton0->get_preferred_size();
    sheetButton0->set_position({ 10, top, 10 + r.width(), top + r.height() });
    r1 = sheetButton1->get_preferred_size();
    sheetButton1->set_position({ 10 + 10 + r.width(), top, 10 + 10 + r.width() + r1.width(), top + r1.height() });
}