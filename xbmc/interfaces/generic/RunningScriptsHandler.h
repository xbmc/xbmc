/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

#include <cstdint>
#include <map>

template<class TScript>
class CRunningScriptsHandler
{
protected:
  using HandleType = int;

  CRunningScriptsHandler() = default;
  virtual ~CRunningScriptsHandler() = default;

  static HandleType GetNewScriptHandle(TScript* script)
  {
    CSingleLock lock(s_critical);
    uint32_t handle = ++s_scriptHandleCounter;
    s_scriptHandles[handle] = script;

    return handle;
  }

  static void ReuseScriptHandle(HandleType handle, TScript* script)
  {
    CSingleLock lock(s_critical);
    s_scriptHandles[handle] = script;
  }

  static void RemoveScriptHandle(HandleType handle)
  {
    CSingleLock lock(s_critical);
    s_scriptHandles.erase(handle);
  }

  static TScript* GetScriptFromHandle(HandleType handle)
  {
    CSingleLock lock(s_critical);
    auto scriptHandle = s_scriptHandles.find(handle);
    if (scriptHandle == s_scriptHandles.end())
      return nullptr;

    return scriptHandle->second;
  }

  static inline CCriticalSection& GetScriptsLock() { return s_critical; }

private:
  static std::map<HandleType, TScript*> s_scriptHandles;
  static CCriticalSection s_critical;
  static HandleType s_scriptHandleCounter;
};

template<class TScript>
std::map<typename CRunningScriptsHandler<TScript>::HandleType, TScript*>
    CRunningScriptsHandler<TScript>::s_scriptHandles;
template<class TScript>
CCriticalSection CRunningScriptsHandler<TScript>::s_critical;
template<class TScript>
typename CRunningScriptsHandler<TScript>::HandleType
    CRunningScriptsHandler<TScript>::s_scriptHandleCounter = 0;
