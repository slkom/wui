// wui.cpp : Defines the entry point for the application.
//

#include <wui/framework/framework.hpp>
#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>
#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>
#include <wui/window/window.hpp>
#include <wui/control/button.hpp>
#include <wui/control/input.hpp>
#include <wui/control/menu.hpp>
#include <wui/control/list.hpp>
#include <wui/control/select.hpp>
#include <wui/control/image.hpp>
#include <wui/control/text.hpp>
#include <wui/control/message.hpp>
#include <wui/control/splitter.hpp>
#include <wui/control/progress.hpp>
#include <wui/control/slider.hpp>
#include <wui/control/panel.hpp>

#ifdef _WIN32
#include <tchar.h>
#endif

#include <Resource.h>

#include <iostream>

static constexpr int32_t WND_WIDTH = 900, WND_HEIGHT = 640;
static constexpr wui::window_style main_window_style =
    wui::window_style::frame | wui::window_style::switch_theme_button | wui::window_style::border_all;

static constexpr wui::window_style pluged_window_style =
    wui::window_style::pinned | wui::window_style::border_right | wui::window_style::title_showed;
static constexpr wui::window_style unpluged_window_style =
    wui::window_style::pinned | wui::window_style::border_all | wui::window_style::title_showed;

constexpr int32_t _splitter_width = 5;
constexpr int32_t _splitter_default_left = 300;

static std::shared_ptr<wui::i_theme> MakeRedButtonTheme(const bool dark)
{
    auto redButtonTheme = wui::make_custom_theme();
    redButtonTheme->set_name("redButton");

    redButtonTheme->load_theme(*wui::get_default_theme());

    redButtonTheme->set_color(wui::button::tc, wui::button::tv_calm, wui::make_color(165, 15, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_active, wui::make_color(205, 15, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_border, dark ? wui::make_color(160, 160, 160) : wui::make_color(205, 15, 20));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_hover_border, dark ? wui::make_color(180, 180, 180) : wui::make_color(255, 0, 0));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_focused_border, dark ? wui::make_color(255, 255, 255) : wui::make_color(25, 25, 25));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_text, wui::make_color(190, 205, 190));
    redButtonTheme->set_color(wui::button::tc, wui::button::tv_disabled, wui::make_color(180, 190, 180));

    return redButtonTheme;
}

struct PluggedWindow : public std::enable_shared_from_this<PluggedWindow>
{
    std::weak_ptr<wui::window> parentWindow;
    std::shared_ptr<wui::splitter> vertSplitter;

    std::shared_ptr<wui::window> window;
    std::shared_ptr<wui::text> itemNumText;
    std::shared_ptr<wui::list> list3;
    std::shared_ptr<wui::menu> popupMenu;
    std::shared_ptr<wui::splitter> horizSplitter;
    std::shared_ptr<wui::panel> panel;
    std::shared_ptr<wui::button> button1, button2, button3, close_button3;
    std::shared_ptr<wui::text> text_account;
    std::shared_ptr<wui::input> input1;
    std::shared_ptr<wui::input> input2;
    std::shared_ptr<wui::text> selectedText;
    std::shared_ptr<wui::select> select1;
    std::shared_ptr<wui::input> input;
    std::shared_ptr<wui::message> messageBox2;
    std::shared_ptr<wui::window> dialog3;
    int32_t selectItemNum{ 0 };
    int32_t splitterPos{ 50 };
    bool plugged{ false };
    wui::rect unplugRect{};
    wui::rect plugRect{};

    static constexpr auto caption_plug = "Child plugged & docked";
    static constexpr auto caption_unplug = "Window unplugged";

    void Plug()
    {
        if (!unplugRect.empty())
            unplugRect = window->position(); // old unplug window position
        auto parentWindow_ = parentWindow.lock();
        if (parentWindow_)
        {
            plugged = true;
            parentWindow_->emit_event(5555, -1);

            const auto pos = parentWindow_->position();
            const auto cap_h = parentWindow_->caption_height(main_window_style);
            const auto splitter_pos = vertSplitter->position();
            plugRect = {
                    0, cap_h,
                    0 == splitter_pos.left ? _splitter_default_left : splitter_pos.left,
                    pos.empty() ? WND_HEIGHT : pos.height()
                };

            parentWindow_->add_control(window, plugRect);
            window->set_caption(caption_plug);
            window->set_style(pluged_window_style);
        }
    }

    void Unplug()
    {
        auto parentWindow_ = parentWindow.lock();
        if (parentWindow_)
        {
            parentWindow_->emit_event(5555, 1);
            parentWindow_->remove_control(window);
        }
        if (unplugRect.empty())
        {
            unplugRect = window->position(); // first
            const auto pos = parentWindow_->position();
            unplugRect.resize(unplugRect.width(), pos.height());
            unplugRect.put(pos.left - unplugRect.width() - 1, pos.top);
        }

        plugged = false;
        Init();
        window->set_caption(caption_unplug);
    }

    void Init()
    {
        if (!plugged)
        {
            auto parentWindow_ = parentWindow.lock();
            if (parentWindow_)
            {
                const auto pos = parentWindow_->position();
                unplugRect.put(pos.left - unplugRect.width() - 1, pos.top); // move left
            }
        }

        window->init(caption_plug, plugged ? plugRect : unplugRect,
            plugged ? pluged_window_style : unpluged_window_style,
            [this]() {
                auto parentWindow_ = parentWindow.lock();
                if (parentWindow_)
                {
                    parentWindow_->emit_event(5555, 0);
                }
            }
        );
    }

    void SplitterChange(int32_t , int32_t y)
    {
        const auto p = window->position();
        const auto h = p.height(), w = p.width();

        splitterPos = h - y;

        set_position_controls(w, h);
        window->redraw({ 0, 0, p.right, p.bottom }, true);
    }

    void set_position_controls(const int32_t w, const int32_t h)
    {
        constexpr int32_t space = 10;
        const int32_t caption_h = 2 + window->caption_height(pluged_window_style);

        //auto y = window->position().height() - splitterPos;
        auto y = h - splitterPos;

        const wui::rect rc_text = itemNumText->get_preferred_size();
        int32_t top = caption_h + 1 + caption_h/4;
        itemNumText->set_position({ space, top, w - 10, top + rc_text.height() });

        top += rc_text.height() + 1;
        list3->set_position({ space, top, w - 10, y - 10 });
        horizSplitter->set_position({ 0, y - 8 + 2, w - 1, y - 2 + 2 });
        horizSplitter->set_margins(top, h - 50);

        panel->set_position({ 0, y, w - 1, h });
        const wui::rect rc_button = button1->get_preferred_size();
        const int32_t y_t = y + 5;
        const int32_t y_b = y_t + rc_button.height() + 6;
        const int32_t button_w = rc_button.width() + 6;
        int32_t x_t = space;
        button1->set_position({ x_t, y_t, x_t + button_w, y_b });
        x_t += button_w + space;
        button2->set_position({ x_t, y_t, x_t + button_w, y_b });
        x_t += button_w + space;
        button3->set_position({ x_t, y_t, x_t + button_w, y_b });
        x_t += button_w + space;
        input->set_position({ x_t, y_t, w - 10, h - 10 });
    }

    PluggedWindow(std::shared_ptr<wui::window>& parentWindow_
        , std::shared_ptr<wui::splitter> vertSplitter_)
        : parentWindow(parentWindow_),
        vertSplitter(vertSplitter_),
        window(std::make_shared<wui::window>()),
        itemNumText(std::make_shared<wui::text>("")),
        list3(std::make_shared<wui::list>()),
        popupMenu(std::make_shared<wui::menu>()),

        horizSplitter(std::make_shared<wui::splitter>(wui::splitter::orientation::horizontal,
            [this](int32_t left, int32_t top) { SplitterChange(left, top); })),

        panel(std::make_shared<wui::panel>()),
        button1(std::make_shared<wui::button>("Button 1",
            [this]() {
                messageBox2->show(
                    "Lorem Ipsum is simply dummy text of the printing and typesetting industry."
                    "\nLorem Ipsum has been the industry's"
                    "\nstandard dummy text ever since the 1500s, when an unknown printer took"
                    "\na galley of type and scrambled it to make a type specimen book.",
                    "message box, modal [docked]", wui::message_icon::information, wui::message_button::ok, [](wui::message_result) {});
            }
        , wui::button_view::image, IMG_ACCOUNT, 16)),
        button2(std::make_shared<wui::button>("Button 2",
            [this]() {
                window->emit_event(310, 200);
            }
        , wui::button_view::image, IMG_ACCOUNT, 16)),
        button3(std::make_shared<wui::button>("Button 3",
            [this]() {
                dialog3->subscribe(
                    [this](const wui::event& e)
                    {
                        // The parent window for this control is already initialized (context and graphics).
                        // Font size and text length calculations are available for add_control(), etc (get_preferred_size(), measure_text(), ...)
                        if (e.type & wui::event_type::internal)
                        {
                            switch (e.internal_event_.type)
                            {
                                case wui::internal_event_type::window_created:
                                {
                                    // do not create temporary ui control objects
                                    constexpr int32_t space = 10;
                                    constexpr int32_t ctrl_width = 190;

                                    int32_t top = dialog3->caption_height();
                                    top += space;

                                    wui::rect r = text_account->get_preferred_size();
                                    int32_t hc = r.height() + 8;
                                    dialog3->add_control(text_account, { space, top, space + r.width(), top + hc });
                                    top += hc + space;

                                    hc = input1->get_font_size() + 8;
                                    dialog3->add_control(input1, { space, top, space + ctrl_width, top + hc });
                                    top += hc + space;

                                    hc = input2->get_font_size() + 8;
                                    dialog3->add_control(input2, { space, top, space + ctrl_width, top + hc });
                                    top += hc + space;

                                    hc = select1->get_font_size() + 8;

                                    const wui::rect rc_text = selectedText->get_preferred_size();
                                    dialog3->add_control(selectedText, { space, top + hc + space,
                                        space + ctrl_width, top + hc + space + rc_text.height() + 8 });

                                    select1->set_items( {
                                        { 1, "123" }, { 2, "256" }, { 3, "389" }, // test up
                                        { 4, "401112" }, { 5, "531415" }, { 6, "661718" }, { 7, "792021" }, // test down
                                        { 8, "822324" }, { 9, "922324" }
                                        } );

                                    select1->set_change_callback(
                                        [this](int32_t number, int64_t id)
                                        {
                                            selectItemNum = number;
                                            selectedText->set_text(-1 != id ? "Select item: " + select1->selected_item().text : "Select item: ---");
                                        }
                                    );

                                    if (selectItemNum >= select1->items().size())
                                    {
                                        selectItemNum = !select1->items().empty() ?
                                            static_cast<int32_t>(select1->items().size()) - 1 : -1;
                                    }

                                    select1->select_item_number(selectItemNum);

                                    dialog3->add_control(select1, { space, top, space + ctrl_width, top + hc });
                                    top += hc + space  + 50;

                                    const wui::rect rc_button = close_button3->get_preferred_size();

                                    const auto w_dialog = 2 * space + ctrl_width;
                                    const auto left_button = (w_dialog - rc_button.width() - 40)/2;
                                    dialog3->add_control(close_button3, { left_button, top, left_button + rc_button.width() + 40, top + rc_button.height() });
                                    top += hc + space;

                                    const auto caption = dialog3->docked() ?
                                        "Transient, docked, modal" : "Transient, dialog, modal";
                                    dialog3->set_caption(caption);
                                    dialog3->set_tw_preferred_position(w_dialog, top + 10);
                                }
                                break;
                            }
                        }
                    }, wui::event_type::internal);

                dialog3->set_transient_for(window);
                dialog3->init("", { 0, 0, 260, 350 },
                    wui::window_style::dialog);
            }, wui::button_view::image, IMG_ACCOUNT, 16)),

        close_button3(std::make_shared<wui::button>("OK",
            [this]()
            {
                dialog3->close();
            })),
        text_account(std::make_shared<wui::text>("Account",
            wui::hori_alignment::center, wui::vert_alignment::center,
            wui::text::tc)),
        input1(std::make_shared<wui::input>("98753")),
        input2(std::make_shared<wui::input>("99wegdyug")),
        selectedText(std::make_shared<wui::text>("Select item:")),
        select1(std::make_shared<wui::select>()),
        input(std::make_shared<wui::input>("multi-line text", wui::input_view::multiline)),
        messageBox2(std::make_shared<wui::message>(parentWindow_, true)),
        dialog3(std::make_shared<wui::window>())
    {
    }

    void Create()
    {
        window->subscribe(
            [this](const wui::event& e)
            {
                if (e.type & wui::event_type::internal)
                {
                    switch (e.internal_event_.type)
                    {
                        case wui::internal_event_type::window_created:
                        {
                            // do not create temporary ui control objects
                        }
                        break;
                    }
                }
            }
        , wui::event_type::internal);

        list3->set_draw_callback(std::bind(&PluggedWindow::draw_list_item,
            this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));

        list3->set_item_click_callback(
            [this](wui::list::click_button btn, int32_t n_item, int32_t x, int32_t y)
            {
                if (btn == wui::list::click_button::right)
                    popupMenu->show_on_point(x, y);
                else
                {
                    // saving the initial count
                    static int count = list3->get_item_count();
                    if (n_item % 2 == 0)
                        list3->set_item_count(count);
                    else
                        list3->set_item_count(count - 2);
                }
            });

        list3->set_item_change_callback([this](int32_t item)
            {
                itemNumText->set_text(item >= 0 ? "Select item: " + std::to_string(item + 1) : "Select item: ---");
            });

        list3->update_columns({ { 30, "##" }, { 80, "Name" }, { 100, "Role" } });

        list3->set_item_height_callback([](int32_t i, int32_t& h)
            {
                // change (increment) height items
                h = 32 + i * 2;
            });

        list3->set_item_count(21);
        list3->select_item(5);

        popupMenu->set_items({
            {{ 0, wui::menu_item_state::normal, "First", "", nullptr, {}, [](int32_t) {} }},
            {{ 1, wui::menu_item_state::separator, "Other", "", nullptr, {}, [](int32_t ) {} }},
            {{ 2, wui::menu_item_state::normal, "Another", "", nullptr, {}, [](int32_t ) {} }}
            });

        input->set_change_callback(
            [this]()
            {
                const auto constAdd = 35;
                auto lines = input->get_lines().size();
                auto font_size = input->get_font_size();

                if (lines < 9)
                {
                    auto wp = window->position();
                    const int32_t inputTop = static_cast<int32_t>(wp.height() - constAdd - (lines * font_size));
                    horizSplitter->set_position({ 0, inputTop + 2, wp.right, inputTop + 8 });
                    SplitterChange(0, inputTop);
                }
                else
                {
                    auto ip = input->position();
                    if (constAdd + lines * font_size > ip.height() && ip.height() < constAdd + 8 * font_size)
                    {
                        if (lines > 8) lines = 8;
                        auto wp = window->position();
                        const int32_t inputTop = static_cast<int32_t>(wp.height() - constAdd - (lines * font_size));
                        horizSplitter->set_position({ 0, inputTop + 2, wp.right, inputTop + 8 });
                        SplitterChange(0, inputTop);
                    }
                }
            });

        window->add_control(popupMenu, { 0 });

        window->add_control(itemNumText, { 0 });

        window->add_control(list3, { 0 });
        window->add_control(horizSplitter, { 0 });

        window->add_control(panel, { 0 });
        window->add_control(button1, { 0 });
        window->add_control(button2, { 0 });
        window->add_control(button3, { 0 });
        window->add_control(input, { 0 });

        window->set_control_callback([this](wui::window_control control,
            std::string &tooltip_text, bool )
            {
                if (control != wui::window_control::pin)
                {
                    return;
                }

                if (plugged)
                {
                    Unplug();
                    tooltip_text = wui::locale("window", "pin");
                }
                else
                {
                    Plug();
                    tooltip_text = wui::locale("window", "unpin");
                }
            });

        window->subscribe([this](const wui::event &e) {
            switch (e.type)
            {
            case wui::event_type::internal:
                switch(e.internal_event_.type)
                {
                    case wui::internal_event_type::window_created:
                    {
                        // do not create temporary ui control objects
                    }
                    break;
                    case wui::internal_event_type::size_changed:
                    {
                        set_position_controls(e.internal_event_.x, e.internal_event_.y);
                    }
                    break;
                    case wui::internal_event_type::user_emitted:
                    {
#if 1
                        // Uncomment this code to see the received event.
                        int32_t x = e.internal_event_.x, y = e.internal_event_.y;

                        messageBox2->show("user emitted event received, x: " + std::to_string(x) + ", y: " + std::to_string(y) + "\nShow dialog?",
                            "message box, docked", wui::message_icon::information, wui::message_button::yes_no, [this](wui::message_result result) {
                                if (result == wui::message_result::yes)
                                {
                                    dialog3->set_transient_for(window);
                                    dialog3->init("Transient, topmost, modal", { 50, 50, 350, 350 }, wui::window_style::dialog, []() {});
                                }
                            });
#endif
                        list3->make_selected_visible();
                    }
                    break;
                }
            break;
            case wui::event_type::system:
                switch (e.system_event_.type)
                {
                    case wui::system_event_type::device_connected:
#ifdef _WIN32
                        OutputDebugStringA("connect device: ");
                        OutputDebugStringA(to_string(e.system_event_.device).data());
                        OutputDebugStringA("\n");
#elif __linux__
                        printf("connect device: %s\n", to_string(e.system_event_.device).data());
#endif
                    break;
                    case wui::system_event_type::device_disconnected:
#ifdef _WIN32
                        OutputDebugStringA("disconnect device: ");
                        OutputDebugStringA(to_string(e.system_event_.device).data());
                        OutputDebugStringA("\n");
#elif __linux__
                        printf("disconnect device: %s\n", to_string(e.system_event_.device).data());
#endif
                    break;
                }
            break;
            }
        }, wui::event_type::internal | wui::event_type::system);

        Plug();
        plugged = true;
        Init();
    }

    void draw_list_item(wui::graphic &gr, const int32_t nItem, const wui::rect &itemRect, const wui::list::item_state state)
    {
        const auto& td = list3->get_theme_data();
        const auto border_width = td.border_width;

        if (state == wui::list::item_state::active)
        {
            gr.draw_rect(itemRect, td.active_item);
        }
        else if (state == wui::list::item_state::selected)
        {
            gr.draw_rect(itemRect, td.selected_item);
        }

        auto textColor = wui::theme_color(wui::input::tc, wui::input::tv_text);

        auto textRect = itemRect;
        const auto shift_y = (itemRect.height() - td.font_.size) / 2 - border_width;
        textRect.move(list3->get_left_position_text(0), shift_y);
        gr.draw_text(textRect, std::to_string(nItem + 1), textColor, td.font_);

        textRect = itemRect;
        textRect.move(list3->get_left_position_text(1), shift_y);
        gr.draw_text(textRect, "Item ", textColor, td.font_);

        textRect = itemRect;
        textRect.move(list3->get_left_position_text(2), shift_y);
        gr.draw_text(textRect, "right click", textColor, td.font_);
    }
};

#ifdef _WIN32
int APIENTRY _tWinMain(_In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPTSTR lpCmdLine [[maybe_unused]],
    _In_ int nCmdShow [[maybe_unused]] )
#elif __linux__
int main(int argc, char *argv[])
#endif
{
    if (!wui::framework::init())
    {
        return -1;
    }
    wui::error err;

    wui::set_app_locales({
        { wui::locale_type::eng, "English", "res/en_locale.json", TXT_LOCALE_EN },
        { wui::locale_type::rus, "Русский", "res/ru_locale.json", TXT_LOCALE_RU },
        });

    auto current_locale = wui::get_default_system_locale();
    wui::set_current_app_locale(current_locale);

    wui::set_locale_from_type(current_locale, err);
    if (!err.is_ok())
    {
        std::cerr << err.str() << std::endl;
        return -1;
    }

    wui::set_app_themes({
        { "dark", "res/dark.json", TXT_DARK_THEME },
        { "light", "res/light.json", TXT_LIGHT_THEME }
        });

    auto current_theme = "dark";
    wui::set_current_app_theme(current_theme);
    wui::set_default_theme_from_name(current_theme, err);
    if (!err.is_ok())
    {
        std::cerr << err.str() << std::endl;
        return -1;
    }

    //NB: main thread
    auto window = std::make_shared<wui::window>();

    // The main window has not yet been initialized (context and graphics).
    // Font sizes and text length calculations are not available.

    // When the wui::internal_event_type::window_create event occurs, the following become available:
    // font size, text length calculations (get_preferred_size(), measure_text(), ...)

    auto menuImage1 = std::make_shared<wui::image>(IMG_ACCOUNT);
    auto menuImage2 = std::make_shared<wui::image>(IMG_SETTINGS);

    auto menu_select_text = std::make_shared<wui::text>("Menu select: ", wui::hori_alignment::left, wui::vert_alignment::center, "text");

    auto menu = std::make_shared<wui::menu>();
    menu->set_items({
        {{ 0, wui::menu_item_state::separator, "Menu name, 0", "", menuImage1 }},
            { 1, wui::menu_item_state::normal, "Expand me, 1", "", nullptr, {
                    { 11, wui::menu_item_state::normal, "Expanded, 1.1", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 12, wui::menu_item_state::normal, "Expand me, 1.2", "", nullptr, {
                            { 121, wui::menu_item_state::normal, "Expanded, 1.2.1", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                            { 122, wui::menu_item_state::normal, "Expanded, 1.2.2", "Shift+Del", menuImage2, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                            { 123, wui::menu_item_state::separator, "Expanded, 1.2.3", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                        }, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 13, wui::menu_item_state::normal, "Expanded, 1.3", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                }, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
            { 2, wui::menu_item_state::separator, "Expand me, 2", "Ctrl+Z", nullptr, {
                    { 21, wui::menu_item_state::normal, "Expanded, 2.1", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 22, wui::menu_item_state::normal, "Expanded, 2.2", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 23, wui::menu_item_state::normal, "Expanded, 2.3", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 24, wui::menu_item_state::normal, "Expanded, 2.4", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 25, wui::menu_item_state::normal, "Expanded, 2.5", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                    { 26, wui::menu_item_state::separator, "Expanded, 2.6", "", nullptr, {}, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu: ") + std::to_string(i)); } },
                }, [&menu_select_text](int32_t i) { menu_select_text->set_text(std::string("Menu select: ") + std::to_string(i)); } },
            { 3, wui::menu_item_state::normal, "Exit, 3", "Alt+F4", nullptr, {},
            [&window](int32_t ) {
                window->close();
            } }
        });

    window->add_control(menu, { 0 });

    // button_view::image_menu : no border if control is not focused or active
    auto menuButton = std::make_shared<wui::button>("Settings", []()
        {}, wui::button_view::image_menu, IMG_SETTINGS, 32, "button_menu");

    menuButton->set_callback([&menu, &menuButton]()
        {
            if (menu->is_showed())
                menu->hide();
            else
                menu->show_on_control(menuButton, 5);
        });

    window->add_control(menuButton, {});

    auto horizProgressBar = std::make_shared<wui::progress>(0, 100, 50);
    window->add_control(horizProgressBar, {});

    auto horizSlider = std::make_shared<wui::slider>(0, 100, 50,
        [&horizProgressBar](int32_t value) { horizProgressBar->set_value(value); },
        wui::slider_orientation::horizontal);
    window->add_control(horizSlider, {});

    auto vertProgressBar = std::make_shared<wui::progress>(0, 100, 80, wui::orientation::vertical);
    auto vertSlider = std::make_shared<wui::slider>(0, 100, 80, [&vertProgressBar](int32_t value) { vertProgressBar->set_value(value); }, wui::slider_orientation::vertical);

    auto accountImage = std::make_shared<wui::image>(IMG_ACCOUNT);
    window->add_control(accountImage, {});

    auto vertSplitter = std::make_shared<wui::splitter>(wui::splitter::orientation::vertical, nullptr);

    auto panel2 = std::make_shared<wui::panel>();

    auto list2 = std::make_shared<wui::list>();
    list2->set_item_height_callback([](int32_t , int32_t& h) { h = 24; });
    list2->set_item_count(5);
    list2->select_item(0);
    list2->set_draw_callback([&list2](wui::graphic& gr, int32_t nItem, const wui::rect& itemRect, wui::list::item_state state)
        {
            const auto& td = list2->get_theme_data();
            const auto border_width = td.border_width;

            if (state == wui::list::item_state::active)
            {
                gr.draw_rect(itemRect, td.active_item);
            }
            else if (state == wui::list::item_state::selected)
            {
                gr.draw_rect(itemRect, td.selected_item);
            }

            auto textColor = wui::theme_color(wui::input::tc, wui::input::tv_text);

            auto textRect = itemRect;
            const auto shift_y = (itemRect.height() - td.font_.size) / 2 - border_width;
            textRect.move(list2->get_left_position_text(0), shift_y);
            gr.draw_text(textRect, std::to_string(nItem + 1), textColor, td.font_);

            textRect = itemRect;
            textRect.move(list2->get_left_position_text(1), shift_y);
            gr.draw_text(textRect, "******" , textColor, td.font_);
        });


    auto pluggedWindow = std::make_shared<PluggedWindow>(window, vertSplitter);
    pluggedWindow->Create();

    auto createPluggedButton = std::make_shared<wui::button>("Create plugged window", nullptr);
    auto closePluggedButton = std::make_shared<wui::button>("Close plugged window", nullptr);

    createPluggedButton->set_callback([&]()
        {
            createPluggedButton->disable();
            closePluggedButton->enable();
            pluggedWindow = std::make_shared<PluggedWindow>(window, vertSplitter);
            pluggedWindow->Create();
        });
    createPluggedButton->disable();

    closePluggedButton->set_callback([&]()
        {
            if (pluggedWindow)
            {
                createPluggedButton->enable();
                closePluggedButton->disable();
                pluggedWindow->window->close();
            }
        });

    window->add_control(createPluggedButton, {});
    window->add_control(closePluggedButton, {});

    auto text0 = std::make_shared<wui::text>(
        "Высокий уровень вовлечения представителей целевой аудитории является "
        "четким доказательством простого факта: граница обучения кадров "
        "создаёт предпосылки для новых предложений. Однозначно, "
        "непосредственные участники технического прогресса, превозмогая "
        "сложившуюся непростую экономическую ситуацию, превращены в "
        "посмешище, хотя само их существование приносит несомненную "
        "пользу обществу. А ещё базовые сценарии поведения пользователей, "
        "превозмогая сложившуюся непростую экономическую ситуацию, "
        "ограничены исключительно образом.");
    window->add_control(text0, {});

    auto nameInput = std::make_shared<wui::input>(/*"", wui::input_view::password*/);
    nameInput->set_text("Hello world!");
    //nameInput->set_input_content(wui::input_content::numeric);
    //nameInput->set_symbols_limit(20);
    //nameInput->set_input_view(wui::input_view::readonly);
    window->add_control(nameInput, {});

    auto select3 = std::make_shared<wui::select>();
    select3->set_items({
            { 0, "Item 0" },
            { 1, "Item 1" },
            { 2, "Item 2" },
            { 3, "Item 3" },
            { 4, "Item 4" },
            { 5, "Item 5" }
        });
    select3->select_item_number(0);

    window->add_control(select3, {});

    /*std::thread t([select3, window]() {
        bool has = false;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (!has)
            {
                window->add_control(select3, { 320, 300, 890, 325 });
            }
            else
            {
                window->remove_control(select3);
            }
            has = !has;
        }
    });
    t.detach();*/

    auto editor = std::make_shared<wui::input>("", wui::input_view::multiline);

    // Setup input multiline:
    //editor->set_symbols_limit(-1);
    window->set_focus_mode(wui::focus_mode::react); // restore input focus for this
    //window->set_focus_mode(wui::focus_mode::always); // always restore input focus for this

    window->add_control(editor, {});

    editor->set_text<false, false>("Текстовый редактор\n\n"
        "Мы вынуждены отталкиваться от того, что дальнейшее развитие различных \n"
        "форм деятельности выявляет срочную потребность вывода текущих активов.\n"
        "Сложно сказать, почему элементы политического процесса набирают популярность\n"
        "среди определенных слоев населения, а значит, должны быть объединены в целые кластеры себе подобных.\n\n"
        "Но акционеры крупнейших компаний лишь добавляют фракционных разногласий и заблокированы в рамках\n"
        "своих собственных рациональных ограничений.");

    //editor->scroll_to_end();

    auto dialog = std::make_shared<wui::window>();
    auto messageBox = std::make_shared<wui::message>(dialog);
    auto dialogMsgButton = std::make_shared<wui::button>("Test message",
        [&]()
        {
            messageBox->show("Test message",
                "message box", wui::message_icon::information,
                wui::message_button::ok);
        });
    auto dialogCloseButton = std::make_shared<wui::button>("Close",
        [&dialog]()
        {
            dialog->close();
        });
    auto text_account = std::make_shared<wui::text_ex>("ACCOUNT:");
    auto text_phone = std::make_shared<wui::text>("Password:");

    auto input1 = std::make_shared<wui::input>();
    auto input2 = std::make_shared<wui::input>("0A8T652A8-9879", wui::input_view::password);
    auto select1 = std::make_shared<wui::select>();
    auto list1 = std::make_shared<wui::list>();

    list1->update_columns({ { 80, "Account" }, { 80, "Your Phone" } });
    std::vector<std::vector<std::string>> list1_rows;

    list1->set_draw_callback(
        [&list1_rows, &list1](wui::graphic& gr, int32_t nItem, const wui::rect& itemRect, wui::list::item_state state)
        {
            if (nItem >= list1_rows.size())
                return;
            const auto& td = list1->get_theme_data();
            const auto border_width = td.border_width;

            if (state == wui::list::item_state::active)
            {
                gr.draw_rect(itemRect, td.active_item);
            }
            else if (state == wui::list::item_state::selected)
            {
                gr.draw_rect(itemRect, td.selected_item);
            }

            auto textColor = wui::theme_color(wui::input::tc, wui::input::tv_text);

            if (!list1_rows.empty() && !list1_rows[nItem].empty())
            {
                const auto shift_y = (itemRect.height() - td.font_.size) / 2 - border_width;
                auto textRect = itemRect;
                textRect.move(list1->get_left_position_text(0), shift_y);
                gr.draw_text(textRect, list1_rows[nItem][0], textColor, td.font_);

                if (2 == list1_rows[nItem].size())
                {
                    textRect = itemRect;
                    textRect.move(list1->get_left_position_text(1), shift_y);
                    gr.draw_text(textRect, list1_rows[nItem][1], textColor, td.font_);
                }
            }
        }
    );

    auto list1_item_size = list1->get_font_size();
    list1->set_item_height_callback([list1_item_size](int32_t , int32_t& h)
        { h = list1_item_size + 2; });

    auto editButton = std::make_shared<wui::button>("Edit",
        [&]()
    {
        dialog->subscribe(
            [&](const wui::event& e)
            {
                if (e.type & wui::event_type::internal) {
                    switch (e.internal_event_.type) {
                        case wui::internal_event_type::window_created:
                        {
                            // do not create temporary ui control objects
                            constexpr int32_t space_w = 30;
                            constexpr int32_t space_h = 10;

                            list1_item_size = list1->get_font_size();
                            list1->set_item_height_callback([list1_item_size](int32_t , int32_t& h)
                                { h = list1_item_size + 2; });

                            const wui::rect r1 = dialogMsgButton->get_preferred_size();
                            const wui::rect r2 = dialogCloseButton->get_preferred_size();
                            const int32_t ctrl_width = 40 + (r1.width() + space_w + r2.width()); // 210

                            int32_t top = dialog->caption_height();
                            top += space_h;
                            wui::rect r = text_account->get_preferred_size();
                            int32_t hc = r.height() + 8;
                            dialog->add_control(text_account, { space_w, top, space_w + r.width(), top + hc });
                            top += hc + 3;
                            hc = input1->get_font_size() + 8;
                            dialog->add_control(input1, { space_w, top, space_w + ctrl_width, top + hc });
                            top += hc + space_h;

                            hc = text_phone->get_font_size() + 8;
                            dialog->add_control(text_phone, { space_w, top, space_w + ctrl_width, top + hc });
                            top += hc + 3;

                            hc = input2->get_font_size() + 8;
                            dialog->add_control(input2, { space_w, top, space_w + ctrl_width, top + hc });
                            top += hc + space_h;

                            wui::select_items_t items = {
                                { 1, "+1 128 222 3334" }, { 2, "+1 456" },
                                { 3, "+1 789" }, { 4, "+1 101 112" },
                                { 5, "+1 131 415" }, { 6, "+1 161 718" },
                                { 7, "+1 192 021" }, { 8, "+1 222 324" }
                            };
                            select1->set_items(items);
                            //select1->select_item_number(0);
                            select1->set_change_callback(
                                [&](int32_t n_item [[maybe_unused]], int64_t id [[maybe_unused]] )
                                {
                                    const std::string imput_text = input1->text();
                                    const auto item = select1->selected_item();

                                    if (-1 != item.id && !imput_text.empty() && !item.text.empty()
                                        && list1_rows.end() == std::find_if(list1_rows.begin(), list1_rows.end(),
                                            [&imput_text, &item](const auto& c)
                                            {
                                                return c[0] == imput_text && c[1] == item.text;
                                            }
                                        ))
                                    {
                                        list1_rows.push_back({});
                                        list1_rows[list1_rows.size() - 1].emplace_back(imput_text);
                                        list1_rows[list1_rows.size() - 1].emplace_back(item.text);
                                        list1->set_item_count(static_cast<int32_t>(list1_rows.size()));
                                    }
                                }
                            );

                            hc = select1->get_font_size() + 8;
                            dialog->add_control(select1, { space_w, top, space_w + ctrl_width, top + hc });
                            top += hc + space_h;

                            hc = 10 * list1->get_font_size() + 8;
                            dialog->add_control(list1, { space_w, top, space_w + ctrl_width, top + hc });
                            top += hc + space_h;

                            const int32_t hc_but = std::max(r1.height(), r2.height());
                            const int32_t width = 2 * space_w + ctrl_width;
                            const int32_t left_but = (width - (r.width() + space_w + r.width())) / 2;

                            dialog->add_control(dialogMsgButton, { left_but, top, left_but + r1.width(), top + hc_but });
                            dialog->add_control(dialogCloseButton,
                                { left_but + r1.width() + space_w, top, left_but + r1.width() + space_w + r2.width(), top + hc_but });

                            // NB: key 'return' close dialog, bad idea if this dialog used input control.
                            // dialog->set_default_push_control(dialogCloseButton);

                            const auto height = top + hc_but + space_h;

                            const auto caption = dialog->docked() ?
                                "Transient, docked, modal" : "Transient, dialog, modal";
                            dialog->set_caption(caption);
                            dialog->set_tw_preferred_position(width, height);
                        }
                        break;
                    }
                }
            }, wui::event_type::internal);

        dialog->set_transient_for(window);
        dialog->init("", { 0, 0, 270, 430 }, wui::window_style::dialog);
    });

    auto exitButton = std::make_shared<wui::button>("Exit",
        [window]() {
            window->close();
        },
        wui::button_view::image_right_text, IMG_ACCOUNT_RED, 24, wui::button::tc, MakeRedButtonTheme(true));

    auto darkThemeButton = std::make_shared<wui::button>("The dark theme", nullptr);
    auto whiteThemeButton = std::make_shared<wui::button>("The light theme", nullptr);

    darkThemeButton->set_callback(
        [&]()
        {
            const auto current_theme = "dark";
            wui::set_current_app_theme(current_theme);
            wui::error err;
            wui::set_default_theme_from_name(current_theme, err);
            if (!err.is_ok())
            {
                std::cerr << err.str() << std::endl;
                return;
            }

            darkThemeButton->turn(true);
            whiteThemeButton->turn(false);

            window->update_theme();
            window->set_button_next_theme();
            pluggedWindow->window->update_theme();
            dialog->update_theme();
            exitButton->update_theme(MakeRedButtonTheme(true));
        }
    );
    window->add_control(darkThemeButton, {});

    whiteThemeButton->set_callback(
        [&]()
        {
            const auto current_theme = "light";
            wui::set_current_app_theme(current_theme);
            wui::error err;
            wui::set_default_theme_from_name(current_theme, err);
            if (!err.is_ok())
            {
                std::cerr << err.str() << std::endl;
                return;
            }

            darkThemeButton->turn(false);
            whiteThemeButton->turn(true);
            window->update_theme();
            window->set_button_next_theme();

            pluggedWindow->window->update_theme();
            dialog->update_theme();
            exitButton->update_theme(MakeRedButtonTheme(false));
        });

    window->add_control(whiteThemeButton, {});

    window->add_control(menu_select_text, {});

    window->add_control(editButton, {});
    window->add_control(exitButton, {});

    vertSplitter->set_callback_ex([&](const wui::rect& act_pos, const wui::rect& prev_pos)
        {
            if (pluggedWindow->plugged)
            {
                const auto pos = pluggedWindow->window->position();
                pluggedWindow->window->set_position({ 0, pos.top, act_pos.left, pos.bottom });
            }
            const auto dx = act_pos.left - prev_pos.left;
            auto pos = createPluggedButton->position();
            auto left = dx + pos.left;
            createPluggedButton->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            left = dx + left + pos.width() + 4;
            pos = closePluggedButton->position();
            closePluggedButton->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = horizProgressBar->position();
            left = dx + pos.left;
            horizProgressBar->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = horizSlider->position();
            left = dx + pos.left;
            horizSlider->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = vertSlider->position();
            left = dx + pos.left;
            vertSlider->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = vertProgressBar->position();
            left = dx + pos.left;
            vertProgressBar->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = panel2->position();
            left = dx + pos.left;
            panel2->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = list2->position();
            left = dx + pos.left;
            list2->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = accountImage->position();
            left = dx + pos.left;
            accountImage->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = text0->position();
            left = dx + pos.left;
            text0->set_position({ left, pos.top, pos.right, pos.bottom });

            pos = nameInput->position();
            left = dx + pos.left;
            nameInput->set_position({ left, pos.top, pos.right, pos.bottom });

            pos = select3->position();
            left = dx + pos.left;
            select3->set_position({ left, pos.top, pos.right, pos.bottom });

            pos = darkThemeButton->position();
            left = dx + pos.left;
            darkThemeButton->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = whiteThemeButton->position();
            left = dx + pos.left;
            whiteThemeButton->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            pos = editor->position();
            left = dx + pos.left;
            editor->set_position({ left, pos.top, pos.right, pos.bottom });

            pos = menu_select_text->position();
            left = dx + pos.left;
            menu_select_text->set_position({ left, pos.top, left + pos.width(), pos.bottom });

            auto wp = window->position();
            window->redraw({ 0, 0, wp.right, wp.bottom }, true);
        });
    window->add_control(vertSplitter, { 0 });

    auto sid = window->subscribe([&](const wui::event &e) {
        // The main window is already initialized (context and graphics).
        // Font size and text length calculations are available for add_control(), etc (get_preferred_size(), measure_text(), ...)
        if (e.internal_event_.type == wui::internal_event_type::size_changed
            || e.internal_event_.type == wui::internal_event_type::window_expanded)
        {
            const int32_t w = e.internal_event_.x;
            const int32_t h = e.internal_event_.y;

            const auto cap_h = window->caption_height();
            if (pluggedWindow->plugged)
            {
                auto pos = pluggedWindow->window->position();
                if (pos.height() != h)
                {
                    pluggedWindow->window->set_position({ 0, cap_h, pos.width(), h });
                    vertSplitter->set_position({ pos.width(), cap_h, pos.width() + _splitter_width, h });
                }
            }

            vertSplitter->set_margins(150, w - w/2);

            const auto rect_mb = menuButton->get_preferred_size();
            menuButton->set_position({ w - 10 - rect_mb.width(), cap_h, w - 10, cap_h + rect_mb.height() });
            menu->set_position({});
            constexpr int32_t space = 20;

            auto pos_splitter = vertSplitter->position();
            const int32_t left = pos_splitter.left + pos_splitter.width() + space;

            wui::rect pos = text0->position();
            text0->set_position({ left, pos.top, w - 10, pos.bottom });

            pos = nameInput->position();
            nameInput->set_position({ left, pos.top, w - 10, pos.bottom });

            pos = select3->position();
            select3->set_position({ left, pos.top, w - 10, pos.bottom });

            editor->set_position({ left, 400, w - 10, h - 60 });

            menu_select_text->set_position({ left, h - 55, left + 120, h - 20 }); //w - 260

            editButton->set_position({ w - 250, h - 55, w - 150, h - 20 });
            exitButton->set_position({ w - 120, h - 55, w - 20, h - 20 });
        }
        else if (e.internal_event_.type == wui::internal_event_type::window_created)
        {
            // linux: X11 thread
            accountImage->set_position( { 350, 100, 414, 164 });

            int32_t h_text = text0->get_font_size() + 8;
            text0->set_position( { 320, 190, 890, 190 + h_text });
            int32_t h_ = 180 + h_text + 30;

            h_text = nameInput->get_font_size() + 8;
            nameInput->set_position( { 320, h_, 890, h_ + h_text });
            h_ = h_ + h_text + 30;

            select3->set_position( { 320, h_, 890, h_ + 25 });

            editor->set_position( { 320, 400, 890, 500 });

            darkThemeButton->set_position( { 320, 350, 320 + 120, 375 });
            darkThemeButton->turn(true);

            whiteThemeButton->set_position( { 440 + 20, 350, 440 + 20 + 120, 375 });

            menu_select_text->set_position( { 120, 450, 200, 480 });

            editButton->set_position( { 240, 450, 350, 480 });
            exitButton->set_position( { 370, 450, 480, 480 });

            const auto cap_h = window->caption_height();
            const auto rect1 = createPluggedButton->get_preferred_size();
            createPluggedButton->set_position( { 320,
                cap_h + 24, 320 + rect1.width(), cap_h + 24 + rect1.height() + 4 });

            auto rect2 = closePluggedButton->get_preferred_size();
            auto left = 320 + rect1.width() + 4;
            auto x_pos = left + rect2.width();
            closePluggedButton->set_position( { left, cap_h + 24, x_pos,
                cap_h + 24 + rect2.height() + 4 });

            horizProgressBar->set_position({ left, 100, left + rect2.width(), 125 });
            horizSlider->set_position({ left, 140, left + rect2.width(), 165 });

            x_pos += 10;
            window->add_control(vertSlider,      { x_pos, cap_h + 24, x_pos + 25, 155 });
            x_pos += 40;
            window->add_control(vertProgressBar, { x_pos, cap_h + 24, x_pos + 25, 155 });
            x_pos += 25;

            if (0 == (wui::focus_mode::always & window->get_focus_mode()))
            {
                list2->update_columns({ { 30, "##" }, { 62, "day" } });
                rect2 = list2->get_preferred_size(195 - cap_h - 24); // 0: height max

                x_pos += 18;
                window->add_control(panel2, { x_pos - 4, cap_h + 24 - 4,
                    x_pos + rect2.width() + 4, cap_h + 24 + rect2.height() + 4 });

                window->add_control(list2, { x_pos, cap_h + 24,
                    x_pos + rect2.width(), cap_h + 24 + rect2.height() });
                x_pos += rect2.width();
            }

            const auto rect_mb = menuButton->get_preferred_size();
            x_pos += rect_mb.width() + 2*10;

            const auto rc = window->position();
            if (x_pos > rc.width())
            {
                window->set_position({ rc.left, rc.top, rc.left + x_pos, rc.bottom });
            }
        }
        else if (e.internal_event_.type == wui::internal_event_type::user_emitted)
        {
            if (e.internal_event_.x == 5555)
            {
                if (0 == e.internal_event_.y)
                {
                    createPluggedButton->enable(); // call only if destroy window
                    closePluggedButton->disable();
                }
                else
                {
                    if (1 == e.internal_event_.y)
                        closePluggedButton->disable();
                    else
                        closePluggedButton->enable();
                }
            }
        }
    }, wui::event_type::internal);

    window->set_control_callback([&](wui::window_control control,
        std::string &tooltip_text, bool continue_ [[maybe_unused]]) {
        if (control == wui::window_control::theme)
        {
            auto theme_name = wui::get_default_theme()->get_name();

            tooltip_text = wui::locale("window", theme_name == "dark" ? "dark_theme" : "light_theme");

            auto current_theme = theme_name == "dark" ? "light" : "dark";
            wui::set_current_app_theme(current_theme);
            wui::error err;
            wui::set_default_theme_from_name(current_theme, err);
            if (!err.is_ok())
            {
                std::cerr << err.str() << std::endl;
            }
            if (theme_name == "dark")
            {
                darkThemeButton->turn(false);
                whiteThemeButton->turn(true);
            }
            else
            {
                darkThemeButton->turn(true);
                whiteThemeButton->turn(false);
            }

            window->update_theme();
            pluggedWindow->window->update_theme();
            dialog->update_theme();
            exitButton->update_theme(MakeRedButtonTheme(theme_name == "dark"));
        }
    });

    window->set_min_size(WND_WIDTH - WND_WIDTH/4, WND_HEIGHT - WND_HEIGHT/4);

    window->init("Hello from WUI!", { -1, -1, WND_WIDTH, WND_HEIGHT },
        main_window_style, [window]() {});

    window->enable_device_change_handling(true);

    wui::framework::run();

    return 0;
}
