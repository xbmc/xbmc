#include "stdafx.h"
#include "ASAPFileDirectory.h"
#include "FileSystem/File.h"
#include "MusicInfoTagLoaderASAP.h"

using namespace DIRECTORY;

CASAPFileDirectory::CASAPFileDirectory()
{
  m_strExt = "asapstream";
}

CASAPFileDirectory::~CASAPFileDirectory()
{
}

int CASAPFileDirectory::GetTrackCount(const CStdString &strPath)
{
  if (!m_dll.Load())
    return 0;
  
  MUSIC_INFO::CMusicInfoTagLoaderASAP loader;
  loader.Load(strPath,m_tag);
  m_tag.SetDuration(0); // ignore duration or all songs get duration of track 1

  // hack until i figure out why the dll isnt properly wrapped
  if (!XFILE::CFile::Cache(strPath,"z:\\cachedasap.sap"))
    return 0;

  return m_dll.asapGetSongs("z:\\cachedasap.sap");
}
