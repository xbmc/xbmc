/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <pthread.h>
#include <unistd.h>

const int THREAD_BASE_PRIORITY_LOWRT{15};
const int THREAD_BASE_PRIORITY_MAX{2};
const int THREAD_BASE_PRIORITY_MIN{-2};
const int THREAD_BASE_PRIORITY_IDLE{-15};
const int THREAD_PRIORITY_LOWEST{THREAD_BASE_PRIORITY_MIN};
const int THREAD_PRIORITY_BELOW_NORMAL{THREAD_PRIORITY_LOWEST + 1};
const int THREAD_PRIORITY_NORMAL{0};
const int THREAD_PRIORITY_HIGHEST{THREAD_BASE_PRIORITY_MAX};
const int THREAD_PRIORITY_ABOVE_NORMAL{THREAD_PRIORITY_HIGHEST - 1};

struct threadOpaque
{
  pid_t LwpId;
};

typedef pthread_t ThreadIdentifier;
typedef threadOpaque ThreadOpaque;
typedef int THREADFUNC;

namespace XbmcThreads
{
  inline static void ThreadSleep(unsigned int millis) { usleep(millis*1000); }
}

