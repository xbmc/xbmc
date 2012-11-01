/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MusicInfoLoader.h"
#include "MusicDatabase.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "utils/URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "Artist.h"
#include "Album.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

// HACK until we make this threadable - specify 1 thread only for now
CMusicInfoLoader::CMusicInfoLoader() : CBackgroundInfoLoader(1)
{
  m_mapFileItems = new CFileItemList;
}

CMusicInfoLoader::~CMusicInfoLoader()
{
  StopThread();
  delete m_mapFileItems;
}

void CMusicInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  if (!m_strCacheFileName.IsEmpty())
    LoadCache(m_strCacheFileName, *m_mapFileItems);
  else
  {
    m_mapFileItems->SetPath(m_pVecItems->GetPath());
    m_mapFileItems->Load();
    m_mapFileItems->SetFastLookup(true);
  }

  m_strPrevPath.Empty();

  m_databaseHits = m_tagReads = 0;

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());

  m_musicDatabase.Open();
}

bool CMusicInfoLoader::LoadAdditionalTagInfo(CFileItem* pItem)
{
  if (!pItem || pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (pItem->GetProperty("hasfullmusictag") == "true")
    return false; // already have the information

  CStdString path(pItem->GetPath());
  if (pItem->IsMusicDb())
  {
    // set the artist / album properties
    XFILE::MUSICDATABASEDIRECTORY::CQueryParams param;
    XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->GetPath(),param);
    CArtist artist;
    CMusicDatabase database;
    database.Open();
    if (database.GetArtistInfo(param.GetArtistId(),artist,false))
      CMusicDatabase::SetPropertiesFromArtist(*pItem,artist);

    CAlbum album;
    if (database.GetAlbumInfo(param.GetAlbumId(),album,NULL))
      CMusicDatabase::SetPropertiesFromAlbum(*pItem,album);

    path = pItem->GetMusicInfoTag()->GetURL();
  }

  CLog::Log(LOGDEBUG, "Loading additional tag info for file %s", path.c_str());

  // we load up the actual tag for this file
  auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(path));
  if (NULL != pLoader.get())
  {
    CMusicInfoTag tag;
    pLoader->Load(path, tag);
    // then we set the fields from the file tags to the item
    pItem->SetProperty("lyrics", tag.GetLyrics());
    pItem->SetProperty("hasfullmusictag", "true");
    return true;
  }
  return false;
}

bool CMusicInfoLoader::LoadItem(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->m_bIsFolder)
    m_pProgressCallback->SetProgressAdvance();

  if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
    return false;

  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
    return true;

  // first check the cached item
  CFileItemPtr mapItem = (*m_mapFileItems)[pItem->GetPath()];
  if (mapItem && mapItem->m_dateTime==pItem->m_dateTime && mapItem->HasMusicInfoTag() && mapItem->GetMusicInfoTag()->Loaded())
  { // Query map if we previously cached the file on HD
    *pItem->GetMusicInfoTag() = *mapItem->GetMusicInfoTag();
    pItem->SetArt("thumb", mapItem->GetArt("thumb"));
    return true;
  }

  CStdString strPath;
  URIUtils::GetDirectory(pItem->GetPath(), strPath);
  URIUtils::AddSlashAtEnd(strPath);
  if (strPath!=m_strPrevPath)
  {
    // The item is from another directory as the last one,
    // query the database for the new directory...
    m_musicDatabase.GetSongsByPath(strPath, m_songsMap);
    m_databaseHits++;
  }

  CSong *song=NULL;

  if ((song=m_songsMap.Find(pItem->GetPath()))!=NULL)
  {  // Have we loaded this item from database before
    pItem->GetMusicInfoTag()->SetSong(*song);
    pItem->SetArt("thumb", song->strThumb);
  }
  else if (pItem->IsMusicDb())
  { // a music db item that doesn't have tag loaded - grab details from the database
    XFILE::MUSICDATABASEDIRECTORY::CQueryParams param;
    XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(pItem->GetPath(),param);
    CSong song;
    if (m_musicDatabase.GetSongById(param.GetSongId(), song))
    {
      pItem->GetMusicInfoTag()->SetSong(song);
      pItem->SetArt("thumb", song.strThumb);
    }
  }
  else if (g_guiSettings.GetBool("musicfiles.usetags") || pItem->IsCDDA())
  { // Nothing found, load tag from file,
    // always try to load cddb info
    // get correct tag parser
    auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->GetPath()));
    if (NULL != pLoader.get())
      // get tag
      pLoader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag());
    m_tagReads++;
  }

  m_strPrevPath = strPath;
  return true;
}

void CMusicInfoLoader::OnLoaderFinish()
{
  // cleanup last loaded songs from database
  m_songsMap.Clear();

  // cleanup cache loaded from HD
  m_mapFileItems->Clear();

  if (!m_bStop)
  { // check for art
    VECSONGS songs;
    songs.reserve(m_pVecItems->Size());
    for (int i = 0; i < m_pVecItems->Size(); ++i)
    {
      CFileItemPtr pItem = m_pVecItems->Get(i);
      if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
        continue;
      if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
      {
        CSong song(*pItem->GetMusicInfoTag());
        song.strThumb = pItem->GetArt("thumb");
        song.idSong = i; // for the lookup below
        songs.push_back(song);
      }
    }
    VECALBUMS albums;
    CMusicInfoScanner::CategoriseAlbums(songs, albums);
    CMusicInfoScanner::FindArtForAlbums(albums, m_pVecItems->GetPath());
    for (VECALBUMS::iterator i = albums.begin(); i != albums.end(); ++i)
    {
      string albumArt = i->art["thumb"];
      for (VECSONGS::iterator j = i->songs.begin(); j != i->songs.end(); ++j)
      {
        if (!j->strThumb.empty())
          m_pVecItems->Get(j->idSong)->SetArt("thumb", j->strThumb);
        else
          m_pVecItems->Get(j->idSong)->SetArt("thumb", albumArt);
      }
    }
  }

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
      CFileItemPtr pItem(new CFileItem());
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
      CFileItemPtr pItem = items[i];
      ar << *pItem;
    }
    ar.Close();
    file.Close();
  }

}
