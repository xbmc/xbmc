/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdint.h>

#include "SystemClock.h"
#include "utils/TimeUtils.h"

namespace XbmcThreads
{
  unsigned int SystemClockMillis()
  {
    uint64_t now_time;
    static uint64_t start_time = 0;
    static bool start_time_set = false;

    now_time = static_cast<uint64_t>(1000 * CurrentHostCounter() / CurrentHostFrequency());

    if (!start_time_set)
    {
      start_time = now_time;
      start_time_set = true;
    }
    return (unsigned int)(now_time - start_time);
  }
  const unsigned int EndTime::InfiniteValue = std::numeric_limits<unsigned int>::max();
}
