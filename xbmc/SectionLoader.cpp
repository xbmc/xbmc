/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SectionLoader.h"

#include "cores/DllLoader/DllLoaderContainer.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/GlobalsHandling.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#define g_sectionLoader XBMC_GLOBAL_USE(CSectionLoader)

//  delay for unloading dll's
#define UNLOAD_DELAY 30*1000 // 30 sec.

//Define this to get loggin on all calls to load/unload sections/dlls
//#define LOGALL

CSectionLoader::CSectionLoader(void) = default;

CSectionLoader::~CSectionLoader(void)
{
  UnloadAll();
}

LibraryLoader *CSectionLoader::LoadDLL(const std::string &dllname, bool bDelayUnload /*=true*/, bool bLoadSymbols /*=false*/)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (dllname.empty()) return NULL;
  // check if it's already loaded, and increase the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (StringUtils::EqualsNoCase(dll.m_strDllName, dllname))
    {
      dll.m_lReferenceCount++;
      return dll.m_pDll;
    }
  }

  // ok, now load the dll
  CLog::Log(LOGDEBUG, "SECTION:LoadDLL(%s)", dllname.c_str());
  LibraryLoader* pDll = DllLoaderContainer::LoadModule(dllname.c_str(), NULL, bLoadSymbols);
  if (!pDll)
    return NULL;

  CDll newDLL;
  newDLL.m_strDllName = dllname;
  newDLL.m_lReferenceCount = 1;
  newDLL.m_bDelayUnload=bDelayUnload;
  newDLL.m_pDll=pDll;
  g_sectionLoader.m_vecLoadedDLLs.push_back(newDLL);

  return newDLL.m_pDll;
}

void CSectionLoader::UnloadDLL(const std::string &dllname)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (dllname.empty()) return;
  // check if it's already loaded, and decrease the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (StringUtils::EqualsNoCase(dll.m_strDllName, dllname))
    {
      dll.m_lReferenceCount--;
      if (0 == dll.m_lReferenceCount)
      {
        if (dll.m_bDelayUnload)
          dll.m_unloadDelayStartTick = XbmcThreads::SystemClockMillis();
        else
        {
          CLog::Log(LOGDEBUG,"SECTION:UnloadDll(%s)", dllname.c_str());
          if (dll.m_pDll)
            DllLoaderContainer::ReleaseModule(dll.m_pDll);
          g_sectionLoader.m_vecLoadedDLLs.erase(g_sectionLoader.m_vecLoadedDLLs.begin() + i);
        }

        return;
      }
    }
  }
}

void CSectionLoader::UnloadDelayed()
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  // check if we can unload any unreferenced dlls
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_lReferenceCount == 0 && XbmcThreads::SystemClockMillis() - dll.m_unloadDelayStartTick > UNLOAD_DELAY)
    {
      CLog::Log(LOGDEBUG,"SECTION:UnloadDelayed(DLL: %s)", dll.m_strDllName.c_str());

      if (dll.m_pDll)
        DllLoaderContainer::ReleaseModule(dll.m_pDll);
      g_sectionLoader.m_vecLoadedDLLs.erase(g_sectionLoader.m_vecLoadedDLLs.begin() + i);
      return;
    }
  }
}

void CSectionLoader::UnloadAll()
{
  // delete the dll's
  CSingleLock lock(g_sectionLoader.m_critSection);
  std::vector<CDll>::iterator it = g_sectionLoader.m_vecLoadedDLLs.begin();
  while (it != g_sectionLoader.m_vecLoadedDLLs.end())
  {
    CDll& dll = *it;
    if (dll.m_pDll)
      DllLoaderContainer::ReleaseModule(dll.m_pDll);
    it = g_sectionLoader.m_vecLoadedDLLs.erase(it);
  }
}
