/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*
 * addon.h
 *
 *  Created on: Aug 21, 2010
 *      Author: jim
 */

//#define ENABLE_XBMC_TRACE_API

#include "threads/SingleLock.h"

#include <memory>
#include <vector>

#ifdef TARGET_WINDOWS
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

/**
 * This file contains the public definitions for the Addon api. It's meant to be used
 * by those writing language bindings.
 */

namespace XBMCAddon
{
class LanguageHook;
}

namespace XBMCAddonUtils
{
  class GuiLock
  {
  public:
    GuiLock(XBMCAddon::LanguageHook* languageHook, bool offScreen);
    ~GuiLock();

  protected:
    XBMCAddon::LanguageHook* m_languageHook = nullptr;
    bool m_offScreen = false;
  };

  class InvertSingleLockGuard
  {
    CSingleLock& lock;
  public:
    explicit InvertSingleLockGuard(CSingleLock& _lock) : lock(_lock) { lock.Leave(); }
    ~InvertSingleLockGuard() { lock.Enter(); }
  };


  /*
   * Looks in references.xml for image name
   * If none exist return default image name
   */
  const char *getDefaultImage(const char* cControlType, const char* cTextureType);

#ifdef ENABLE_XBMC_TRACE_API
  class TraceGuard
  {
    const char* function;
  public:
    TraceGuard* parent;
    int depth;

    const char* getSpaces();

    explicit TraceGuard(const char* _function);
    TraceGuard();
    ~TraceGuard();
  };
#endif
}

#ifdef ENABLE_XBMC_TRACE_API
#define XBMC_TRACE XBMCAddonUtils::TraceGuard _tg(__PRETTY_FUNCTION__)
#else
#define XBMC_TRACE
#endif


