/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TimeUtils.h"

std::int64_t KODI::LINUX::TimespecDifference(timespec const& start, timespec const& end)
{
  return (end.tv_sec - start.tv_sec) * UINT64_C(1000000000) + (end.tv_nsec - start.tv_nsec);
}