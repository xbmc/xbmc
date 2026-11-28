/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/CPUInfo.h"

#include <vector>

namespace KODI
{
namespace LINUX
{
//! CPU time used by each thread of this process, read from /proc/self/task
std::vector<ThreadCpuTime> GetThreadCpuTimes();
} // namespace LINUX
} // namespace KODI
