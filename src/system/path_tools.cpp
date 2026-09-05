//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/system/path_tools.hpp>

#ifndef _WIN32

#include <pwd.h>

#include <stdlib.h>

#include <unistd.h>
#include <filesystem>
#include <iostream>
#endif

namespace wui
{

std::string real_path(std::string_view relative_path)
{
#ifndef _WIN32
    auto index = relative_path.find("~/");
    if (index != std::string::npos)
    {
        const char *homedir = getenv("HOME");
        if (nullptr == homedir)
        {
            const struct passwd* pw = getpwuid(getuid());
            if (nullptr != pw)
                homedir = pw->pw_dir;
        }

        if (nullptr != homedir)
        {
            std::string new_path(relative_path.begin(), relative_path.end());

            new_path.replace(index, 1, homedir);
            return std::move(new_path);
        }
    }
#endif

    return relative_path.data();
}

}
