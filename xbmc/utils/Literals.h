/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

constexpr unsigned long long int operator"" _kib (unsigned long long int val)
{
    return val * 1024ull;
}

constexpr unsigned long long int operator"" _kb (unsigned long long int val)
{
    return val * 1000ull;
}

constexpr unsigned long long int operator"" _mib (unsigned long long int val)
{
    return val * 1024ull * 1024ull;
}

constexpr unsigned long long int operator"" _mb (unsigned long long int val)
{
    return val * 1000ull * 1000ull;
}
