//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/window/i_window.hpp>
#include <wui/system/system_context.hpp>
#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/common/rect.hpp>

#include <limits>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

namespace wui
{

/// window geometry state
enum class window_geometry_state
{
    normal,
    minimized, /// non-geometric state, left for compatibility old wui, for create window
    maximized,
    fullscreen
};

struct window_state_current
{
    window_geometry_state prev_geometry{ window_geometry_state::normal }; // (for maximized/fullscreen restore state) old geometric state, exclude minimized
    window_geometry_state geometry{ window_geometry_state::normal }; // geometric state, exclude minimized
    bool is_iconic{ false }; // eq. minimized (X11: IconicState : ICCCM)
    bool is_hidden{ false }; // (X11: EWMH _NET_WM_STATE_HIDDEN)
    bool focused{ false };
    //bool is_pinned{ false }; // for future: _NET_WM_STATE_ABOVE / _NET_WM_STATE_BELOW  ?
    //bool active{ false };
    //bool monitor_power{ true };

    void set_geometry(const window_geometry_state state) noexcept
    {
        if (window_geometry_state::minimized == state)
        {
            is_iconic = true;
            return; // non-geometric state
        }

        prev_geometry = geometry;
        if (window_geometry_state::maximized != state
            && window_geometry_state::fullscreen != state)
        {
            geometry = window_geometry_state::normal;
            //is_iconic = false;
        }
        else
        {
            geometry = state;
        }
    }

    void set_iconic(const bool iconic) noexcept
    {
        is_iconic = iconic;
    }

    void set_hidden(const bool hidden) noexcept
    {
        is_hidden = hidden;
    }
};

using window_state = window_geometry_state; // left for compatibility old wui

enum class window_control
{
    close,
    state,
    pin,
    theme,
    lang
};

class button;
class udev_handler;
class listener;

class window : public i_window, public i_control, public std::enable_shared_from_this<window>
{
    friend listener;
public:
    explicit window(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr,
        window_geometry_state state = window_geometry_state::normal);
    ~window() override;

    /// i_window impl
    bool init(std::string_view caption, const rect position,
        window_style style, std::function<void(void)> close_callback = nullptr) override;
    void destroy() override;

    void add_control(std::shared_ptr<i_control> control, const rect position) override;
    void remove_control(std::shared_ptr<i_control> control) override;

    void bring_to_front(std::shared_ptr<i_control> control) override;
    void move_to_back(std::shared_ptr<i_control> control) override;

    ///NB: To fully refresh the child window, use i_control::redraw()
    void redraw(const rect position, bool clear = false) override;

    std::string subscribe(std::function<void(const event&)> receive_callback,
        event_type event_types, std::shared_ptr<i_control> control = nullptr) override;
    void unsubscribe(std::string_view subscriber_id) override;

    /// Get the context of this window or parent's.
    [[nodiscard]] system_context &context() override;

    /// i_control impl

    /// drawing the child window
    void draw(graphic &gr, const rect paint_rect) override;

    /// The method uses an extended child window border
    void redraw([[maybe_unused]] bool clear = false ) override;

    void set_position(const rect position) override;
    [[nodiscard]] rect position() const override;
    void move(const int32_t dx, const int32_t dy) override;

    void set_parent(std::shared_ptr<window> window_) override;
    [[nodiscard]] std::weak_ptr<window> parent() const override;
    void clear_parent() override;

    void set_topmost(bool yes) override;
    [[nodiscard]] bool topmost() const override;

    [[nodiscard]] bool focused() const override;    // only ctrl
    [[nodiscard]] bool focusing() const override;   // only ctrl
    [[nodiscard]] focus_mode get_focus_mode() const override;

    void update_theme_control_name(std::string_view theme_control_name) override;
    void update_theme(std::shared_ptr<i_theme> theme_ = nullptr) override;

    void show() override;
    void hide() override;
    [[nodiscard]] bool showed() const override;

    void enable() override;
    void disable() override;
    [[nodiscard]] bool enabled() const override;

    [[nodiscard]] error get_error() override;


    /// Window style methods
    [[nodiscard]] int32_t caption_height(const window_style create_style = window_style::frame) const;

    // [[nodiscard]] static rect get_dispay_position() const; // TODO

    void set_caption(std::string_view caption);
    void set_style(window_style style);
    void set_min_size(int32_t width, int32_t height);

    /// <summary>
    /// Set window preferred position use transient window position
    /// </summary>
    /// <param name="width__"></param>
    /// <param name="height__"></param>
    void set_tw_preferred_position(const int32_t width__, const int32_t height__);

    // Set parent window for this transient window
    void set_transient_for(std::shared_ptr<window> window_, bool docked = true);

    /// Window state methods
    void switch_lang();
    void switch_theme();
    void set_button_next_theme();
    void pin();
    void minimize();
    void expand();
    void normal();

    // The "minimized" state is left for compatibility
    // EWMH:: Состояние iconic (свернутое в панель задач / иконку) концептуально ортогонально
    // (не зависит) от геометрических состояний вроде maximized или fullscreen.
    [[nodiscard]] window_state state() const noexcept
    {
        return (window_state_.is_iconic && window_state::normal != window_state_.geometry) ?
            window_state::minimized : window_state_.geometry;
    }

    [[nodiscard]] const window_state_current& state_current() const noexcept
    {
        return window_state_;
    }

    [[nodiscard]] bool is_iconic() const noexcept
    {
        return window_state_.is_iconic;
    }

    [[nodiscard]] bool is_visual() const noexcept
    {
        return !window_state_.is_hidden;
    }

    /// event loop for dialog, etc (see example hello_word)
    void event_loop();

    void close();

    /// Disabling draw for increase performance on mass operations
    void disable_draw();
    void enable_draw();
    [[nodiscard]] bool draw_enabled() const;

    /// Emit event methods
    void emit_event(int32_t x, int32_t y);

    /// Method to set the focus of the child control
    void set_focused(std::shared_ptr<i_control> control);

    /// Callbacks
    void set_control_callback(std::function<void(window_control control, std::string &text, bool &continue_)> callback);

    /// Set the control (button) to be pressed when user press enter on the keyboard
    void set_default_push_control(std::shared_ptr<i_control> control);

    /// Enable/disable the window to receive events about connecting / disconnecting devices (will be sent to system_event)
    /// Call this only after the window::init()
    void enable_device_change_handling(bool yes);

    /// Direct link to window graphic. Initialized (graphic&) if at least one window is created.
    [[nodiscard]] graphic& get_graphic()
    {
        return graphic_;
    }

    /// Get the type of this window
    [[nodiscard]] bool is_physical_window() const noexcept
    {
        return context_.valid();
    }

    [[nodiscard]] bool docked() const noexcept
    {
        return docked_;
    }

    void root_window(const bool root_wnd) noexcept
    {
        root_window_ = root_wnd;
    }

    [[nodiscard]] std::string_view get_control_name() const noexcept
    {
        return tcn; // string_view(), если lock() ?
    }
    [[nodiscard]] std::shared_ptr<i_theme> get_theme() const
    {
        return theme_;
    }

    void set_focus_mode(const focus_mode mode) noexcept
    {
        if (mode & focus_mode::always)
        {
            focus_mode_ = focus_mode::wnd | focus_mode::always;
            return;
        }
        if (mode & focus_mode::react)
        {
            focus_mode_ = focus_mode::wnd | focus_mode::react;
            return;
        }
        focus_mode_ = focus_mode::wnd | focus_mode::default_;
    }

    [[nodiscard]] rect get_parent_position();

    void set_scale(const double scale) noexcept
    {
        context_.scale = scale;
    }

    /// Control name in theme / locale
    static constexpr const char *tc = "window";

    /// Used theme values
    static constexpr const char *tv_background = "background";
    static constexpr const char *tv_border = "border";
    static constexpr const char *tv_border_focus = "border_focus";
    static constexpr const char *tv_round = "round";
    static constexpr const char *tv_border_width = "border_width";
    static constexpr const char *tv_text = "text";
    static constexpr const char *tv_caption_font = "caption_font";

    ///Used theme images
    static constexpr const char *ti_close = "window_close";
    static constexpr const char *ti_expand = "window_expand";
    static constexpr const char *ti_normal = "window_normal";
    static constexpr const char *ti_minimize = "window_minimize";
    static constexpr const char *ti_pin = "window_pin";
    static constexpr const char *ti_switch_theme = "window_switch_theme";
    static constexpr const char *ti_switch_lang = "window_switch_lang";

    /// Used locale values (from section window)
    static constexpr const char *cl_pin = "pin";
    static constexpr const char *cl_unpin = "unpin";
    static constexpr const char *cl_dark_theme = "dark_theme";
    static constexpr const char *cl_light_theme = "light_theme";
    static constexpr const char *cl_switch_lang = "switch_lang";

    static constexpr const int32_t _border_select_height = 2;
    static constexpr int32_t _btn_width = 42, _btn_height = 28;
    static constexpr int32_t _btn_width_child = 26, _btn_height_child = 12;
private:
    bool _destroy();

#if __linux__
    void destroy_window();
#endif

    system_context context_{};
    graphic graphic_;

    std::vector<std::shared_ptr<i_control>> controls;
    std::shared_ptr<i_control> active_control;
    std::shared_ptr<i_control> focused_control;
    std::shared_ptr<i_control> input_control;

    std::string caption;
    rect position_{}, parent_position_{}, normal_position{};
    rect caption_text_rect{};
    int32_t min_width{}, min_height{};
    window_style window_style_{ window_style::frame };
    window_state_current window_state_{};

    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    bool showed_{ true };
    bool enabled_{ true };

    /// This flag is set in the first window opened:
    /// assigned to the first opened window, used when closing all windows
    bool root_window_{ false };

    bool docked_{ false };
    bool docked_setup{ false }; // initially docked during setup
    bool disable_close_callback{ false };
    bool exit_{ false };

    bool skip_draw_{ false };

    focus_mode focus_mode_{ focus_mode::wnd | focus_mode::default_ };

    std::weak_ptr<window> parent_;
    std::weak_ptr<window> transient_window;
    std::shared_ptr<window> docked_control;
    std::shared_ptr<window> selected_child;

    std::string my_control_sid, my_plain_sid;

    struct event_subscriber
    {
        std::string id;
        std::function<void(const event&)> receive_callback;
        event_type event_types;
        std::shared_ptr<i_control> control;
    };
    std::vector<event_subscriber> subscribers_;
    std::mutex subscribers_mutex_;

    enum class moving_mode
    {
        none,
        move,
        size_we_left,
        size_we_right,
        size_ns_top,
        size_ns_bottom,
        size_nwse_top,
        size_nwse_bottom,
        size_nesw_top,
        size_nesw_bottom
    };
    moving_mode moving_mode_{ moving_mode::none };

    int16_t x_click{ 0 }, y_click{ 0 };

    error err;

    std::function<void(void)> close_callback;
    std::function<void(window_control control, std::string &text, bool &continue_)> control_callback;

    std::shared_ptr<i_control> default_push_control;

    std::shared_ptr<button> switch_lang_button, switch_theme_button, pin_button, minimize_button, expand_button, close_button;

#ifdef _WIN32

    bool mouse_tracked{ false };
    bool device_change_handling{ false };

    HDEVNOTIFY dev_notify_handle{ 0 };

    static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM w_param, LPARAM l_param);

    static void ProcessDeviceChanges(window* wnd, WPARAM w, LPARAM l);
    HPOWERNOTIFY hPowerNotify{ NULL };
#elif __linux__
    inline static bool xcb_atom_init{ false };

    inline static xcb_atom_t wm_protocols_event{ XCB_NONE },
        wm_delete_msg{ XCB_NONE },
        wm_change_state{ XCB_NONE },
        wm_state{ XCB_NONE },
        net_wm_state{ XCB_NONE },
        net_wm_state_focused{ XCB_NONE },
        net_wm_state_above{ XCB_NONE },
        net_wm_state_skip_taskbar{ XCB_NONE },
        net_wm_name{ XCB_NONE },
        utf8_string{ XCB_NONE },
        net_active_window{ XCB_NONE },
        net_wm_state_fullscreen{ XCB_NONE },
        net_wm_state_max_vert{ XCB_NONE }, net_wm_state_max_horz{ XCB_NONE },
        net_wm_state_hidden{ XCB_NONE },
        net_wm_moveresize{ XCB_NONE };

    time_t prev_button_click{ 0 };

    std::unique_ptr<udev_handler> udev_handler_;

    uint8_t key_modifier{ 0 };
    uint32_t key_sym{ 0 };

    void process_events(xcb_generic_event_t &e);

    void init_atoms();
    void set_size_hints();

    void send_destroy_event(const bool flush_ = true);

    void change_style(xcb_atom_t type, xcb_atom_t action, xcb_atom_t style1, xcb_atom_t style2=0) noexcept;

    void update_window_style();

    void update_window_state(const xcb_atom_t atom);

    void set_wm_name(std::string_view caption);

#endif
    void receive_control_events(const event &ev);
    void receive_plain_events(const event &ev);

    void send_event_to_control(const std::shared_ptr<i_control> &control, const event &ev);
    void send_event_to_plains(const event &ev);
    void send_event_to_plains_and_control(const event& ev, const std::shared_ptr<i_control>& control);

    void send_mouse_event(const mouse_event &ev);

    [[nodiscard]] bool check_control_here(int32_t x, int32_t y);

    void change_focus();
    void remove_focus();
    void selected_child_remove_focus();

    void execute_focused();
    void set_next_focused(const bool next=false);
    void set_input_focused();
    void setup_input_focused();
    std::shared_ptr<window> get_next_window();

    [[nodiscard]] std::shared_ptr<i_control> get_focused();

    void start_docking(std::shared_ptr<window> control);
    void end_docking();

    [[nodiscard]] std::shared_ptr<window> get_transient_window();

    void update_button_images();
    void update_buttons();

    void draw_border(graphic &gr, const color c, const int32_t bw);
    void draw_selected_border(graphic &gr, int32_t border_width);
    void draw_caption(graphic& gr, const rect paint_rect);

    void send_internal(internal_event_type type, int32_t x, int32_t y);

};

}
