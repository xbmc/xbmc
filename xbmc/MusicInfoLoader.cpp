#include "stdafx.h"
#include "MusicInfoLoader.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/DirectoryCache.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoLoader::CMusicInfoLoader()
{
}

CMusicInfoLoader::~CMusicInfoLoader()
{
}

void CMusicInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  if (!m_strCacheFileName.IsEmpty())
    LoadCache(m_strCacheFileName, m_mapFileItems);

  // Precache album thumbs
  g_directoryCache.InitMusicThumbCache();

  m_musicDatabase.Open();
}

bool CMusicInfoLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (pItem->m_musicInfoTag.Loaded())
    return false;

  CStdString strFileName, strPath;
  CUtil::GetDirectory(pItem->m_strPath, strPath);

  // First query cached items
  it = m_mapFileItems.find(pItem->m_strPath);
  if (it != m_mapFileItems.end() && it->second->m_musicInfoTag.Loaded() && CUtil::CompareSystemTime(&it->second->m_stTime, &pItem->m_stTime) == 0)
  {
    pItem->m_musicInfoTag = it->second->m_musicInfoTag;
  }
  else
  {
    // Have we loaded this item from database before
    IMAPSONGS it = m_songsMap.find(pItem->m_strPath);
    if (it != m_songsMap.end())
    {
      CSong& song = it->second;
      pItem->m_musicInfoTag.SetSong(song);
    }
    else if (strPath != m_strPrevPath)
    {
      // The item is from another directory as the last one,
      // query the database for the new directory...
      m_musicDatabase.GetSongsByPath(strPath, m_songsMap);

      // ...and look if we find it
      IMAPSONGS it = m_songsMap.find(pItem->m_strPath);
      if (it != m_songsMap.end())
      {
        CSong& song = it->second;
        pItem->m_musicInfoTag.SetSong(song);
      }
    }

    // Nothing found, load tag from file
    if (g_guiSettings.GetBool("MyMusic.UseTags") && !pItem->m_musicInfoTag.Loaded())
    {
      // get correct tag parser
      CMusicInfoTagLoaderFactory factory;
      auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
      if (NULL != pLoader.get())
        // get id3tag
        pLoader->Load(pItem->m_strPath, pItem->m_musicInfoTag);
    }
  }

  m_strPrevPath = strPath;
  return true;
}

void CMusicInfoLoader::OnLoaderFinish()
{
  // clear precached album thumbs
  g_directoryCache.ClearMusicThumbCache();

  // cleanup last loaded songs from database
  m_songsMap.erase(m_songsMap.begin(), m_songsMap.end());

  // cleanup cache loaded from HD
  it = m_mapFileItems.begin();
  while (it != m_mapFileItems.end())
  {
    delete it->second;
    it++;
  }
  m_mapFileItems.erase(m_mapFileItems.begin(), m_mapFileItems.end());

  // Save loaded items to HD
  if (!m_strCacheFileName.IsEmpty())
    SaveCache(m_strCacheFileName, *m_pVecItems);

  m_musicDatabase.Close();
}

void CMusicInfoLoader::UseCacheOnHD(const CStdString& strFileName)
{
  m_strCacheFileName = strFileName;
}

void CMusicInfoLoader::LoadCache(const CStdString& strFileName, MAPFILEITEMS& items)
{
  CFile file;

  if (file.Open(strFileName))
  {
    CArchive ar(&file, CArchive::load);
    int iSize = 0;
    ar >> iSize;
    for (int i = 0; i < iSize; i++)
    {
      CFileItem* pItem = new CFileItem();
      ar >> *pItem;
      items.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
    }
    ar.Close();
    file.Close();
  }
}

void CMusicInfoLoader::SaveCache(const CStdString& strFileName, CFileItemList& items)
{
  int iSize = items.Size();

  if (iSize <= 0)
    return ;

  CFile file;

  if (file.OpenForWrite(strFileName))
  {
    CArchive ar(&file, CArchive::store);
    ar << (int)items.Size();
    for (int i = 0; i < iSize; i++)
    {
      CFileItem* pItem = items[i];
      ar << *pItem;
    }
    ar.Close();
    file.Close();
  }

}
