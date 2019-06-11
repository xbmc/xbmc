/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <sys/types.h>
#include <unistd.h>

constexpr int THREAD_BASE_PRIORITY_LOWRT{15};
constexpr int THREAD_BASE_PRIORITY_MAX{2};
constexpr int THREAD_BASE_PRIORITY_MIN{-2};
constexpr int THREAD_BASE_PRIORITY_IDLE{-15};
constexpr int THREAD_PRIORITY_LOWEST{THREAD_BASE_PRIORITY_MIN};
constexpr int THREAD_PRIORITY_BELOW_NORMAL{THREAD_PRIORITY_LOWEST + 1};
constexpr int THREAD_PRIORITY_NORMAL{0};
constexpr int THREAD_PRIORITY_HIGHEST{THREAD_BASE_PRIORITY_MAX};
constexpr int THREAD_PRIORITY_ABOVE_NORMAL{THREAD_PRIORITY_HIGHEST - 1};

typedef pid_t ThreadLwpId;

