/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <chrono>
#include <iostream>
#include <string_view>

auto
my_locate_zone(std::string_view tz)
{
    try
    {
        return std::chrono::locate_zone(tz);
    }
    catch(...)
    {
        return static_cast<std::chrono::time_zone const*>(nullptr);
    }
}

int main(int, char**)
{
    using namespace std::chrono;
    auto z = my_locate_zone("America/Montreal");
    if (z != nullptr)
    {
        auto QuebecDay = 2023y / 6 / 24;
        auto info = z->get_info(std::chrono::sys_days(QuebecDay));
        std::cout << info << std::endl;
    }
    return 0;
}
