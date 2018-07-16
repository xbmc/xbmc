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

