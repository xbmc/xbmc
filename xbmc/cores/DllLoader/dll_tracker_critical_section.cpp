/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 
#include "stdafx.h"
#include "dll_tracker_critical_section.h"
#include "dll_tracker.h"
#include "dll.h"
#include "DllLoader.h"
#include "exports/emu_kernel32.h"

extern "C" inline void tracker_critical_section_track(uintptr_t caller, LPCRITICAL_SECTION cs)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && cs)
  {
    CSingleLock lock(*pInfo);
    pInfo->criticalSectionList.push_back(cs);
  }
}

extern "C" inline void tracker_critical_section_free(uintptr_t caller, LPCRITICAL_SECTION cs)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && cs)
  {
    CSingleLock lock(*pInfo);
    for (CriticalSectionListIter it = pInfo->criticalSectionList.begin(); it != pInfo->criticalSectionList.end(); ++it)
    {
      if (*it == cs)
      {
        pInfo->criticalSectionList.erase(it);
        break;
      }
    }
  }
}

extern "C" void tracker_critical_section_free_all(DllTrackInfo* pInfo)
{
  // unloading unloaded dll's
  if (!pInfo->criticalSectionList.empty())
  {
    CSingleLock lock(*pInfo);
    CLog::Log(LOGDEBUG,"%s: Detected %d unfreed critical sections", pInfo->pDll->GetFileName(), pInfo->criticalSectionList.size());
    for (CriticalSectionListIter it = pInfo->criticalSectionList.begin(); it != pInfo->criticalSectionList.end(); ++it)
    {
      LPCRITICAL_SECTION cs = *it;
      dllDeleteCriticalSection(cs);
    }
  }
  
  pInfo->criticalSectionList.erase(pInfo->criticalSectionList.begin(), pInfo->criticalSectionList.end());
}

extern "C" void __stdcall track_InitializeCriticalSection(LPCRITICAL_SECTION cs)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();
  
  dllInitializeCriticalSection(cs);
  
  tracker_critical_section_track(loc, cs);
}

extern "C" void __stdcall track_DeleteCriticalSection(LPCRITICAL_SECTION cs)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  tracker_critical_section_free(loc, cs);

  dllDeleteCriticalSection(cs);
}
