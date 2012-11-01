/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * addon.h
 *
 *  Created on: Aug 21, 2010
 *      Author: jim
 */

#pragma once

//#define ENABLE_TRACE_API

#include "threads/SingleLock.h"

#include <vector>

#ifdef TARGET_WINDOWS
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

/**
 * This file contains the public definitions for the Addon api. It's meant to be used
 * by those writing language bindings.
 */
namespace XBMCAddonUtils
{
  //***********************************************************
  // Some simple helpers
  void guiLock();
  void guiUnlock();
  //***********************************************************

  class GuiLock
  {
  public:
    GuiLock() { guiLock(); }
    ~GuiLock() { guiUnlock(); }
  };

  class InvertSingleLockGuard
  {
    CSingleLock& lock;
  public:
    InvertSingleLockGuard(CSingleLock& _lock) : lock(_lock) { lock.Leave(); }
    ~InvertSingleLockGuard() { lock.Enter(); }
  };

//  class WaitForNotify
//  {
//    std::vector<HANDLE> thoseWaiting;
//    CCriticalSection csection; // Ha!
//    CCriticalSection atomicWait;
//  public:
//    void wait();
//    void notify();
//    void notifyAll();
//  };

#define LOCKGUI XBMCAddonUtils::GuiLock __gl

  /*
   * Looks in references.xml for image name
   * If none exist return default image name
   */
  const char *getDefaultImage(char* cControlType, char* cTextureType, char* cDefault);

#ifdef ENABLE_TRACE_API
  class TraceGuard
  {
    const char* function;
  public:
    TraceGuard* parent;
    int depth;

    const char* getSpaces();

    TraceGuard(const char* _function);
    TraceGuard();
    ~TraceGuard();
  };
#endif
}

#ifdef ENABLE_TRACE_API
#define TRACE XBMCAddonUtils::TraceGuard _tg(__PRETTY_FUNCTION__)
#else
#define TRACE
#endif


