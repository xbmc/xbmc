/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MusicInfoLoader.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/DirectoryCache.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "Util.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace MUSIC_INFO;

CMusicInfoLoader::CMusicInfoLoader()
{
  StopThread();
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

  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
    return true;

  CFileItem* mapItem=NULL;
  // first check the cached item
  if ((mapItem=m_mapFileItems[pItem->m_strPath])!=NULL && mapItem->m_dateTime==pItem->m_dateTime && mapItem->HasMusicInfoTag() && mapItem->GetMusicInfoTag()->Loaded())
  { // Query map if we previously cached the file on HD
    *pItem->GetMusicInfoTag() = *mapItem->GetMusicInfoTag();
    pItem->SetThumbnailImage(mapItem->GetThumbnailImage());
    return true;
  }

  CStdString strPath;
  CUtil::GetDirectory(pItem->m_strPath, strPath);
  CUtil::AddSlashAtEnd(strPath);
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
    pItem->GetMusicInfoTag()->SetSong(*song);
    pItem->SetThumbnailImage(song->strThumb);
  }
  else if (pItem->IsMusicDb())
  { // a music db item that doesn't have tag loaded - grab details from the database
    DIRECTORY::MUSICDATABASEDIRECTORY::CQueryParams param;
    DIRECTORY::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->m_strPath,param);
    CSong song;
    if (m_musicDatabase.GetSongById(param.GetSongId(), song))
    {
      pItem->GetMusicInfoTag()->SetSong(song);
      pItem->SetThumbnailImage(song.strThumb);
    }
  }
  else if (g_guiSettings.GetBool("musicfiles.usetags") || pItem->IsCDDA())
  { // Nothing found, load tag from file,
    // always try to load cddb info
    // get correct tag parser
    auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->m_strPath));
    if (NULL != pLoader.get())
      // get tag
      pLoader->Load(pItem->m_strPath, *pItem->GetMusicInfoTag());
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
