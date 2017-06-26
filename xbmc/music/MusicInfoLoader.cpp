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

#include "MusicInfoLoader.h"
#include "ServiceBroker.h"
#include "MusicDatabase.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "utils/URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/Archive.h"
#include "Artist.h"
#include "Album.h"
#include "MusicThumbLoader.h"

using namespace XFILE;
using namespace MUSIC_INFO;

// HACK until we make this threadable - specify 1 thread only for now
CMusicInfoLoader::CMusicInfoLoader()
  : CBackgroundInfoLoader()
  , m_databaseHits{0}
  , m_tagReads{0}
{
  m_mapFileItems = new CFileItemList;

  m_thumbLoader = new CMusicThumbLoader();
}

CMusicInfoLoader::~CMusicInfoLoader()
{
  StopThread();
  if (m_musicDatabase.IsOpen())
    m_musicDatabase.Close();
  delete m_mapFileItems;
  delete m_thumbLoader;
}

void CMusicInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  if (!m_strCacheFileName.empty())
    LoadCache(m_strCacheFileName, *m_mapFileItems);
  else
  {
    m_mapFileItems->SetPath(m_pVecItems->GetPath());
    m_mapFileItems->Load();
    m_mapFileItems->SetFastLookup(true);
  }

  m_strPrevPath.clear();

  m_databaseHits = m_tagReads = 0;

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());

  m_musicDatabase.Open();

  if (m_thumbLoader)
    m_thumbLoader->OnLoaderStart();
}

bool CMusicInfoLoader::LoadAdditionalTagInfo(CFileItem* pItem)
{
  if (!pItem || (pItem->m_bIsFolder && !pItem->IsAudio()) ||
      pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (pItem->GetProperty("hasfullmusictag") == "true")
    return false; // already have the information

  std::string path(pItem->GetPath());
  if (pItem->IsMusicDb())
  {
    // set the artist / album properties
    XFILE::MUSICDATABASEDIRECTORY::CQueryParams param;
    XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->GetPath(),param);
    CArtist artist;
    CMusicDatabase database;
    database.Open();
    if (database.GetArtist(param.GetArtistId(), artist, false))
      CMusicDatabase::SetPropertiesFromArtist(*pItem,artist);

    CAlbum album;
    if (database.GetAlbum(param.GetAlbumId(), album, false))
      CMusicDatabase::SetPropertiesFromAlbum(*pItem,album);
    
    database.Close();

    path = pItem->GetMusicInfoTag()->GetURL();
  }

  CLog::Log(LOGDEBUG, "Loading additional tag info for file %s", path.c_str());

  // we load up the actual tag for this file in order to
  // fetch the lyrics and add it to the current music info tag
  CFileItem tempItem(path, false);
  std::unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(tempItem));
  if (NULL != pLoader.get())
  {
    CMusicInfoTag tag;
    pLoader->Load(path, tag);
    pItem->GetMusicInfoTag()->SetLyrics(tag.GetLyrics());
    pItem->SetProperty("hasfullmusictag", "true");
    return true;
  }
  return false;
}

bool CMusicInfoLoader::LoadItem(CFileItem* pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CMusicInfoLoader::LoadItemCached(CFileItem* pItem)
{
  if ((pItem->m_bIsFolder && !pItem->IsAudio()) ||
       pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  // Get thumb for item
  m_thumbLoader->LoadItem(pItem);

  return true;
}

bool CMusicInfoLoader::LoadItemLookup(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->m_bIsFolder)
    m_pProgressCallback->SetProgressAdvance();

  if ((pItem->m_bIsFolder && !pItem->IsAudio()) || pItem->IsPlayList() ||
       pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (!pItem->HasMusicInfoTag() || !pItem->GetMusicInfoTag()->Loaded())
  {
    // first check the cached item
    CFileItemPtr mapItem = (*m_mapFileItems)[pItem->GetPath()];
    if (mapItem && mapItem->m_dateTime==pItem->m_dateTime && mapItem->HasMusicInfoTag() && mapItem->GetMusicInfoTag()->Loaded())
    { // Query map if we previously cached the file on HD
      *pItem->GetMusicInfoTag() = *mapItem->GetMusicInfoTag();
      if (mapItem->HasArt("thumb"))
        pItem->SetArt("thumb", mapItem->GetArt("thumb"));
    }
    else
    {
      std::string strPath = URIUtils::GetDirectory(pItem->GetPath());
      URIUtils::AddSlashAtEnd(strPath);
      if (strPath!=m_strPrevPath)
      {
        // The item is from another directory as the last one,
        // query the database for the new directory...
        m_musicDatabase.GetSongsByPath(strPath, m_songsMap);
        m_databaseHits++;
      }

      /* Note for songs from embedded or separate cuesheets strFileName is not unique, so only the first song from such a file 
         gets added to the song map. Any such songs from a cuesheet can be identified by having a non-zero offset value.
         When the item we are looking up has a cue document or is a music file with a cuesheet embedded in the tags, it needs 
         to have the cuesheet fully processed replacing that item with items for every track etc. This is done elsewhere, as 
         changes to the list of items is not possible from here. This method only loads the item with the song from the database 
         when it maps to a single song.
      */

      MAPSONGS::iterator it = m_songsMap.find(pItem->GetPath());
      if (it != m_songsMap.end() && !pItem->HasCueDocument() && it->second.iStartOffset == 0 && it->second.iEndOffset == 0)
      {  // Have we loaded this item from database before (and it is not a cuesheet nor has an embedded cue sheet)
        pItem->GetMusicInfoTag()->SetSong(it->second);
        if (!it->second.strThumb.empty())
          pItem->SetArt("thumb", it->second.strThumb);
      }
      else if (pItem->IsMusicDb())
      { // a music db item that doesn't have tag loaded - grab details from the database
        XFILE::MUSICDATABASEDIRECTORY::CQueryParams param;
        XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->GetPath(),param);
        CSong song;
        if (m_musicDatabase.GetSong(param.GetSongId(), song))
        {
          pItem->GetMusicInfoTag()->SetSong(song);
          if (!song.strThumb.empty())
            pItem->SetArt("thumb", song.strThumb);
        }
      }
      else if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICFILES_USETAGS) || pItem->IsCDDA())
      { // Nothing found, load tag from file,
        // always try to load cddb info
        // get correct tag parser
        std::unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(*pItem));
        if (NULL != pLoader.get())
          // get tag
          pLoader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag());
        m_tagReads++;
      }

      m_strPrevPath = strPath;
    }
  }

  return true;
}

void CMusicInfoLoader::OnLoaderFinish()
{
  // cleanup last loaded songs from database
  m_songsMap.clear();

  // cleanup cache loaded from HD
  m_mapFileItems->Clear();

  // Save loaded items to HD
  if (!m_strCacheFileName.empty())
    SaveCache(m_strCacheFileName, *m_pVecItems);
  else if (!m_bStop && (m_databaseHits > 1 || m_tagReads > 0))
    m_pVecItems->Save();

  m_musicDatabase.Close();

  if (m_thumbLoader)
    m_thumbLoader->OnLoaderFinish();
}

void CMusicInfoLoader::UseCacheOnHD(const std::string& strFileName)
{
  m_strCacheFileName = strFileName;
}

void CMusicInfoLoader::LoadCache(const std::string& strFileName, CFileItemList& items)
{
  CFile file;

  if (file.Open(strFileName))
  {
    CArchive ar(&file, CArchive::load);
    int iSize = 0;
    ar >> iSize;
    for (int i = 0; i < iSize; i++)
    {
      CFileItemPtr pItem(new CFileItem());
      ar >> *pItem;
      items.Add(pItem);
    }
    ar.Close();
    file.Close();
    items.SetFastLookup(true);
  }
}

void CMusicInfoLoader::SaveCache(const std::string& strFileName, CFileItemList& items)
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
      CFileItemPtr pItem = items[i];
      ar << *pItem;
    }
    ar.Close();
    file.Close();
  }

}
