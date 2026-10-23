//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/locale/i_locale.hpp>

#include <map>

namespace wui
{

class locale_impl : public i_locale
{
public:
    locale_impl(locale_type type, std::string_view name);

    [[nodiscard]] locale_type get_type() const override;
    [[nodiscard]] std::string get_name() const override;

    void set(std::string_view section, std::string_view value, std::string_view str) override;
    [[nodiscard]] const std::string &get(std::string_view section, std::string_view value) const override;

#ifdef _WIN32
    void load_resource(int32_t resource_index, std::string_view resource_section) override;
#endif
    void load_json(std::string_view json) override;
    void load_file(std::string_view file_name) override;
    void load_locale(const i_locale &locale_) override;

    [[nodiscard]] error get_error() override;

private:
    locale_type type;
    std::string name;

    std::map<std::pair<std::string, std::string>, std::string> strings;

    std::string dummy_string;

    error err;
};

}
