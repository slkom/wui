//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <cstdint>
#include <array>

namespace wui
{

enum class keyboard_event_type
{
    down,
    up,
    key
};

#ifdef _WIN32
static constexpr uint8_t vk_tab = 0x09;
static constexpr uint8_t vk_return = 0x0D;
static constexpr uint8_t vk_rreturn = 0;
static constexpr uint8_t vk_esc = 0x1B;

static constexpr uint8_t vk_back = 0x08;
static constexpr uint8_t vk_del = 0x2E;

static constexpr uint8_t vk_end = 0x23;
static constexpr uint8_t vk_nend = 0x61;
static constexpr uint8_t vk_home = 0x24;
static constexpr uint8_t vk_nhome = 0x67;
static constexpr uint8_t vk_page_up = 0x21;
static constexpr uint8_t vk_npage_up = 0x69;
static constexpr uint8_t vk_page_down = 0x22;
static constexpr uint8_t vk_npage_down = 0x63;

static constexpr uint8_t vk_up = 0x26;
static constexpr uint8_t vk_nup = 0x68;
static constexpr uint8_t vk_down = 0x28;
static constexpr uint8_t vk_ndown = 0x62;
static constexpr uint8_t vk_left = 0x25;
static constexpr uint8_t vk_nleft = 0x64;
static constexpr uint8_t vk_right = 0x27;
static constexpr uint8_t vk_nright = 0x66;

/// The modifier keys
// TODO: модификаторы могут быть нажаты одновременно,
// нужны бинарные флаги
static constexpr uint8_t vk_capital = 0x14;
static constexpr uint8_t vk_lshift = 0x10;
static constexpr uint8_t vk_rshift = 0x11;
static constexpr uint8_t vk_alt = 0x12;
static constexpr uint8_t vk_lcontrol = 0xA2;
static constexpr uint8_t vk_rcontrol = 0xA3;
static constexpr uint8_t vk_insert = 0x2D;
static constexpr uint8_t vk_numlock = 0x90;
#elif __linux__
static constexpr uint8_t vk_tab = 0x17;
static constexpr uint8_t vk_return = 0x24;
static constexpr uint8_t vk_rreturn = 0x68;
static constexpr uint8_t vk_esc = 0x09;

static constexpr uint8_t vk_back = 0x16;
static constexpr uint8_t vk_del = 0x77;

static constexpr uint8_t vk_end = 0x73;
static constexpr uint8_t vk_nend = 0x57;
static constexpr uint8_t vk_home = 0x6E;
static constexpr uint8_t vk_nhome = 0x4F;
static constexpr uint8_t vk_page_up = 0x70;
static constexpr uint8_t vk_npage_up = 0x51;
static constexpr uint8_t vk_page_down = 0x75;
static constexpr uint8_t vk_npage_down = 0x59;

static constexpr uint8_t vk_up = 0x6F;
static constexpr uint8_t vk_nup = 0x50;
static constexpr uint8_t vk_down = 0x74;
static constexpr uint8_t vk_ndown = 0x58;
static constexpr uint8_t vk_left = 0x71;
static constexpr uint8_t vk_nleft = 0x53;
static constexpr uint8_t vk_right = 0x72;
static constexpr uint8_t vk_nright = 0x55;

/// The modifier keys
// TODO: модификаторы могут быть нажаты одновременно,
// нужны бинарные флаги
static constexpr uint8_t vk_capital = 0x02;
static constexpr uint8_t vk_lshift = 0x32;
static constexpr uint8_t vk_rshift = 0x3E;
static constexpr uint8_t vk_alt = 0x40;
static constexpr uint8_t vk_lcontrol = 0x25;
static constexpr uint8_t vk_rcontrol = 0x69;
static constexpr uint8_t vk_insert = 0x76;
static constexpr uint8_t vk_numlock = 0x4D;

#endif

// TODO: F1-F12

// TODO
static constexpr uint32_t vk_flag_capital = 1;
static constexpr uint32_t vk_flag_lshift = 1 << 1;
static constexpr uint32_t vk_flag_rshift = 1 << 2;
static constexpr uint32_t vk_flag_alt = 1 << 3;
static constexpr uint32_t vk_flag_lcontrol = 1 << 4;
static constexpr uint32_t vk_flag_rcontrol = 1 << 5;
static constexpr uint32_t vk_flag_insert = 1 << 6;
static constexpr uint32_t vk_flag_numlock = 1 << 7;

struct keyboard_event
{
    keyboard_event_type type;

    uint8_t modifier; /// vk_capital or vk_shift or vk_alt or vk_insert
    char key[4];
    uint8_t key_size;
};

}
