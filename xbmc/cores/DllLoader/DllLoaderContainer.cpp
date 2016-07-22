/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DllLoaderContainer.h"
#ifdef TARGET_POSIX
#include "SoLoader.h"
#endif
#ifdef TARGET_WINDOWS
#include "Win32DllLoader.h"
#endif
#include "DllLoader.h"
#include "dll_tracker.h" // for python unload hack
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "URL.h"

#if defined(TARGET_WINDOWS)
#define ENV_PARTIAL_PATH \
                 "special://xbmcbin/;" \
                 "special://xbmcbin/system/;" \
                 "special://xbmcbin/system/python/;" \
                 "special://xbmc/;" \
                 "special://xbmc/system/;" \
                 "special://xbmc/system/python/"
#else
#define ENV_PARTIAL_PATH \
                 "special://xbmcbin/system/;" \
                 "special://xbmcbin/system/players/mplayer/;" \
                 "special://xbmcbin/system/players/VideoPlayer/;" \
                 "special://xbmcbin/system/players/paplayer/;" \
                 "special://xbmcbin/system/python/;" \
                 "special://xbmc/system/;" \
                 "special://xbmc/system/players/mplayer/;" \
                 "special://xbmc/system/players/VideoPlayer/;" \
                 "special://xbmc/system/players/paplayer/;" \
                 "special://xbmc/system/python/"
#endif
#if defined(TARGET_DARWIN)
#define ENV_PATH ENV_PARTIAL_PATH \
                 ";special://frameworks/"
#else
#define ENV_PATH ENV_PARTIAL_PATH
#endif

//Define this to get loggin on all calls to load/unload of dlls
//#define LOGALL


using namespace XFILE;

LibraryLoader* DllLoaderContainer::m_dlls[64] = {};
int        DllLoaderContainer::m_iNrOfDlls = 0;
bool       DllLoaderContainer::m_bTrack = true;

void DllLoaderContainer::Clear()
{
}

HMODULE DllLoaderContainer::GetModuleAddress(const char* sName)
{
  return (HMODULE)GetModule(sName);
}

LibraryLoader* DllLoaderContainer::GetModule(const char* sName)
{
  for (int i = 0; i < m_iNrOfDlls && m_dlls[i] != NULL; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }

  return NULL;
}

LibraryLoader* DllLoaderContainer::GetModule(HMODULE hModule)
{
  for (int i = 0; i < m_iNrOfDlls && m_dlls[i] != NULL; i++)
  {
    if (m_dlls[i]->GetHModule() == hModule) return m_dlls[i];
  }
  return NULL;
}

LibraryLoader* DllLoaderContainer::LoadModule(const char* sName, const char* sCurrentDir/*=NULL*/, bool bLoadSymbols/*=false*/)
{
  LibraryLoader* pDll=NULL;

  if (IsSystemDll(sName))
  {
    pDll = GetModule(sName);
  }
  else if (sCurrentDir)
  {
    std::string strPath=sCurrentDir;
    strPath+=sName;
    pDll = GetModule(strPath.c_str());
  }

  if (!pDll)
  {
    pDll = GetModule(sName);
  }

  if (!pDll)
  {
    pDll = FindModule(sName, sCurrentDir, bLoadSymbols);
  }
  else if (!pDll->IsSystemDll())
  {
    pDll->IncrRef();

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Already loaded Dll %s at 0x%x", pDll->GetFileName(), pDll);
#endif

  }

  return pDll;
}

LibraryLoader* DllLoaderContainer::FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols)
{
  if (URIUtils::IsInArchive(sName))
  {
    CURL url(sName);
    std::string newName = "special://temp/";
    newName += url.GetFileName();
    CFile::Copy(sName, newName);
    return FindModule(newName.c_str(), sCurrentDir, bLoadSymbols);
  }

  if (CURL::IsFullPath(sName))
  { //  Has a path, just try to load
    return LoadDll(sName, bLoadSymbols);
  }
#ifdef TARGET_POSIX
  else if (strcmp(sName, "xbmc.so") == 0)
    return LoadDll(sName, bLoadSymbols);
#endif
  else if (sCurrentDir)
  { // in the path of the parent dll?
    std::string strPath=sCurrentDir;
    strPath+=sName;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  //  in environment variable?
  std::vector<std::string> vecEnv;

#if defined(TARGET_ANDROID)
  std::string systemLibs = getenv("XBMC_ANDROID_SYSTEM_LIBS");
  vecEnv = StringUtils::Split(systemLibs, ':');
  std::string localLibs = getenv("XBMC_ANDROID_LIBS");
  vecEnv.insert(vecEnv.begin(),localLibs);
#else
  vecEnv = StringUtils::Split(ENV_PATH, ';');
#endif
  LibraryLoader* pDll = NULL;

  for (std::vector<std::string>::const_iterator i = vecEnv.begin(); i != vecEnv.end(); ++i)
  {
    std::string strPath = *i;
    URIUtils::AddSlashAtEnd(strPath);

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Searching for the dll %s in directory %s", sName, strPath.c_str());
#endif

    strPath+=sName;

    // Have we already loaded this dll
    if ((pDll = GetModule(strPath.c_str())) != NULL)
      return pDll;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  // can't find it in any of our paths - could be a system dll
  if ((pDll = LoadDll(sName, bLoadSymbols)) != NULL)
    return pDll;

  CLog::Log(LOGDEBUG, "Dll %s was not found in path", sName);
  return NULL;
}

void DllLoaderContainer::ReleaseModule(LibraryLoader*& pDll)
{
  if (!pDll)
    return;
  if (pDll->IsSystemDll())
  {
    CLog::Log(LOGFATAL, "%s is a system dll and should never be released", pDll->GetName());
    return;
  }

  int iRefCount=pDll->DecrRef();
  if (iRefCount==0)
  {

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Releasing Dll %s", pDll->GetFileName());
#endif

    if (!pDll->HasSymbols())
    {
      pDll->Unload();
      delete pDll;
      pDll=NULL;
    }
    else
      CLog::Log(LOGINFO, "%s has symbols loaded and can never be unloaded", pDll->GetName());
  }
#ifdef LOGALL
  else
  {
    CLog::Log(LOGDEBUG, "Dll %s is still referenced with a count of %d", pDll->GetFileName(), iRefCount);
  }
#endif
}

LibraryLoader* DllLoaderContainer::LoadDll(const char* sName, bool bLoadSymbols)
{

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Loading dll %s", sName);
#endif

  LibraryLoader* pLoader;
#ifdef TARGET_POSIX
  pLoader = new SoLoader(sName, bLoadSymbols);
#elif defined(TARGET_WINDOWS)
  pLoader = new Win32DllLoader(sName);
#else
  pLoader = new DllLoader(sName, m_bTrack, false, bLoadSymbols);
#endif

  if (!pLoader)
  {
    CLog::Log(LOGERROR, "Unable to create dll %s", sName);
    return NULL;
  }

  if (!pLoader->Load())
  {
    delete pLoader;
    return NULL;
  }

  return pLoader;
}

bool DllLoaderContainer::IsSystemDll(const char* sName)
{
  for (int i = 0; i < m_iNrOfDlls && m_dlls[i] != NULL; i++)
  {
    if (m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetName(), sName) == 0) return true;
  }

  return false;
}

int DllLoaderContainer::GetNrOfModules()
{
  return m_iNrOfDlls;
}

LibraryLoader* DllLoaderContainer::GetModule(int iPos)
{
  if (iPos < m_iNrOfDlls) return m_dlls[iPos];
  return NULL;
}

void DllLoaderContainer::RegisterDll(LibraryLoader* pDll)
{
  for (int i = 0; i < 64; i++)
  {
    if (m_dlls[i] == NULL)
    {
      m_dlls[i] = pDll;
      m_iNrOfDlls++;
      break;
    }
  }
}

void DllLoaderContainer::UnRegisterDll(LibraryLoader* pDll)
{
  if (pDll)
  {
    if (pDll->IsSystemDll())
    {
      CLog::Log(LOGFATAL, "%s is a system dll and should never be removed", pDll->GetName());
    }
    else
    {
      // remove from the list
      bool bRemoved = false;
      for (int i = 0; i < m_iNrOfDlls && m_dlls[i]; i++)
      {
        if (m_dlls[i] == pDll) bRemoved = true;
        if (bRemoved && i + 1 < m_iNrOfDlls)
        {
          m_dlls[i] = m_dlls[i + 1];
        }
      }
      if (bRemoved)
      {
        m_iNrOfDlls--;
        m_dlls[m_iNrOfDlls] = NULL;
      }
    }
  }
}

void DllLoaderContainer::UnloadPythonDlls()
{
  // unload all dlls that python could have loaded
  for (int i = 0; i < m_iNrOfDlls && m_dlls[i] != NULL; i++)
  {
    const char* name = m_dlls[i]->GetName();
    if (strstr(name, ".pyd") != NULL)
    {
      LibraryLoader* pDll = m_dlls[i];
      ReleaseModule(pDll);
      i = 0;
    }
  }

}
