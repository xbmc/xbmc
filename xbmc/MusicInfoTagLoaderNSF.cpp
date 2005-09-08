#include "stdafx.h"
#include "MusicInfoTagLoaderNSF.h"
#include "lib/mikxbox/mikmod.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderNSF::CMusicInfoTagLoaderNSF(void)
{
  m_bDllLoaded = false;
  m_nsf = 0;
}

CMusicInfoTagLoaderNSF::~CMusicInfoTagLoaderNSF()
{
  CSectionLoader::UnloadDLL(NSF_DLL);
  m_bDllLoaded = false;
}

int CMusicInfoTagLoaderNSF::GetStreamCount(const CStdString& strFileName)
{
  if (!LoadDLL())
    return 0;
   
  m_nsf = m_dll.DLL_LoadNSF(strFileName.c_str());
  if (!m_nsf)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderNSF: failed to open NSF %s",strFileName.c_str());
    return 0;
  }
  int result = m_dll.DLL_GetNumberOfSongs(m_nsf);
  m_dll.DLL_FreeNSF(m_nsf);
  
  return result;
}

bool CMusicInfoTagLoaderNSF::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!LoadDLL())
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderNSF: failed loading DLL %s",NSF_DLL);
    return false;
  }

  m_nsf = m_dll.DLL_LoadNSF(strFileName.c_str());
  if (!m_nsf)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderNSF: failed to open NSF %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.DLL_GetTitle(m_nsf); // no alloc
  if( strcmp(szTitle,"<?>") ) 
  {
    tag.SetTitle(szTitle);
    tag.SetLoaded(true);
  }
  
  char* szArtist = (char*)m_dll.DLL_GetArtist(m_nsf); // no alloc
  if( strcmp(szArtist,"<?>") && tag.Loaded() ) 
    tag.SetArtist(szArtist);
  
  m_dll.DLL_FreeNSF(m_nsf);
  m_nsf = 0;

  return tag.Loaded();
}

bool CMusicInfoTagLoaderNSF::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll = CSectionLoader::LoadDLL(NSF_DLL);
  if (!pDll)
  {
    CLog::Log(LOGERROR, "NSFCodec: Unable to load dll %s", NSF_DLL);
    return false;
  }

  // get handle to the functions in the dll
  pDll->ResolveExport("DLL_LoadNSF", (void**)&m_dll.DLL_LoadNSF);
  pDll->ResolveExport("DLL_FreeNSF", (void**)&m_dll.DLL_FreeNSF);

  pDll->ResolveExport("DLL_GetTitle", (void**)&m_dll.DLL_GetTitle);
  pDll->ResolveExport("DLL_GetArtist", (void**)&m_dll.DLL_GetArtist);
  pDll->ResolveExport("DLL_GetNumberOfSongs", (void**)&m_dll.DLL_GetNumberOfSongs);
 
  // Check resolves + version number
  if ( !m_dll.DLL_FreeNSF || ! m_dll.DLL_LoadNSF || !m_dll.DLL_GetTitle || !m_dll.DLL_GetArtist || !m_dll.DLL_GetNumberOfSongs)
  {
    CLog::Log(LOGERROR, "MusicInfoTagLoaderNSF: Unable to resolve exports from %s", NSF_DLL);
    CSectionLoader::UnloadDLL(NSF_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}