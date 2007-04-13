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
#include "MusicInfoScanner.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "FileSystem/DirectoryCache.h"
#include "Util.h"


using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoScanner::CMusicInfoScanner()
{
  m_bRunning = false;
  m_pObserver = NULL;
  m_bCanInterrupt = false;
  m_currentItem=0;
  m_itemCount=0;
}

CMusicInfoScanner::~CMusicInfoScanner()
{
}

void CMusicInfoScanner::Process()
{
  try
  {
    DWORD dwTick = timeGetTime();

    m_musicDatabase.Open();

    if (m_pObserver)
      m_pObserver->OnStateChanged(PREPARING);

    m_bCanInterrupt = true;

    // check whether we have scanned here before
    CStdString strPaths;
    if (!m_musicDatabase.GetSubpathsFromPath(m_strStartDir, strPaths))
    {
      m_musicDatabase.Close();
      return ;
    }

    CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

    CLog::Log(LOGDEBUG, __FUNCTION__" - Starting scan");
    m_musicDatabase.BeginTransaction();

    bool bOKtoScan = true;
    if (m_bUpdateAll)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(REMOVING_OLD);

      bOKtoScan = m_musicDatabase.RemoveSongsFromPaths(strPaths);
      if (bOKtoScan)
      {
        if (m_pObserver)
          m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

        bOKtoScan = m_musicDatabase.CleanupAlbumsArtistsGenres();
      }
    }

    if (bOKtoScan)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      CThread fileCountReader(this);
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCommit = false;
      if (bOKtoScan)
        bCommit = DoScan(m_strStartDir);

      if (bCommit)
      {
        m_musicDatabase.CommitTransaction();

        if (m_bUpdateAll)
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

          m_musicDatabase.Compress();
        }
      }
      else
        m_musicDatabase.RollbackTransaction();

      fileCountReader.StopThread();

    }
    else
      m_musicDatabase.RollbackTransaction();

    m_musicDatabase.EmptyCache();

    CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

    m_musicDatabase.Close();
    CLog::Log(LOGDEBUG, __FUNCTION__" - Finished scan");

    dwTick = timeGetTime() - dwTick;
    CStdString strTmp, strTmp1;
    StringUtils::SecondsToTimeString(dwTick / 1000, strTmp1);
    strTmp.Format("My Music: Scanning for music info using worker thread, operation took %s", strTmp1);
    CLog::Log(LOGNOTICE, strTmp.c_str());

    m_bRunning = false;
    if (m_pObserver)
      m_pObserver->OnFinished();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception will scanning.");
  }
}

void CMusicInfoScanner::Start(const CStdString& strDirectory, bool bUpdateAll)
{
  m_strStartDir = strDirectory;
  m_bUpdateAll = bUpdateAll;
  StopThread();
  Create();
  m_bRunning = true;
}

bool CMusicInfoScanner::IsScanning()
{
  return m_bRunning;
}

void CMusicInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_musicDatabase.Interupt();

  StopThread();
}

void CMusicInfoScanner::SetObserver(IMusicInfoScannerObserver* pObserver)
{
  m_pObserver = pObserver;
}

bool CMusicInfoScanner::DoScan(const CStdString& strDirectory)
{
  if (m_pObserver)
    m_pObserver->OnDirectoryChanged(strDirectory);

  CLog::Log(LOGDEBUG, __FUNCTION__" - Scanning dir: %s", strDirectory.c_str());
  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_stSettings.m_musicExtensions);
  // filter items in the sub dir (for .cue sheet support)
  items.FilterCueItems();
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  // get the folder's thumb (this will cache the album thumb)
  items.SetMusicThumb(true); // true forces it to get a remote thumb

  if (RetrieveMusicInfo(items, strDirectory) > 0)
  {
    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - Finished dir: %s", strDirectory.c_str());

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem *pItem = items[i];

    if (m_bStop)
      break;
    // if we have a directory item (non-playlist) we then recurse into that folder
    if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList())
    {
      CStdString strPath=pItem->m_strPath;
      if (!DoScan(strPath))
      {
        m_bStop = true;
      }
    }
  }

  return !m_bStop;
}

int CMusicInfoScanner::RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory)
{
  int nFolderCount = items.GetFolderCount();
  // Skip items with folders only
  if (nFolderCount == items.Size())
    return 0;

  int nFileCount = items.Size() - nFolderCount;

  CStdString strItem;
  CSongMap songsMap;
  // get all information for all files in current directory from database
  m_musicDatabase.GetSongsByPath(strDirectory, songsMap);

  VECSONGS songsToAdd;
  // for every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    CStdString strExtension;
    CUtil::GetExtension(pItem->m_strPath, strExtension);

    if (m_bStop)
      return 0;

    // dont try reading id3tags for folders, playlists or shoutcast streams
    if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsShoutCast() )
    {
      m_currentItem++;
      // is tag for this file already loaded?
      CLog::Log(LOGDEBUG, __FUNCTION__" - Reading tag for: %s", pItem->m_strPath.c_str());
      bool bNewFile = false;
      CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
      if (!tag.Loaded() )
      {
        // no, then we gonna load it.
        // first search for file in our list of the current directory
        CSong *song = songsMap.Find(pItem->m_strPath);
        if (song)
        {
          tag.SetSong(*song);
        }
        else
        {
          // if id3 tag scanning is turned on OR we're scanning the directory
          // then parse id3tag from file
          // get correct tag parser
          auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->m_strPath));
          if (NULL != pLoader.get())
          {
            // get id3tag
            if ( pLoader->Load(pItem->m_strPath, tag))
            {
              bNewFile = true;
            }
          }
        }
      } //if (!tag.Loaded() )
      else
      {
        CSong *song = songsMap.Find(pItem->m_strPath);
        if (!song)
          bNewFile = true;
      }

      // if we have the itemcount, notify our
      // observer with the progress we made
      if (m_pObserver && m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);

      if (tag.Loaded() && bNewFile)
      {
        CSong song(tag);
        song.iStartOffset = pItem->m_lStartOffset;
        song.iEndOffset = pItem->m_lEndOffset;
        pItem->SetMusicThumb();
        song.strThumb = pItem->GetThumbnailImage();
        songsToAdd.push_back(song);
        CLog::Log(LOGDEBUG, __FUNCTION__" - Tag loaded for: %s", pItem->m_strPath.c_str());
      }
      else if (bNewFile)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - No tag found for: %s", pItem->m_strPath.c_str());
      }
    } //if (!pItem->m_bIsFolder)
  }

  CheckForVariousArtists(songsToAdd);
  if (!items.HasThumbnail())
    UpdateFolderThumb(songsToAdd, items.m_strPath);

  // finally, add these to the database
  for (unsigned int i = 0; i < songsToAdd.size(); ++i)
  {
    if (m_bStop) return i;
    CSong &song = songsToAdd[i];
    m_musicDatabase.AddSong(song, false);
  }
  return songsToAdd.size();
}

void CMusicInfoScanner::CheckForVariousArtists(VECSONGS &songsToCheck)
{
  // first, find all the album names for these songs
  map<CStdString, vector<CSong *> > albumsToAdd;
  map<CStdString, vector<CSong *> >::iterator it;
  for (unsigned int i = 0; i < songsToCheck.size(); ++i)
  {
    CSong &song = songsToCheck[i];
    if (!song.strAlbumArtist.IsEmpty()) // albumartist specified, so assume the user knows what they're doing
      continue;
    it = albumsToAdd.find(song.strAlbum);
    if (it == albumsToAdd.end())
    {
      vector<CSong *> songs;
      songs.push_back(&song);
      albumsToAdd.insert(make_pair(song.strAlbum, songs));
    }
    else
      it->second.push_back(&song);
  }
  // ok, now run through these albums, and check whether they qualify as a "various artist" album
  // an album is considered a various artists album if the songs' primary artist differs
  // it qualifies as a "single artist with featured artists" album if the primary artist is the same, but secondary artists differ
  for (it = albumsToAdd.begin(); it != albumsToAdd.end(); it++)
  {
    const CStdString &album = it->first;
    vector<CSong *> &songs = it->second;
    if (!album.IsEmpty() && songs.size() > 1)
    {
      bool variousArtists(false);
      bool singleArtistWithFeaturedArtists(false);
      for (unsigned int i = 0; i < songs.size() - 1; i++)
      {
        CSong *song1 = songs[i];
        CSong *song2 = songs[i+1];
        CStdStringArray vecArtists1, vecArtists2;
        StringUtils::SplitString(song1->strArtist, " / ", vecArtists1);
        StringUtils::SplitString(song2->strArtist, " / ", vecArtists2);
        CStdString primaryArtist1 = vecArtists1[0]; primaryArtist1.TrimRight();
        CStdString primaryArtist2 = vecArtists2[0]; primaryArtist2.TrimRight();
        if (primaryArtist1 != primaryArtist2)
        { // primary artist differs -> a various artists album
          variousArtists = true;
          break;
        }
        else if (song1->strArtist != song2->strArtist)
        { // have more than one artist, the first artist(s) agree, but the full artist name doesn't
          // so this is likely a single-artist compilation (ie with other artists featured on some tracks) album
          singleArtistWithFeaturedArtists = true;
        }
      }
      if (variousArtists)
      { // have a various artists album - update all songs to be the various artist
        for (unsigned int i = 0; i < songs.size(); i++)
        {
          CSong *song = songs[i];
          song->strAlbumArtist = g_localizeStrings.Get(340); // Various Artists
        }
      }
      else if (singleArtistWithFeaturedArtists)
      { // have an album where all the first artists agree - make this the album artist
        CStdStringArray vecArtists;
        StringUtils::SplitString(songs[0]->strArtist, " / ", vecArtists);
        CStdString albumArtist(vecArtists[0]);
        for (unsigned int i = 0; i < songs.size(); i++)
        {
          CSong *song = songs[i];
          song->strAlbumArtist = albumArtist; // first artist of all tracks
        }
      }
    }
  }
}

void CMusicInfoScanner::UpdateFolderThumb(VECSONGS &songs, const CStdString &folderPath)
{
  // check how many unique albums are in this path, and if there's only one, and it has a thumb
  // then cache the thumb as the folder thumb
  CStdString album, artist;
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    CSong &song = songs[i];
    // don't bother with empty album tags - they're unlikely to have an embedded thumb anyway,
    // and if one is not tagged correctly, how can we know whether there is only one album?
    if (song.strAlbum.IsEmpty())
      return;

    CStdString albumArtist = song.strAlbumArtist.IsEmpty() ? song.strArtist : song.strAlbumArtist;

    if (!album.IsEmpty() && (album != song.strAlbum || artist != albumArtist))
      return; // have more than one album

    album = song.strAlbum;
    artist = albumArtist;
  }
  if (album.IsEmpty()) return;

  // Was the album art of this album read during scan?
  CStdString albumCoverArt(CUtil::GetCachedAlbumThumb(album, artist));
  if (CUtil::ThumbExists(albumCoverArt))
  {
    CStdString folderPath1(folderPath);
    CUtil::RemoveSlashAtEnd(folderPath1);
    CStdString folderCoverArt(CUtil::GetCachedMusicThumb(folderPath1));
    // copy as directory thumb as well
    if (::CopyFile(albumCoverArt, folderCoverArt, false))
      CUtil::ThumbCacheAdd(folderCoverArt, true);
  }
}

// This function is run by another thread
void CMusicInfoScanner::Run()
{
  m_itemCount=CountFiles(m_strStartDir);
}

// Recurse through all folders we scan and count files
int CMusicInfoScanner::CountFiles(const CStdString& strPath)
{
  int count=0;
  // load subfolder
  CFileItemList items;
  CLog::Log(LOGDEBUG, __FUNCTION__" - processing dir: %s", strPath.c_str());
  CDirectory::GetDirectory(strPath, items, g_stSettings.m_musicExtensions, false);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItem* pItem=items[i];

    if (m_bStop)
      return 0;

    if (pItem->m_bIsFolder)
      count+=CountFiles(pItem->m_strPath);
    else if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - finished processing dir: %s", strPath.c_str());
  return count;
}
