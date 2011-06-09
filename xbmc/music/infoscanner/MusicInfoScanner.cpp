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

#include "MusicInfoScanner.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "Util.h"
#include "utils/md5.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "NfoFile.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "pictures/Picture.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace MUSIC_GRABBER;

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
    unsigned int tick = CTimeUtils::GetTimeMS();

    m_musicDatabase.Open();

    if (m_pObserver)
      m_pObserver->OnStateChanged(PREPARING);

    m_bCanInterrupt = true;

    CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

    if (m_scanType == 0) // load info from files
    {
      CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);

      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      SetPriority( GetMinPriority() );
      CThread fileCountReader(this);
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;
      m_needsCleanup = false;

      bool commit = false;
      bool cancelled = false;
      while (!cancelled && m_pathsToScan.size())
      {
        /*
         * A copy of the directory path is used because the path supplied is
         * immediately removed from the m_pathsToScan set in DoScan(). If the
         * reference points to the entry in the set a null reference error
         * occurs.
         */
        CStdString directory = *m_pathsToScan.begin();
        if (!DoScan(directory))
          cancelled = true;
        commit = !cancelled;
      }

      if (commit)
      {
        g_infoManager.ResetPersistentCache();

        if (m_needsCleanup)
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

          m_musicDatabase.CleanupOrphanedItems();

          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

          m_musicDatabase.Compress(false);
        }
      }

      fileCountReader.StopThread();

      m_musicDatabase.EmptyCache();

      CUtil::ThumbCacheClear();
      g_directoryCache.ClearMusicThumbCache();

      m_musicDatabase.Close();
      CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);

      tick = CTimeUtils::GetTimeMS() - tick;
      CLog::Log(LOGNOTICE, "My Music: Scanning for music info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
    }
    bool bCanceled;
    if (m_scanType == 1) // load album info
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(DOWNLOADING_ALBUM_INFO);

      int iCurrentItem = 1;
      for (set<CAlbum>::iterator it=m_albumsToScan.begin();it != m_albumsToScan.end();++it)
      {
        if (m_pObserver)
        {
          m_pObserver->OnDirectoryChanged(it->strArtist+" - "+it->strAlbum);
          m_pObserver->OnSetProgress(iCurrentItem++, m_albumsToScan.size());
        }

        CMusicAlbumInfo albumInfo;
        DownloadAlbumInfo(it->strGenre,it->strArtist,it->strAlbum, bCanceled, albumInfo); // genre field holds path - see fetchalbuminfo()

        if (m_bStop || bCanceled)
          break;
      }
    }
    if (m_scanType == 2) // load artist info
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(DOWNLOADING_ARTIST_INFO);

      int iCurrentItem=1;
      for (set<CArtist>::iterator it=m_artistsToScan.begin();it != m_artistsToScan.end();++it)
      {
        if (m_pObserver)
        {
          m_pObserver->OnDirectoryChanged(it->strArtist);
          m_pObserver->OnSetProgress(iCurrentItem++, m_artistsToScan.size());
        }

        DownloadArtistInfo(it->strGenre,it->strArtist,bCanceled); // genre field holds path - see fetchartistinfo()

        if (m_bStop || bCanceled)
          break;
      }
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception while scanning.");
  }
  m_bRunning = false;
  if (m_pObserver)
    m_pObserver->OnFinished();
}

void CMusicInfoScanner::Start(const CStdString& strDirectory)
{
  m_pathsToScan.clear();
  m_albumsScanned.clear();
  m_artistsScanned.clear();

  if (strDirectory.IsEmpty())
  { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
    // we go.
    m_musicDatabase.Open();
    m_musicDatabase.GetPaths(m_pathsToScan);
    m_musicDatabase.Close();
  }
  else
    m_pathsToScan.insert(strDirectory);
  m_pathsToCount = m_pathsToScan;
  m_scanType = 0;
  StopThread();
  Create();
  m_bRunning = true;
}

void CMusicInfoScanner::FetchAlbumInfo(const CStdString& strDirectory)
{
  m_albumsToScan.clear();
  m_albumsScanned.clear();

  CFileItemList items;
  if (strDirectory.IsEmpty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetAlbumsNav("musicdb://3/",items,-1,-1,-1,-1);
    m_musicDatabase.Close();
  }
  else
  {
    if (URIUtils::HasSlashAtEnd(strDirectory)) // directory
      CDirectory::GetDirectory(strDirectory,items);
    else
    {
      CFileItemPtr item(new CFileItem(strDirectory,false));
      items.Add(item);
    }
  }

  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->m_strPath) || items[i]->IsParentFolder())
      continue;

    CAlbum album;
    album.strAlbum = items[i]->GetMusicInfoTag()->GetAlbum();
    album.strArtist = items[i]->GetMusicInfoTag()->GetArtist();
    album.strGenre = items[i]->m_strPath; // a bit hacky use of field
    m_albumsToScan.insert(album);
  }

  m_scanType = 1;
  StopThread();
  Create();
  m_bRunning = true;
}

void CMusicInfoScanner::FetchArtistInfo(const CStdString& strDirectory)
{
  m_artistsToScan.clear();
  m_artistsScanned.clear();
  CFileItemList items;

  if (strDirectory.IsEmpty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetArtistsNav("musicdb://2/",items,-1,false);
    m_musicDatabase.Close();
  }
  else
  {
    if (URIUtils::HasSlashAtEnd(strDirectory)) // directory
      CDirectory::GetDirectory(strDirectory,items);
    else
    {
      CFileItemPtr newItem(new CFileItem(strDirectory,false));
      items.Add(newItem);
    }
  }

  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->m_strPath) || items[i]->IsParentFolder())
      continue;

    CArtist artist;
    artist.strArtist = items[i]->GetMusicInfoTag()->GetArtist();
    artist.strGenre = items[i]->m_strPath; // a bit hacky use of field
    m_artistsToScan.insert(artist);
  }

  m_scanType = 2;
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

  /*
   * remove this path from the list we're processing. This must be done prior to
   * the check for file or folder exclusion to prevent an infinite while loop
   * in Process().
   */
  set<CStdString>::iterator it = m_pathsToScan.find(strDirectory);
  if (it != m_pathsToScan.end())
    m_pathsToScan.erase(it);

  // Discard all excluded files defined by m_musicExcludeRegExps

  CStdStringArray regexps = g_advancedSettings.AudioSettings->ExcludeFromScanRegExps();

  if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
    return true;

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_settings.m_musicExtensions + "|.jpg|.tbn|.lrc|.cdg");

  // sort and get the path hash.  Note that we don't filter .cue sheet items here as we want
  // to detect changes in the .cue sheet as well.  The .cue sheet items only need filtering
  // if we have a changed hash.
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  CStdString hash;
  GetPathHash(items, hash);

  // get the folder's thumb (this will cache the album thumb).
  items.SetMusicThumb(true); // true forces it to get a remote thumb

  // check whether we need to rescan or not
  CStdString dbHash;
  if (!m_musicDatabase.GetPathHash(strDirectory, dbHash) || dbHash != hash)
  { // path has changed - rescan
    if (dbHash.IsEmpty())
      CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, strDirectory.c_str());
    else
      CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, strDirectory.c_str());

    // filter items in the sub dir (for .cue sheet support)
    items.FilterCueItems();
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    // and then scan in the new information
    if (RetrieveMusicInfo(items, strDirectory) > 0)
    {
      if (m_pObserver)
        m_pObserver->OnDirectoryScanned(strDirectory);
    }

    // save information about this folder
    m_musicDatabase.SetPathHash(strDirectory, hash);
  }
  else
  { // path is the same - no need to rescan
    CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, strDirectory.c_str());
    m_currentItem += CountFiles(items, false);  // false for non-recursive

    // notify our observer of our progress
    if (m_pObserver)
    {
      if (m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
      m_pObserver->OnDirectoryScanned(strDirectory);
    }
  }

  // now scan the subfolders
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];

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
  CSongMap songsMap;

  // get all information for all files in current directory from database, and remove them
  if (m_musicDatabase.RemoveSongsFromPath(strDirectory, songsMap))
    m_needsCleanup = true;

  VECSONGS songsToAdd;

  CStdStringArray regexps = g_advancedSettings.AudioSettings->ExcludeFromScanRegExps();

  // for every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    CStdString strExtension;
    URIUtils::GetExtension(pItem->m_strPath, strExtension);

    if (m_bStop)
      return 0;

    // Discard all excluded files defined by m_musicExcludeRegExps
    if (CUtil::ExcludeFileOrFolder(pItem->m_strPath, regexps))
      continue;

    // dont try reading id3tags for folders, playlists or shoutcast streams
    if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsPicture() && !pItem->IsLyrics() )
    {
      m_currentItem++;
//      CLog::Log(LOGDEBUG, "%s - Reading tag for: %s", __FUNCTION__, pItem->m_strPath.c_str());

      // grab info from the song
      CSong *dbSong = songsMap.Find(pItem->m_strPath);

      CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
      if (!tag.Loaded() )
      { // read the tag from a file
        auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->m_strPath));
        if (NULL != pLoader.get())
          pLoader->Load(pItem->m_strPath, tag);
      }

      // if we have the itemcount, notify our
      // observer with the progress we made
      if (m_pObserver && m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);

      if (tag.Loaded())
      {
        CSong song(tag);

        // ensure our song has a valid filename or else it will assert in AddSong()
        if (song.strFileName.IsEmpty())
        {
          // copy filename from path in case UPnP or other tag loaders didn't specify one (FIXME?)
          song.strFileName = pItem->m_strPath;

          // if we still don't have a valid filename, skip the song
          if (song.strFileName.IsEmpty())
          {
            // this shouldn't ideally happen!
            CLog::Log(LOGERROR, "Skipping song since it doesn't seem to have a filename");
            continue;
          }
        }

        song.iStartOffset = pItem->m_lStartOffset;
        song.iEndOffset = pItem->m_lEndOffset;
        if (dbSong)
        { // keep the db-only fields intact on rescan...
          song.iTimesPlayed = dbSong->iTimesPlayed;
          song.lastPlayed = dbSong->lastPlayed;
          song.iKaraokeNumber = dbSong->iKaraokeNumber;

          if (song.rating == '0') song.rating = dbSong->rating;
        }
        pItem->SetMusicThumb();
        song.strThumb = pItem->GetThumbnailImage();
        songsToAdd.push_back(song);
//        CLog::Log(LOGDEBUG, "%s - Tag loaded for: %s", __FUNCTION__, pItem->m_strPath.c_str());
      }
      else
        CLog::Log(LOGDEBUG, "%s - No tag found for: %s", __FUNCTION__, pItem->m_strPath.c_str());
    }
  }

  CheckForVariousArtists(songsToAdd);
  if (!items.HasThumbnail())
    UpdateFolderThumb(songsToAdd, items.m_strPath);

  // finally, add these to the database
  set<CStdString> artistsToScan;
  set< pair<CStdString, CStdString> > albumsToScan;
  m_musicDatabase.BeginTransaction();
  for (unsigned int i = 0; i < songsToAdd.size(); ++i)
  {
    if (m_bStop)
    {
      m_musicDatabase.RollbackTransaction();
      return i;
    }
    CSong &song = songsToAdd[i];
    m_musicDatabase.AddSong(song, false);

    artistsToScan.insert(song.strArtist);
    albumsToScan.insert(make_pair(song.strAlbum, song.strArtist));
  }
  m_musicDatabase.CommitTransaction();

  bool bCanceled;
  for (set<CStdString>::iterator i = artistsToScan.begin(); i != artistsToScan.end(); ++i)
  {
    bCanceled = false;
    long iArtist = m_musicDatabase.GetArtistByName(*i);
    if (find(m_artistsScanned.begin(),m_artistsScanned.end(),iArtist) == m_artistsScanned.end())
    {
      m_artistsScanned.push_back(iArtist);
      if (!m_bStop && g_guiSettings.GetBool("musiclibrary.downloadinfo"))
      {
        CStdString strPath;
        strPath.Format("musicdb://2/%u/",iArtist);
        if (!DownloadArtistInfo(strPath,*i, bCanceled)) // assume we want to retry
          m_artistsScanned.pop_back();
      }
      else
        GetArtistArtwork(iArtist, *i);
    }
  }

  if (g_guiSettings.GetBool("musiclibrary.downloadinfo"))
  {
    for (set< pair<CStdString, CStdString> >::iterator i = albumsToScan.begin(); i != albumsToScan.end(); ++i)
    {
      if (m_bStop)
        return songsToAdd.size();

      long iAlbum = m_musicDatabase.GetAlbumByName(i->first, i->second);
      CStdString strPath;
      strPath.Format("musicdb://3/%u/",iAlbum);

      bCanceled = false;
      if (find(m_albumsScanned.begin(), m_albumsScanned.end(), iAlbum) == m_albumsScanned.end())
      {
        CMusicAlbumInfo albumInfo;
        if (DownloadAlbumInfo(strPath, i->second, i->first, bCanceled, albumInfo))
          m_albumsScanned.push_back(iAlbum);
      }
    }
  }
  if (m_pObserver)
    m_pObserver->OnStateChanged(READING_MUSIC_INFO);

  return songsToAdd.size();
}

static bool SortSongsByTrack(CSong *song, CSong *song2)
{
  return song->iTrack < song2->iTrack;
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
  // as an additional check for those that have multiple albums in the same folder, ignore albums
  // that have overlapping track numbers
  for (it = albumsToAdd.begin(); it != albumsToAdd.end();)
  {
    vector<CSong *> &songs = it->second;
    bool overlappingTrackNumbers(false);
    if (songs.size() > 1)
    {
      sort(songs.begin(), songs.end(), SortSongsByTrack);
      for (unsigned int i = 0; i < songs.size() - 1; i++)
      {
        CSong *song = songs[i];
        CSong *song2 = songs[i+1];
        if (song->iTrack == song2->iTrack)
        {
          overlappingTrackNumbers = true;
          break;
        }
      }
    }
    if (overlappingTrackNumbers)
    { // remove this album
      albumsToAdd.erase(it++);
    }
    else
      it++;
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
        StringUtils::SplitString(song1->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists1);
        StringUtils::SplitString(song2->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists2);
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
        StringUtils::SplitString(songs[0]->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists);
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

bool CMusicInfoScanner::HasSingleAlbum(const VECSONGS &songs, CStdString &album, CStdString &artist)
{
  // check how many unique albums are in this path, and if there's only one, and it has a thumb
  // then cache the thumb as the folder thumb
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong &song = songs[i];
    // don't bother with empty album tags - they're treated as singles, and there's no way to determine
    // whether more than one track in the folder is supposed to mean they belong to an "album"
    if (song.strAlbum.IsEmpty())
      return false;

    CStdString albumArtist = song.strAlbumArtist.IsEmpty() ? song.strArtist : song.strAlbumArtist;

    if (!album.IsEmpty() && (album != song.strAlbum || artist != albumArtist))
      return false; // have more than one album

    album = song.strAlbum;
    artist = albumArtist;
  }
  return !album.IsEmpty();
}

void CMusicInfoScanner::UpdateFolderThumb(const VECSONGS &songs, const CStdString &folderPath)
{
  CStdString album, artist;
  if (!HasSingleAlbum(songs, album, artist)) return;
  // Was the album art of this album read during scan?
  CStdString albumCoverArt(CUtil::GetCachedAlbumThumb(album, artist));
  if (CUtil::ThumbExists(albumCoverArt))
  {
    CStdString folderPath1(folderPath);
    // Folder art is cached without the slash at end
    URIUtils::RemoveSlashAtEnd(folderPath1);
    CStdString folderCoverArt(CUtil::GetCachedMusicThumb(folderPath1));
    // copy as directory thumb as well
    if (CFile::Cache(albumCoverArt, folderCoverArt))
      CUtil::ThumbCacheAdd(folderCoverArt, true);
  }
}

// This function is run by another thread
void CMusicInfoScanner::Run()
{
  int count = 0;
  while (!m_bStop && m_pathsToCount.size())
    count+=CountFilesRecursively(*m_pathsToCount.begin());
  m_itemCount = count;
}

// Recurse through all folders we scan and count files
int CMusicInfoScanner::CountFilesRecursively(const CStdString& strPath)
{
  // load subfolder
  CFileItemList items;
//  CLog::Log(LOGDEBUG, __FUNCTION__" - processing dir: %s", strPath.c_str());
  CDirectory::GetDirectory(strPath, items, g_settings.m_musicExtensions, false);

  if (m_bStop)
    return 0;

  // true for recursive counting
  int count = CountFiles(items, true);

  // remove this path from the list we're processing
  set<CStdString>::iterator it = m_pathsToCount.find(strPath);
  if (it != m_pathsToCount.end())
    m_pathsToCount.erase(it);

//  CLog::Log(LOGDEBUG, __FUNCTION__" - finished processing dir: %s", strPath.c_str());
  return count;
}

int CMusicInfoScanner::CountFiles(const CFileItemList &items, bool recursive)
{
  int count = 0;
  for (int i=0; i<items.Size(); ++i)
  {
    const CFileItemPtr pItem=items[i];

    if (recursive && pItem->m_bIsFolder)
      count+=CountFilesRecursively(pItem->m_strPath);
    else if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  return count;
}

int CMusicInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
{
  // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
  if (0 == items.Size()) return 0;
  XBMC::XBMC_MD5 md5state;
  int count = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr pItem = items[i];
    md5state.append(pItem->m_strPath);
    md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
    FILETIME time = pItem->m_dateTime;
    md5state.append((unsigned char *)&time, sizeof(FILETIME));
    if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  md5state.getDigest(hash);
  return count;
}

#define THRESHOLD .95f

bool CMusicInfoScanner::DownloadAlbumInfo(const CStdString& strPath, const CStdString& strArtist, const CStdString& strAlbum, bool& bCanceled, CMusicAlbumInfo& albumInfo, CGUIDialogProgress* pDialog)
{
  CAlbum album;
  VECSONGS songs;
  XFILE::MUSICDATABASEDIRECTORY::CQueryParams params;
  XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strPath, params);
  bCanceled = false;
  m_musicDatabase.Open();
  if (m_musicDatabase.HasAlbumInfo(params.GetAlbumId()) && m_musicDatabase.GetAlbumInfo(params.GetAlbumId(),album,&songs))
    return true;

  // find album info
  ADDON::ScraperPtr info;
  if (!m_musicDatabase.GetScraperForPath(strPath, info, ADDON::ADDON_SCRAPER_ALBUMS) || !info)
  {
    m_musicDatabase.Close();
    return false;
  }

  if (m_pObserver)
  {
    m_pObserver->OnStateChanged(DOWNLOADING_ALBUM_INFO);
    m_pObserver->OnDirectoryChanged(strAlbum);
  }

  // clear our scraper cache
  info->ClearCache();

  CMusicInfoScraper scraper(info);

  // handle nfo files
  CStdString strAlbumPath, strNfo;
  m_musicDatabase.GetAlbumPath(params.GetAlbumId(),strAlbumPath);
  URIUtils::AddFileToFolder(strAlbumPath,"album.nfo",strNfo);
  CNfoFile::NFOResult result=CNfoFile::NO_NFO;
  CNfoFile nfoReader;
  if (XFILE::CFile::Exists(strNfo))
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
    result = nfoReader.Create(strNfo, info, -1, strPath);
    if (result == CNfoFile::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
      CAlbum album;
      nfoReader.GetDetails(album);
      m_musicDatabase.SetAlbumInfo(params.GetAlbumId(), album, album.songs);
      GetAlbumArtwork(params.GetAlbumId(), album);
      m_musicDatabase.Close();
      return true;
    }
    else if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicAlbumInfo album("nfo",scrUrl);
      info = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",info->Name().c_str());
      CLog::Log(LOGDEBUG,"-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
      scraper.SetScraperInfo(info);
      scraper.GetAlbums().push_back(album);
    }
    else
      CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.CheckValidOrFallback(g_guiSettings.GetString("musiclibrary.albumsscraper")))
  { // the current scraper is invalid, as is the default - bail
    CLog::Log(LOGERROR, "%s - current and default scrapers are invalid.  Pick another one", __FUNCTION__);
    return false;
  }

  if (!scraper.GetAlbumCount())
    scraper.FindAlbumInfo(strAlbum, strArtist);

  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      bCanceled = true;
    }
    Sleep(1);
  }

  CGUIDialogSelect *pDlg=NULL;
  int iSelectedAlbum=0;
  if (result == CNfoFile::NO_NFO)
  {
    iSelectedAlbum = -1; // set negative so that we can detect a failure
    if (scraper.Succeeded() && scraper.GetAlbumCount() >= 1)
    {
      int bestMatch = -1;
      double bestRelevance = 0;
      double minRelevance = THRESHOLD;
      if (scraper.GetAlbumCount() > 1) // score the matches
      {
        //show dialog with all albums found
        if (pDialog)
        {
          pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
          pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
          pDlg->Reset();
          pDlg->EnableButton(true, 413); // manual
        }

        for (int i = 0; i < scraper.GetAlbumCount(); ++i)
        {
          CMusicAlbumInfo& info = scraper.GetAlbum(i);
          double relevance = info.GetRelevance();
          if (relevance < 0)
            relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, info.GetAlbum().strArtist, strArtist);

          // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
          // otherwise, perfect matches only
          if (relevance >= max(minRelevance, bestRelevance))
          { // we auto-select the best of these
            bestRelevance = relevance;
            bestMatch = i;
          }
          if (pDialog)
          {
            // set the label to [relevance]  album - artist
            CStdString strTemp;
            strTemp.Format("[%0.2f]  %s", relevance, info.GetTitle2());
            CFileItem item(strTemp);
            item.m_idepth = i; // use this to hold the index of the album in the scraper
            pDlg->Add(&item);
          }
          if (relevance > .99f) // we're so close, no reason to search further
            break;
        }
      }
      else
      {
        CMusicAlbumInfo& info = scraper.GetAlbum(0);
        double relevance = info.GetRelevance();
        if (relevance < 0)
          relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, info.GetAlbum().strArtist, strArtist);
        if (relevance < THRESHOLD)
        {
          m_musicDatabase.Close();
          return false;
        }
        bestRelevance = relevance;
        bestMatch = 0;
      }

      iSelectedAlbum = bestMatch;
      if (pDialog && bestRelevance < THRESHOLD)
      {
        pDlg->Sort(false);
        pDlg->DoModal();

        // and wait till user selects one
        if (pDlg->GetSelectedLabel() < 0)
        { // none chosen
          if (!pDlg->IsButtonPressed())
          {
            bCanceled = true;
            return false;
          }
          // manual button pressed
          CStdString strNewAlbum = strAlbum;
          if (!CGUIDialogKeyboard::ShowAndGetInput(strNewAlbum, g_localizeStrings.Get(16011), false)) return false;
          if (strNewAlbum == "") return false;

          CStdString strNewArtist = strArtist;
          if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

          pDialog->SetLine(0, strNewAlbum);
          pDialog->SetLine(1, strNewArtist);
          pDialog->Progress();

          m_musicDatabase.Close();
          return DownloadAlbumInfo(strPath,strNewArtist,strNewAlbum,bCanceled,albumInfo,pDialog);
        }
        iSelectedAlbum = pDlg->GetSelectedItem().m_idepth;
      }
    }

    if (iSelectedAlbum < 0)
    {
      m_musicDatabase.Close();
      return false;
    }
  }

  scraper.LoadAlbumInfo(iSelectedAlbum);
  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      bCanceled = true;
      scraper.Cancel();
    }
    Sleep(1);
  }

  if (scraper.Succeeded())
  {
    albumInfo = scraper.GetAlbum(iSelectedAlbum);
    album = scraper.GetAlbum(iSelectedAlbum).GetAlbum();
    if (result == CNfoFile::COMBINED_NFO)
      nfoReader.GetDetails(album);
    m_musicDatabase.SetAlbumInfo(params.GetAlbumId(), album, scraper.GetAlbum(iSelectedAlbum).GetSongs(),false);
  }
  else
  {
    m_musicDatabase.Close();
    return false;
  }

  // check thumb stuff
  GetAlbumArtwork(params.GetAlbumId(), album);
  m_musicDatabase.Close();
  return true;
}

void CMusicInfoScanner::GetAlbumArtwork(long id, const CAlbum &album)
{
  if (album.thumbURL.m_url.size())
  {
    CStdString thumb;
    if (!m_musicDatabase.GetAlbumThumb(id, thumb) || thumb.IsEmpty() || !XFILE::CFile::Exists(thumb))
    {
      thumb = CUtil::GetCachedAlbumThumb(album.strAlbum,album.strArtist);
      CScraperUrl::DownloadThumbnail(thumb,album.thumbURL.m_url[0]);
      m_musicDatabase.SaveAlbumThumb(id, thumb);
    }
  }
}

bool CMusicInfoScanner::DownloadArtistInfo(const CStdString& strPath, const CStdString& strArtist, bool& bCanceled, CGUIDialogProgress* pDialog)
{
  XFILE::MUSICDATABASEDIRECTORY::CQueryParams params;
  XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strPath, params);
  bCanceled = false;
  CArtist artist;
  m_musicDatabase.Open();
  if (m_musicDatabase.GetArtistInfo(params.GetArtistId(),artist)) // already got the info
    return true;

  // find artist info
  ADDON::ScraperPtr info;
  if (!m_musicDatabase.GetScraperForPath(strPath, info, ADDON::ADDON_SCRAPER_ARTISTS) || !info)
  {
    m_musicDatabase.Close();
    return false;
  }

  // clear our scraper cache
  info->ClearCache();

  if (m_pObserver)
  {
    m_pObserver->OnStateChanged(DOWNLOADING_ARTIST_INFO);
    m_pObserver->OnDirectoryChanged(strArtist);
  }

  CMusicInfoScraper scraper(info);
  // handle nfo files
  CStdString strArtistPath, strNfo;
  m_musicDatabase.GetArtistPath(params.GetArtistId(),strArtistPath);
  URIUtils::AddFileToFolder(strArtistPath,"artist.nfo",strNfo);
  CNfoFile::NFOResult result=CNfoFile::NO_NFO;
  CNfoFile nfoReader;
  if (XFILE::CFile::Exists(strNfo))
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
    result = nfoReader.Create(strNfo, info);
    if (result == CNfoFile::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
      CArtist artist;
      nfoReader.GetDetails(artist);
      m_musicDatabase.SetArtistInfo(params.GetArtistId(), artist);
      GetArtistArtwork(params.GetArtistId(), strArtist, &artist);
      m_musicDatabase.Close();
      return true;
    }
    else if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicArtistInfo artist("nfo",scrUrl);
      info = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",info->Name().c_str());
      CLog::Log(LOGDEBUG,"-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
      scraper.SetScraperInfo(info);
      scraper.GetArtists().push_back(artist);
    }
    else
      CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.GetArtistCount())
    scraper.FindArtistInfo(strArtist);

  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      bCanceled = true;
    }
    Sleep(1);
  }

  int iSelectedArtist = 0;
  if (result == CNfoFile::NO_NFO)
  {
    if (scraper.Succeeded() && scraper.GetArtistCount() >= 1)
    {
      // now load the first match
      if (pDialog && scraper.GetArtistCount() > 1)
      {
        // if we found more then 1 album, let user choose one
        CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
        if (pDlg)
        {
          pDlg->SetHeading(g_localizeStrings.Get(21890));
          pDlg->Reset();
          pDlg->EnableButton(true, 413); // manual

          for (int i = 0; i < scraper.GetArtistCount(); ++i)
          {
            // set the label to artist
            CFileItem item(scraper.GetArtist(i).GetArtist());
            CStdString strTemp=scraper.GetArtist(i).GetArtist().strArtist;
            if (!scraper.GetArtist(i).GetArtist().strBorn.IsEmpty())
              strTemp += " ("+scraper.GetArtist(i).GetArtist().strBorn+")";
            if (!scraper.GetArtist(i).GetArtist().strGenre.IsEmpty())
              strTemp.Format("[%s] %s",scraper.GetArtist(i).GetArtist().strGenre.c_str(),strTemp.c_str());
            item.SetLabel(strTemp);
            item.m_idepth = i; // use this to hold the index of the album in the scraper
            pDlg->Add(&item);
          }
          pDlg->DoModal();

          // and wait till user selects one
          if (pDlg->GetSelectedLabel() < 0)
          { // none chosen
            if (!pDlg->IsButtonPressed())
            {
              bCanceled = true;
              return false;
            }
            // manual button pressed
            CStdString strNewArtist = strArtist;
            if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

            if (pDialog)
            {
              pDialog->SetLine(0, strNewArtist);
              pDialog->Progress();
            }
            m_musicDatabase.Close();
            return DownloadArtistInfo(strPath,strNewArtist,bCanceled,pDialog);
          }
          iSelectedArtist = pDlg->GetSelectedItem().m_idepth;
        }
      }
    }
    else
    {
      m_musicDatabase.Close();
      return false;
    }
  }

  scraper.LoadArtistInfo(iSelectedArtist, strArtist);
  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      bCanceled = true;
    }
    Sleep(1);
  }

  if (scraper.Succeeded())
  {
    artist = scraper.GetArtist(iSelectedArtist).GetArtist();
    if (result == CNfoFile::COMBINED_NFO)
      nfoReader.GetDetails(artist);
    m_musicDatabase.SetArtistInfo(params.GetArtistId(), artist);
  }

  // check thumb stuff
  GetArtistArtwork(params.GetArtistId(), strArtist, &artist);

  m_musicDatabase.Close();
  return true;
}

void CMusicInfoScanner::GetArtistArtwork(long id, const CStdString &artistName, const CArtist *artist)
{
  CStdString artistPath;
  CFileItem item(artistName);
  CStdString thumb = item.GetCachedArtistThumb();
  if (m_musicDatabase.GetArtistPath(id, artistPath) && !XFILE::CFile::Exists(thumb))
  {
    CStdString localThumb = URIUtils::AddFileToFolder(artistPath, "folder.jpg");
    if (XFILE::CFile::Exists(localThumb))
      CPicture::CreateThumbnail(localThumb, thumb);
  }
  if (!XFILE::CFile::Exists(thumb) && artist && artist->thumbURL.m_url.size())
    CScraperUrl::DownloadThumbnail(thumb, artist->thumbURL.m_url[0]);

  // check fanart
  CFileItem item2(artistPath, true);
  item2.GetMusicInfoTag()->SetArtist(artistName);
  CStdString cachedImage = item2.GetCachedFanart();
  if (!CFile::Exists(cachedImage))
  { // check for local fanart
    CLog::Log(LOGDEBUG, "%s looking for fanart for artist %s in folder %s", __FUNCTION__, artistName.c_str(), item2.m_strPath.c_str());
    if (!item2.CacheLocalFanart())
    {
      CLog::Log(LOGDEBUG, "%s no local fanart found for artist %s", __FUNCTION__, artistName.c_str());
      if (artist && !artist->fanart.m_xml.IsEmpty() && !artist->fanart.DownloadImage(item2.GetCachedFanart()))
        CLog::Log(LOGERROR, "Failed to download fanart %s to %s", artist->fanart.GetImageURL().c_str(), item2.GetCachedFanart().c_str());
    }
  }
}
