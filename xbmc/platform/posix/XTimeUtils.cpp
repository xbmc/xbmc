/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XTimeUtils.h"

#include <sched.h>
#include <unistd.h>

namespace KODI
{
namespace TIME
{

void Sleep(uint32_t milliSeconds)
{
#if _POSIX_PRIORITY_SCHEDULING
  if (milliSeconds == 0)
  {
    sched_yield();
    return;
  }
#endif

  usleep(milliSeconds * 1000);
}

} // namespace TIME
} // namespace KODI
