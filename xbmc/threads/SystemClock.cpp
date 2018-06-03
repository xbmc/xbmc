/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
