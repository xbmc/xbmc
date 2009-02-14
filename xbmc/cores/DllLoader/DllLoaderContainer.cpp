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
#include "DllLoaderContainer.h"
#include "DllLoader.h"
#include "dll_tracker.h" // for python unload hack
#include "FileSystem/File.h"
#include "Util.h"

#define ENV_PATH "special://xbmc/system/;special://xbmc/system/players/mplayer/;special://xbmc/system/players/dvdplayer/;special://xbmc/system/players/paplayer/;special://xbmc/system/python/"

//Define this to get loggin on all calls to load/unload of dlls
//#define LOGALL


using namespace XFILE;

LibraryLoader* DllLoaderContainer::m_dlls[64] = {};
int        DllLoaderContainer::m_iNrOfDlls = 0;
bool       DllLoaderContainer::m_bTrack = true;


Export export_advapi32[];
Export export_ole32[];
Export export_winmm[];
Export export_user32[];
Export export_msdmo[];
Export export_xbmc_vobsub[];
Export export_kernel32[];
Export export_wsock32[];
Export export_ws2_32[];
Export export_xbox_dx8[];
Export export_xbox___dx8[];
Export export_version[];
Export export_comdlg32[];
Export export_gdi32[];
Export export_ddraw[];
Export export_comctl32[];
Export export_msvcrt[];
Export export_pncrt[];
Export export_iconvx[];
Export export_xbp[];
Export export_zlib[];

DllLoader kernel32("kernel32.dll",        false, true, false, export_kernel32);
DllLoader msvcr80("msvcr80.dll",          false, true, false, export_msvcrt);
DllLoader msvcr71("msvcr71.dll",          false, true, false, export_msvcrt);
DllLoader msvcrt("msvcrt.dll",            false, true, false, export_msvcrt);
DllLoader wsock32("wsock32.dll",          false, true, false, export_wsock32);
DllLoader ws2_32("ws2_32.dll",            false, true, false, export_ws2_32);
DllLoader user32("user32.dll",            false, true, false, export_user32);
DllLoader ddraw("ddraw.dll",              false, true, false, export_ddraw);
DllLoader wininet("wininet.dll",          false, true, false, NULL);
DllLoader advapi32("advapi32.dll",        false, true, false, export_advapi32);
DllLoader ole32("ole32.dll",              false, true, false, export_ole32);
DllLoader oleaut32("oleaut32.dll",        false, true, false, NULL);
DllLoader xbp("xbp.dll",                  false, true, false, export_xbp);
DllLoader winmm("winmm.dll",              false, true, false, export_winmm);
DllLoader msdmo("msdmo.dll",              false, true, false, export_msdmo);
DllLoader xbmc_vobsub("xbmc_vobsub.dll",  false, true, false, export_xbmc_vobsub);
DllLoader xbox_dx8("xbox_dx8.dll",        false, true, false, export_xbox_dx8);
DllLoader version("version.dll",          false, true, false, export_version);
DllLoader comdlg32("comdlg32.dll",        false, true, false, export_comdlg32);
DllLoader gdi32("gdi32.dll",              false, true, false, export_gdi32);
DllLoader comctl32("comctl32.dll",        false, true, false, export_comctl32);
DllLoader pncrt("pncrt.dll",              false, true, false, export_pncrt);
DllLoader iconvx("iconv.dll",             false, true, false, export_iconvx);
DllLoader zlib("zlib1.dll",               false, true, false, export_zlib);
  
void DllLoaderContainer::Clear()
{
}

HMODULE DllLoaderContainer::GetModuleAddress(const char* sName)
{
  return (HMODULE)GetModule(sName);
}

LibraryLoader* DllLoaderContainer::GetModule(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }

  return NULL;
}

LibraryLoader* DllLoaderContainer::GetModule(HMODULE hModule)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
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
    CStdString strPath=sCurrentDir;
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
  if (CUtil::IsInArchive(sName))
  {
    CURL url(sName);
    CStdString newName = "special://temp/";
    newName += url.GetFileName();
    CFile::Cache(sName, newName);
    return FindModule(newName, sCurrentDir, bLoadSymbols);
  }

  if (CURL::IsFullPath(sName))
  { //  Has a path, just try to load
    return LoadDll(sName, bLoadSymbols);
  }
  else if (sCurrentDir)
  { // in the path of the parent dll?
    CStdString strPath=sCurrentDir;
    strPath+=sName;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

  //  in environment variable?
  CStdStringArray vecEnv;
  StringUtils::SplitString(ENV_PATH, ";", vecEnv);

  for (int i=0; i<(int)vecEnv.size(); ++i)
  {
    CStdString strPath=vecEnv[i];

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Searching for the dll %s in directory %s", sName, strPath.c_str());
#endif

    strPath+=sName;

    // Have we already loaded this dll
    LibraryLoader* pDll = GetModule(strPath.c_str());
    if (pDll)
      return pDll;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str(), bLoadSymbols);
  }

#ifdef _WIN32PC
  // can't find it in any of our paths - could be a system dll
  return LoadDll(sName, bLoadSymbols);
#endif
  CLog::Log(LOGDEBUG, "Dll %s was not found in path", sName);

  return NULL;
}

void DllLoaderContainer::ReleaseModule(LibraryLoader*& pDll)
{
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
  pLoader = new DllLoader(sName, m_bTrack, false, bLoadSymbols);
    
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
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
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
  // unload all dlls that python24.dll could have loaded
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();
    if (strstr(name, ".pyd") != NULL)
    {
      LibraryLoader* pDll = m_dlls[i];
      ReleaseModule(pDll);
      i = 0;
    }
  }

  // last dll to unload, python24.dll
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    char* name = m_dlls[i]->GetName();
    if (strstr(name, "python24.dll") != NULL)
    {
      LibraryLoader* pDll = m_dlls[i];
      pDll->IncrRef();
      while (pDll->DecrRef() > 1) pDll->DecrRef();
      
      // since we freed all python extension dlls first, we have to remove any associations with them first
      DllTrackInfo* info = tracker_get_dlltrackinfo_byobject((DllLoader*) pDll);
      if (info != NULL)
      {
        info->dllList.clear();
      }
      
      ReleaseModule(pDll);
      break;
    }
  }

  
}
