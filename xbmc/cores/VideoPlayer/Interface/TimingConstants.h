/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE 0xFFF0000000000000

constexpr int64_t DVD_TIME_TO_MSEC(double x)
{
  return static_cast<int64_t>(x * 1000 / DVD_TIME_BASE);
}
constexpr double DVD_SEC_TO_TIME(double x) { return x * DVD_TIME_BASE; }
constexpr double DVD_MSEC_TO_TIME(double x) { return x * DVD_TIME_BASE / 1000; }

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000
