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

#include "system.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace std;

class CSectionLoader g_sectionLoader;

//  delay for unloading dll's
#define UNLOAD_DELAY 30*1000 // 30 sec.

//Define this to get loggin on all calls to load/unload sections/dlls
//#define LOGALL

CSectionLoader::CSectionLoader(void)
{}

CSectionLoader::~CSectionLoader(void)
{}

bool CSectionLoader::IsLoaded(const CStdString& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedSections.size(); ++i)
  {
    CSection& section = g_sectionLoader.m_vecLoadedSections[i];
    if (section.m_strSectionName == strSection && section.m_lReferenceCount > 0) return true;
  }
  return false;
}

bool CSectionLoader::Load(const CStdString& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedSections.size(); ++i)
  {
    CSection& section = g_sectionLoader.m_vecLoadedSections[i];
    if (section.m_strSectionName == strSection)
    {

#ifdef LOGALL
      CLog::Log(LOGDEBUG,"SECTION:LoadSection(%s) count:%i\n", strSection.c_str(), section.m_lReferenceCount);
#endif

      section.m_lReferenceCount++;
      return true;
    }
  }

#ifdef HAS_SECTIONS
  if ( NULL == XLoadSection(strSection.c_str() ) )
  {
    CLog::Log(LOGDEBUG,"SECTION:LoadSection(%s) load failed!!\n", strSection.c_str());
    return false;
  }
  HANDLE hHandle = XGetSectionHandle(strSection.c_str());

  CLog::Log(LOGDEBUG,"SECTION:Section %s loaded count:1 size:%i\n", strSection.c_str(), XGetSectionSize(hHandle) );
#endif

  CSection newSection;
  newSection.m_strSectionName = strSection;
  newSection.m_lReferenceCount = 1;
  g_sectionLoader.m_vecLoadedSections.push_back(newSection);
  return true;
}

void CSectionLoader::Unload(const CStdString& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);
  if (!CSectionLoader::IsLoaded(strSection)) return ;

  ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section = *i;
    if (section.m_strSectionName == strSection)
    {
#ifdef LOGALL
      CLog::Log(LOGDEBUG,"SECTION:FreeSection(%s) count:%i\n", strSection.c_str(), section.m_lReferenceCount);
#endif
      section.m_lReferenceCount--;
      if ( 0 == section.m_lReferenceCount)
      {
        section.m_unloadDelayStartTick = CTimeUtils::GetTimeMS();
        return ;
      }
    }
    ++i;
  }
}

LibraryLoader *CSectionLoader::LoadDLL(const CStdString &dllname, bool bDelayUnload /*=true*/, bool bLoadSymbols /*=false*/)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (!dllname) return NULL;
  // check if it's already loaded, and increase the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_strDllName.Equals(dllname))
    {
      dll.m_lReferenceCount++;
      return dll.m_pDll;
    }
  }

  // ok, now load the dll
  CLog::Log(LOGDEBUG, "SECTION:LoadDLL(%s)\n", dllname.c_str());
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

void CSectionLoader::UnloadDLL(const CStdString &dllname)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (!dllname) return;
  // check if it's already loaded, and decrease the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_strDllName.Equals(dllname))
    {
      dll.m_lReferenceCount--;
      if (0 == dll.m_lReferenceCount)
      {
        if (dll.m_bDelayUnload)
          dll.m_unloadDelayStartTick = CTimeUtils::GetTimeMS();
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

  ivecLoadedSections i = g_sectionLoader.m_vecLoadedSections.begin();
  while( i != g_sectionLoader.m_vecLoadedSections.end() )
  {
    CSection& section = *i;
    if( section.m_lReferenceCount == 0 && CTimeUtils::GetTimeMS() - section.m_unloadDelayStartTick > UNLOAD_DELAY)
    {
      CLog::Log(LOGDEBUG,"SECTION:UnloadDelayed(SECTION: %s)", section.m_strSectionName.c_str());
#ifdef HAS_SECTIONS
      XFreeSection(section.m_strSectionName.c_str());
#endif
      i = g_sectionLoader.m_vecLoadedSections.erase(i);
      continue;
    }
    i++;
  }

  // check if we can unload any unreferenced dlls
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_lReferenceCount == 0 && CTimeUtils::GetTimeMS() - dll.m_unloadDelayStartTick > UNLOAD_DELAY)
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
  ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section = *i;
    //g_sectionLoader.m_vecLoadedSections.erase(i);
    CLog::Log(LOGDEBUG,"SECTION:UnloadAll(SECTION: %s)", section.m_strSectionName.c_str());
#ifdef HAS_SECTIONS
    XFreeSection(section.m_strSectionName.c_str());
#endif
    i = g_sectionLoader.m_vecLoadedSections.erase(i);
  }

  // delete the dll's
  CSingleLock lock(g_sectionLoader.m_critSection);
  vector<CDll>::iterator it = g_sectionLoader.m_vecLoadedDLLs.begin();
  while (it != g_sectionLoader.m_vecLoadedDLLs.end())
  {
    CDll& dll = *it;
    CLog::Log(LOGDEBUG,"SECTION:UnloadAll(DLL: %s)", dll.m_strDllName.c_str());
    if (dll.m_pDll)
      DllLoaderContainer::ReleaseModule(dll.m_pDll);
    it = g_sectionLoader.m_vecLoadedDLLs.erase(it);
  }
}
