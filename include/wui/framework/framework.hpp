//
// framework.hpp - Contains the ui starting / stoping interface
//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/common/error.hpp>
#include <functional>

namespace wui
{

namespace framework
{

[[nodiscard]] bool init();

int run(std::function<bool()> task = nullptr);
void stop();
[[nodiscard]] bool started();

[[nodiscard]] error get_error();

}

}
