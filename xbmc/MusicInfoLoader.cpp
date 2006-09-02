#include "stdafx.h"
#include "MusicInfoLoader.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/DirectoryCache.h"
#include "Util.h"


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
  else
  {
    m_mapFileItems.m_strPath=m_pVecItems->m_strPath;
    m_mapFileItems.Load();
    m_mapFileItems.SetFastLookup(true);
  }

  // Precache album thumbs
  g_directoryCache.InitMusicThumbCache();

  m_strPrevPath.Empty();

  m_databaseHits = m_tagReads = 0;

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());

  m_musicDatabase.Open();
}

bool CMusicInfoLoader::LoadItem(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->m_bIsFolder)
    m_pProgressCallback->SetProgressAdvance();

  if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (pItem->m_musicInfoTag.Loaded())
    return true;

  CFileItem* mapItem=NULL;
  // first check the cached item
  if ((mapItem=m_mapFileItems[pItem->m_strPath])!=NULL && mapItem->m_dateTime==pItem->m_dateTime)
  { // Query map if we previously cached the file on HD
    pItem->m_musicInfoTag = mapItem->m_musicInfoTag;
    pItem->SetThumbnailImage(mapItem->GetThumbnailImage());
    return true;
  }

  CStdString strPath;
  CUtil::GetDirectory(pItem->m_strPath, strPath);
  if (strPath!=m_strPrevPath)
  {
    // The item is from another directory as the last one,
    // query the database for the new directory...
    m_musicDatabase.GetSongsByPath(strPath, m_songsMap);
    m_databaseHits++;
  }

  CSong *song=NULL;

  if ((song=m_songsMap.Find(pItem->m_strPath))!=NULL)
  {  // Have we loaded this item from database before
    pItem->m_musicInfoTag.SetSong(*song);
    pItem->SetThumbnailImage(song->strThumb);
  }
  else if (g_guiSettings.GetBool("musicfiles.usetags") || pItem->IsCDDA())
  { // Nothing found, load tag from file,
    // always try to load cddb info
    // get correct tag parser
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
    if (NULL != pLoader.get())
      // get tag
      pLoader->Load(pItem->m_strPath, pItem->m_musicInfoTag);
    m_tagReads++;
  }

  m_strPrevPath = strPath;
  return true;
}

void CMusicInfoLoader::OnLoaderFinish()
{
  // clear precached album thumbs
  g_directoryCache.ClearMusicThumbCache();

  // cleanup last loaded songs from database
  m_songsMap.Clear();

  // cleanup cache loaded from HD
  m_mapFileItems.Clear();

  // Save loaded items to HD
  if (!m_strCacheFileName.IsEmpty())
    SaveCache(m_strCacheFileName, *m_pVecItems);
  else if (!m_bStop && (m_databaseHits > 1 || m_tagReads > 0))
    m_pVecItems->Save();

  m_musicDatabase.Close();
}

void CMusicInfoLoader::UseCacheOnHD(const CStdString& strFileName)
{
  m_strCacheFileName = strFileName;
}

void CMusicInfoLoader::LoadCache(const CStdString& strFileName, CFileItemList& items)
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
      items.Add(pItem);
    }
    ar.Close();
    file.Close();
    items.SetFastLookup(true);
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
