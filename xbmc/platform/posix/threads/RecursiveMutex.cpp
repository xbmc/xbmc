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

static pthread_mutexattr_t recursiveAttr;

static bool SetRecursiveAttr()
{
  static bool alreadyCalled = false;

  if (!alreadyCalled)
  {
    pthread_mutexattr_init(&recursiveAttr);
    pthread_mutexattr_settype(&recursiveAttr, PTHREAD_MUTEX_RECURSIVE);
#if !defined(TARGET_ANDROID)
    pthread_mutexattr_setprotocol(&recursiveAttr, PTHREAD_PRIO_INHERIT);
#endif
    alreadyCalled = true;
  }

  return true; // note, we never call destroy.
}

static bool recursiveAttrSet = SetRecursiveAttr();

pthread_mutexattr_t& CRecursiveMutex::getRecursiveAttr()
{
  if (!recursiveAttrSet) // this is only possible in the single threaded startup code
    recursiveAttrSet = SetRecursiveAttr();

  return recursiveAttr;
}

} // namespace XbmcThreads
