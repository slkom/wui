//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#ifdef _WIN32

#include <wui/config/i_config.hpp>

#include <windows.h>

namespace wui
{

namespace config
{

class config_impl_reg : public i_config
{
public:
    explicit config_impl_reg(std::string_view base_application_key, HKEY root = HKEY_CURRENT_USER);
    virtual ~config_impl_reg() override;

    [[nodiscard]] int32_t get_int(std::string_view section, std::string_view entry, int32_t default_) override;
    void set_int(std::string_view section, std::string_view entry, int32_t value) override;

    [[nodiscard]] int64_t get_int64(std::string_view section, std::string_view entry, int64_t default_) override;
    void set_int64(std::string_view section, std::string_view entry, int64_t value) override;

    [[nodiscard]] std::string get_string(std::string_view section, std::string_view entry, std::string_view default_) override;
    void set_string(std::string_view section, std::string_view entry, std::string_view value) override;

    void delete_value(std::string_view section, std::string_view entry) override;

    void delete_key(std::string_view section);

    [[nodiscard]] error get_error() override;

private:
    std::string base_application_key;
    HKEY root;
};

} // config

}

#endif
