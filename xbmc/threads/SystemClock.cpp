/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SystemClock.h"

#include "utils/TimeUtils.h"

#include <stdint.h>

namespace XbmcThreads
{

unsigned int SystemClockMillis()
{
  return static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now().time_since_epoch())
                                       .count());
}
}
