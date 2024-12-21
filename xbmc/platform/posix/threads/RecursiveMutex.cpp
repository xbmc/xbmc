/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RecursiveMutex.h"

namespace XbmcThreads
{

pthread_mutexattr_t& CRecursiveMutex::getRecursiveAttr()
{
  static pthread_mutexattr_t recursiveAttr = []()
  {
    pthread_mutexattr_t recursiveAttr;
    pthread_mutexattr_init(&recursiveAttr);
    pthread_mutexattr_settype(&recursiveAttr, PTHREAD_MUTEX_RECURSIVE);
#if !defined(TARGET_ANDROID)
    pthread_mutexattr_setprotocol(&recursiveAttr, PTHREAD_PRIO_INHERIT);
#endif
    return recursiveAttr;
  }();

  return recursiveAttr;
}

} // namespace XbmcThreads
