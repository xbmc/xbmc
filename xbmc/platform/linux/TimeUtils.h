/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <time.h>

namespace KODI
{
namespace LINUX
{

/**
 * Calculate difference between two timespecs in nanoseconds
 * \param start earlier time
 * \param end later time
 * \return (end - start) in nanoseconds
 */
std::int64_t TimespecDifference(timespec const& start, timespec const& end);

}
}
