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

#include "threads/SystemClock.h"
#include "MusicInfoScanner.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "Util.h"
#include "utils/md5.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "NfoFile.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIKeyboardFactory.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "TextureCache.h"
#include "music/MusicThumbLoader.h"
#include "interfaces/AnnouncementManager.h"
#include "GUIUserMessages.h"

#include <algorithm>

using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace MUSIC_GRABBER;

CMusicInfoScanner::CMusicInfoScanner() : CThread("CMusicInfoScanner")
{
  m_bRunning = false;
  m_showDialog = false;
  m_handle = NULL;
  m_bCanInterrupt = false;
  m_currentItem=0;
  m_itemCount=0;
  m_flags = 0;
}

CMusicInfoScanner::~CMusicInfoScanner()
{
}

void CMusicInfoScanner::Process()
{
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanStarted");
  try
  {
    unsigned int tick = XbmcThreads::SystemClockMillis();

    m_musicDatabase.Open();

    if (m_showDialog && !g_guiSettings.GetBool("musiclibrary.backgroundupdate"))
    {
      CGUIDialogExtendedProgressBar* dialog =
        (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
      m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
    }

    m_bCanInterrupt = true;

    if (m_scanType == 0) // load info from files
    {
      CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);

      if (m_handle)
        m_handle->SetTitle(g_localizeStrings.Get(505));

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      SetPriority( GetMinPriority() );
      CThread fileCountReader(this, "CMusicInfoScanner");
      if (m_handle)
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
        g_infoManager.ResetLibraryBools();

        if (m_needsCleanup)
        {
          if (m_handle)
          {
            m_handle->SetTitle(g_localizeStrings.Get(700));
            m_handle->SetText("");
          }

          m_musicDatabase.CleanupOrphanedItems();

          if (m_handle)
            m_handle->SetTitle(g_localizeStrings.Get(331));

          m_musicDatabase.Compress(false);
        }
      }

      fileCountReader.StopThread();

      m_musicDatabase.EmptyCache();

      m_musicDatabase.Close();
      CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);

      tick = XbmcThreads::SystemClockMillis() - tick;
      CLog::Log(LOGNOTICE, "My Music: Scanning for music info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
    }
    bool bCanceled;
    if (m_scanType == 1) // load album info
    {
      if (m_handle)
        m_handle->SetTitle(g_localizeStrings.Get(21885));
      int iCurrentItem = 1;
      for (set<CAlbum>::iterator it=m_albumsToScan.begin();it != m_albumsToScan.end();++it)
      {
        if (m_handle)
        {
          m_handle->SetText(StringUtils::Join(it->artist, g_advancedSettings.m_musicItemSeparator)+" - "+it->strAlbum);
          m_handle->SetPercentage(iCurrentItem++/(float)m_albumsToScan.size());
        }

        CMusicAlbumInfo albumInfo;
        DownloadAlbumInfo(it->genre[0],StringUtils::Join(it->artist, g_advancedSettings.m_musicItemSeparator),it->strAlbum, bCanceled, albumInfo); // genre field holds path - see fetchalbuminfo()

        if (m_bStop || bCanceled)
          break;
      }
    }
    if (m_scanType == 2) // load artist info
    {
      if (m_handle)
        m_handle->SetTitle(g_localizeStrings.Get(21886));

      int iCurrentItem=1;
      for (set<CArtist>::iterator it=m_artistsToScan.begin();it != m_artistsToScan.end();++it)
      {
        if (m_handle)
        {
          m_handle->SetText(it->strArtist);
          m_handle->SetPercentage(iCurrentItem++/(float)m_artistsToScan.size()*100);
        }

        DownloadArtistInfo(it->genre[0],it->strArtist,bCanceled); // genre field holds path - see fetchartistinfo()

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
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanFinished");
  
  // we need to clear the musicdb cache and update any active lists
  CUtil::DeleteMusicDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
  g_windowManager.SendThreadMessage(msg);
  
  if (m_handle)
    m_handle->MarkFinished();
  m_handle = NULL;
}

void CMusicInfoScanner::Start(const CStdString& strDirectory, int flags)
{
  m_pathsToScan.clear();
  m_albumsScanned.clear();
  m_artistsScanned.clear();
  m_flags = flags;

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

void CMusicInfoScanner::FetchAlbumInfo(const CStdString& strDirectory,
                                       bool refresh)
{
  m_albumsToScan.clear();
  m_albumsScanned.clear();

  CFileItemList items;
  if (strDirectory.IsEmpty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetAlbumsNav("musicdb://3/", items);
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

  m_musicDatabase.Open();
  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
      continue;

    CAlbum album;
    album.strAlbum = items[i]->GetMusicInfoTag()->GetAlbum();
    album.artist = items[i]->GetMusicInfoTag()->GetArtist();
    album.genre.push_back(items[i]->GetPath()); // a bit hacky use of field
    m_albumsToScan.insert(album);
    if (refresh)
    {
      int id = m_musicDatabase.GetAlbumByName(album.strAlbum, album.artist);
      if (id > -1)
        m_musicDatabase.DeleteAlbumInfo(id);
    }
  }
  m_musicDatabase.Close();

  m_scanType = 1;
  StopThread();
  Create();
  m_bRunning = true;
}

void CMusicInfoScanner::FetchArtistInfo(const CStdString& strDirectory,
                                        bool refresh)
{
  m_artistsToScan.clear();
  m_artistsScanned.clear();
  CFileItemList items;

  if (strDirectory.IsEmpty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetArtistsNav("musicdb://2/", items, false, -1);
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

  m_musicDatabase.Open();
  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
      continue;

    CArtist artist;
    artist.strArtist = StringUtils::Join(items[i]->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    artist.genre.push_back(items[i]->GetPath()); // a bit hacky use of field
    m_artistsToScan.insert(artist);
    if (refresh)
    {
      int id = m_musicDatabase.GetArtistByName(artist.strArtist);
      if (id > -1)
        m_musicDatabase.DeleteArtistInfo(id);
    }
  }
  m_musicDatabase.Close();

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

static void OnDirectoryScanned(const CStdString& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  g_windowManager.SendThreadMessage(msg);
}

static CStdString Prettify(const CStdString& strDirectory)
{
  CURL url(strDirectory);
  CStdString strStrippedPath = url.GetWithoutUserDetails();
  CURL::Decode(strStrippedPath);

  return strStrippedPath;
}

bool CMusicInfoScanner::DoScan(const CStdString& strDirectory)
{
  if (m_handle)
    m_handle->SetText(Prettify(strDirectory));

  /*
   * remove this path from the list we're processing. This must be done prior to
   * the check for file or folder exclusion to prevent an infinite while loop
   * in Process().
   */
  set<CStdString>::iterator it = m_pathsToScan.find(strDirectory);
  if (it != m_pathsToScan.end())
    m_pathsToScan.erase(it);

  // Discard all excluded files defined by m_musicExcludeRegExps

  CStdStringArray regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;

  if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
    return true;

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_settings.m_musicExtensions + "|.jpg|.tbn|.lrc|.cdg");

  // sort and get the path hash.  Note that we don't filter .cue sheet items here as we want
  // to detect changes in the .cue sheet as well.  The .cue sheet items only need filtering
  // if we have a changed hash.
  items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
  CStdString hash;
  GetPathHash(items, hash);

  // check whether we need to rescan or not
  CStdString dbHash;
  if ((m_flags & SCAN_RESCAN) || !m_musicDatabase.GetPathHash(strDirectory, dbHash) || dbHash != hash)
  { // path has changed - rescan
    if (dbHash.IsEmpty())
      CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, strDirectory.c_str());
    else
      CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, strDirectory.c_str());

    // filter items in the sub dir (for .cue sheet support)
    items.FilterCueItems();
    items.Sort(SORT_METHOD_LABEL, SortOrderAscending);

    // and then scan in the new information
    if (RetrieveMusicInfo(items, strDirectory) > 0)
    {
      if (m_handle)
        OnDirectoryScanned(strDirectory);
    }

    // save information about this folder
    m_musicDatabase.SetPathHash(strDirectory, hash);
  }
  else
  { // path is the same - no need to rescan
    CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, strDirectory.c_str());
    m_currentItem += CountFiles(items, false);  // false for non-recursive

    // updated the dialog with our progress
    if (m_handle)
    {
      if (m_itemCount>0)
        m_handle->SetPercentage(m_currentItem/(float)m_itemCount*100);
      OnDirectoryScanned(strDirectory);
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
      CStdString strPath=pItem->GetPath();
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

  CStdStringArray regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;

  // for every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    CStdString strExtension;
    URIUtils::GetExtension(pItem->GetPath(), strExtension);

    if (m_bStop)
      return 0;

    // Discard all excluded files defined by m_musicExcludeRegExps
    if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), regexps))
      continue;

    // dont try reading id3tags for folders, playlists or shoutcast streams
    if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsPicture() && !pItem->IsLyrics() )
    {
      m_currentItem++;
//      CLog::Log(LOGDEBUG, "%s - Reading tag for: %s", __FUNCTION__, pItem->GetPath().c_str());

      // grab info from the song
      CSong *dbSong = songsMap.Find(pItem->GetPath());

      CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
      if (!tag.Loaded() )
      { // read the tag from a file
        auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->GetPath()));
        if (NULL != pLoader.get())
          pLoader->Load(pItem->GetPath(), tag);
      }

      // if we have the itemcount, update our
      // dialog with the progress we made
      if (m_handle && m_itemCount>0)
        m_handle->SetPercentage(m_currentItem/(float)m_itemCount*100);

      if (tag.Loaded())
      {
        CSong song(tag);

        // ensure our song has a valid filename or else it will assert in AddSong()
        if (song.strFileName.IsEmpty())
        {
          // copy filename from path in case UPnP or other tag loaders didn't specify one (FIXME?)
          song.strFileName = pItem->GetPath();

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
        song.strThumb = pItem->GetUserMusicThumb(true);
        if (dbSong)
        { // keep the db-only fields intact on rescan...
          song.iTimesPlayed = dbSong->iTimesPlayed;
          song.lastPlayed = dbSong->lastPlayed;
          song.iKaraokeNumber = dbSong->iKaraokeNumber;

          if (song.rating == '0') song.rating = dbSong->rating;
          if (song.strThumb.empty())
            song.strThumb = dbSong->strThumb;
        }
        songsToAdd.push_back(song);
//        CLog::Log(LOGDEBUG, "%s - Tag loaded for: %s", __FUNCTION__, pItem->GetPath().c_str());
      }
      else
        CLog::Log(LOGDEBUG, "%s - No tag found for: %s", __FUNCTION__, pItem->GetPath().c_str());
    }
  }

  VECALBUMS albums;
  CategoriseAlbums(songsToAdd, albums);
  FindArtForAlbums(albums, items.GetPath());

  // finally, add these to the database
  m_musicDatabase.BeginTransaction();
  int numAdded = 0;
  set<long> albumsToScan;
  set<long> artistsToScan;
  for (VECALBUMS::iterator i = albums.begin(); i != albums.end(); ++i)
  {
    vector<int> songIDs;
    int idAlbum = m_musicDatabase.AddAlbum(*i, songIDs);
    numAdded += i->songs.size();
    if (m_bStop)
    {
      m_musicDatabase.RollbackTransaction();
      return numAdded;
    }

    // Build the artist & album sets
    albumsToScan.insert(idAlbum);
    for (vector<int>::iterator j = songIDs.begin(); j != songIDs.end(); ++j)
    {
      vector<long> songArtists;
      m_musicDatabase.GetArtistsBySong(*j, false, songArtists);
      artistsToScan.insert(songArtists.begin(), songArtists.end());
    }
    std::vector<long> albumArtists;
    m_musicDatabase.GetArtistsByAlbum(idAlbum, false, albumArtists);
    artistsToScan.insert(albumArtists.begin(), albumArtists.end());
  }
  m_musicDatabase.CommitTransaction();

  // Download info & artwork
  bool bCanceled;
  for (set<long>::iterator it = artistsToScan.begin(); it != artistsToScan.end(); ++it)
  {
    bCanceled = false;
    if (find(m_artistsScanned.begin(),m_artistsScanned.end(), *it) == m_artistsScanned.end())
    {
      CStdString strArtist = m_musicDatabase.GetArtistById(*it);
      m_artistsScanned.push_back(*it);
      if (!m_bStop && (m_flags & SCAN_ONLINE))
      {
        CStdString strPath;
        strPath.Format("musicdb://2/%u/", *it);

        if (!DownloadArtistInfo(strPath, strArtist, bCanceled)) // assume we want to retry
          m_artistsScanned.pop_back();
      }
      else
      {
        map<string, string> artwork = GetArtistArtwork(*it);
        m_musicDatabase.SetArtForItem(*it, "artist", artwork);
      }
    }
  }

  if (m_flags & SCAN_ONLINE)
  {
    for (set<long>::iterator it = albumsToScan.begin(); it != albumsToScan.end(); ++it)
    {
      if (m_bStop)
        return songsToAdd.size();

      CStdString strPath;
      strPath.Format("musicdb://3/%u/",*it);

      CAlbum album;
      m_musicDatabase.GetAlbumInfo(*it, album, NULL);
      bCanceled = false;
      if (find(m_albumsScanned.begin(), m_albumsScanned.end(), *it) == m_albumsScanned.end())
      {
        CMusicAlbumInfo albumInfo;
        if (DownloadAlbumInfo(strPath, StringUtils::Join(album.artist, g_advancedSettings.m_musicItemSeparator), album.strAlbum, bCanceled, albumInfo))
          m_albumsScanned.push_back(*it);
      }
    }
  }
  if (m_handle)
    m_handle->SetTitle(g_localizeStrings.Get(505));

  return songsToAdd.size();
}

static bool SortSongsByTrack(CSong *song, CSong *song2)
{
  return song->iTrack < song2->iTrack;
}

void CMusicInfoScanner::CategoriseAlbums(VECSONGS &songsToCheck, VECALBUMS &albums)
{
  /* Step 1: categorise on the album name */
  map<string, vector<CSong *> > albumNames;
  for (VECSONGS::iterator i = songsToCheck.begin(); i != songsToCheck.end(); ++i)
    albumNames[i->strAlbum].push_back(&(*i));

  /*
   Step 2: Split into unique albums based on album name and album artist
   In the case where the album artist is unknown, we use the primary artist
   (i.e. first artist from each song).
   */
  albums.clear();
  for (map<string, vector<CSong *> >::iterator i = albumNames.begin(); i != albumNames.end(); ++i)
  {
    // sort the songs by tracknumber to identify duplicate track numbers
    vector<CSong *> &songs = i->second;
    sort(songs.begin(), songs.end(), SortSongsByTrack);

    // map the songs to their primary artists
    bool compilation = !i->first.empty();
    map<string, vector<CSong *> > artists;
    for (vector<CSong *>::iterator j = songs.begin(); j != songs.end(); ++j)
    {
      CSong *song = *j;
      // test for song overlap
      if (j != songs.begin() && song->iTrack == (*(j-1))->iTrack)
        compilation = false;

      // get primary artist
      string primary;
      if (!song->albumArtist.empty())
      {
        primary = song->albumArtist[0];
        compilation = false;
      }
      else if (!song->artist.empty())
        primary = song->artist[0];

      // add to the artist map
      artists[primary].push_back(song);
    }

    /*
     We have a compilation if
     1. album name is non-empty
     2. no tracks overlap
     3. no album artist is specified
     4. we have at least two different primary artists
     */
    if (artists.size() == 1)
      compilation = false;
    if (compilation)
    {
      artists.clear();
      std::string various = g_localizeStrings.Get(340); // Various Artists
      for (vector<CSong *>::iterator j = songs.begin(); j != songs.end(); ++j)
        (*j)->albumArtist.push_back(various);
      artists.insert(make_pair(various, songs));
    }

    /*
     Step 3: Find the common albumartist for each song and assign
     albumartist to those tracks that don't have it set.
     */
    for (map<string, vector<CSong *> >::iterator j = artists.begin(); j != artists.end(); ++j)
    {
      // find the common artist for these songs
      vector<CSong *> &artistSongs = j->second;
      vector<string> common = artistSongs.front()->albumArtist.empty() ? artistSongs.front()->artist : artistSongs.front()->albumArtist;
      for (vector<CSong *>::iterator k = artistSongs.begin() + 1; k != artistSongs.end(); ++k)
      {
        unsigned int match = 0;
        vector<string> &compare = (*k)->albumArtist.empty() ? (*k)->artist : (*k)->albumArtist;
        for (; match < common.size() && match < compare.size(); match++)
        {
          if (compare[match] != common[match])
            break;
        }
        common.erase(common.begin() + match, common.end());
      }

      /*
       Step 4: Assign the album artist for each song that doesn't have it set
       and add to the album vector
       */
      CAlbum album;
      album.strAlbum = i->first;
      album.artist = common;
      album.bCompilation = compilation;
      for (vector<CSong *>::iterator k = artistSongs.begin(); k != artistSongs.end(); ++k)
      {
        if ((*k)->albumArtist.empty())
          (*k)->albumArtist = common;
        album.songs.push_back(*(*k));
        // TODO: in future we may wish to union up the genres, for now we assume they're the same
        if (album.genre.empty())
          album.genre = (*k)->genre;
        //       in addition, we may want to use year as discriminating for albums
        if (album.iYear == 0)
          album.iYear = (*k)->iYear;
      }

      albums.push_back(album);
    }
  }
}

void CMusicInfoScanner::FindArtForAlbums(VECALBUMS &albums, const CStdString &path)
{
  /*
   If there's a single album in the folder, then art can be taken from
   the folder art.
   */
  std::string albumArt;
  if (albums.size() == 1)
  {
    CFileItem album(path, true);
    albumArt = album.GetUserMusicThumb(true);
    albums[0].art["thumb"] = albumArt;
  }
  for (VECALBUMS::iterator i = albums.begin(); i != albums.end(); ++i)
  {
    CAlbum &album = *i;

    if (albums.size() != 1)
      albumArt = "";

    /*
     Find art that is common across these items
     If we find a single art image we treat it as the album art
     and discard song art else we use first as album art and
     keep everything as song art.
     */
    bool singleArt = true;
    CSong *art = NULL;
    for (VECSONGS::iterator k = album.songs.begin(); k != album.songs.end(); ++k)
    {
      CSong &song = *k;
      if (song.HasArt())
      {
        if (art && !art->ArtMatches(song))
        {
          singleArt = false;
          break;
        }
        if (!art)
          art = &song;
      }
    }

    /*
      assign the first art found to the album - better than no art at all
    */

    if (art && albumArt.empty())
    {
      if (!art->strThumb.empty())
        albumArt = art->strThumb;
      else
        albumArt = CTextureCache::GetWrappedImageURL(art->strFileName, "music");
    }

    album.art["thumb"] = albumArt;

    if (singleArt)
    { //if singleArt then we can clear the artwork for all songs
      for (VECSONGS::iterator k = album.songs.begin(); k != album.songs.end(); ++k)
        k->strThumb.clear();
    }
    else
    { // more than one piece of art was found for these songs, so cache per song
      for (VECSONGS::iterator k = album.songs.begin(); k != album.songs.end(); ++k)
      {
        if (k->strThumb.empty() && !k->embeddedArt.empty())
          k->strThumb = CTextureCache::GetWrappedImageURL(k->strFileName, "music");
      }
    }
  }
  if (albums.size() == 1 && !albumArt.empty())
  { // assign to folder thumb as well
    CMusicThumbLoader::SetCachedImage(path, "thumb", albumArt);
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
  CDirectory::GetDirectory(strPath, items, g_settings.m_musicExtensions, DIR_FLAG_NO_FILE_DIRS);

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
      count+=CountFilesRecursively(pItem->GetPath());
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
    md5state.append(pItem->GetPath());
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

  if (m_handle)
  {
    m_handle->SetTitle(g_localizeStrings.Get(21885));
    m_handle->SetText(strArtist+" - "+strAlbum);
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
  {
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
            relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, StringUtils::Join(info.GetAlbum().artist, g_advancedSettings.m_musicItemSeparator), strArtist);

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
          relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, StringUtils::Join(info.GetAlbum().artist, g_advancedSettings.m_musicItemSeparator), strArtist);
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
          if (!CGUIKeyboardFactory::ShowAndGetInput(strNewAlbum, g_localizeStrings.Get(16011), false)) return false;
          if (strNewAlbum == "") return false;

          CStdString strNewArtist = strArtist;
          if (!CGUIKeyboardFactory::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

          pDialog->SetLine(0, strNewAlbum);
          pDialog->SetLine(1, strNewArtist);
          pDialog->Progress();

          m_musicDatabase.Close();
          return DownloadAlbumInfo(strPath,strNewArtist,strNewAlbum,bCanceled,albumInfo,pDialog);
        }
        iSelectedAlbum = pDlg->GetSelectedItem()->m_idepth;
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
      nfoReader.GetDetails(album,NULL,true);
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
    if (m_musicDatabase.GetArtForItem(id, "album", "thumb").empty())
    {
      string thumb = CScraperUrl::GetThumbURL(album.thumbURL.GetFirstThumb());
      if (!thumb.empty())
      {
        CTextureCache::Get().BackgroundCacheImage(thumb);
        m_musicDatabase.SetArtForItem(id, "album", "thumb", thumb);
      }
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

  if (m_handle)
  {
    m_handle->SetTitle(g_localizeStrings.Get(21886));
    m_handle->SetText(strArtist);
  }

  // clear our scraper cache
  info->ClearCache();

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
      map<string, string> artwork = GetArtistArtwork(params.GetArtistId(), &artist);
      m_musicDatabase.SetArtForItem(params.GetArtistId(), "artist", artwork);
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
  {
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
            if (!scraper.GetArtist(i).GetArtist().genre.empty())
            {
              CStdString genres = StringUtils::Join(scraper.GetArtist(i).GetArtist().genre, g_advancedSettings.m_musicItemSeparator);
              if (!genres.empty())
                strTemp.Format("[%s] %s", genres.c_str(), strTemp.c_str());
            }
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
            if (!CGUIKeyboardFactory::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

            if (pDialog)
            {
              pDialog->SetLine(0, strNewArtist);
              pDialog->Progress();
            }
            m_musicDatabase.Close();
            return DownloadArtistInfo(strPath,strNewArtist,bCanceled,pDialog);
          }
          iSelectedArtist = pDlg->GetSelectedItem()->m_idepth;
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
      nfoReader.GetDetails(artist,NULL,true);
    m_musicDatabase.SetArtistInfo(params.GetArtistId(), artist);
  }

  // check thumb stuff
  map<string, string> artwork = GetArtistArtwork(params.GetArtistId(), &artist);
  m_musicDatabase.SetArtForItem(params.GetArtistId(), "artist", artwork);

  m_musicDatabase.Close();
  return true;
}

map<string, string> CMusicInfoScanner::GetArtistArtwork(long id, const CArtist *artist)
{
  CStdString artistPath;
  m_musicDatabase.Open();
  bool checkLocal = m_musicDatabase.GetArtistPath(id, artistPath);
  m_musicDatabase.Close();

  CFileItem item(artistPath, true);
  map<string, string> artwork;

  // check thumb
  CStdString thumb;
  if (checkLocal)
    thumb = item.GetUserMusicThumb(true);
  if (thumb.IsEmpty() && artist)
    thumb = CScraperUrl::GetThumbURL(artist->thumbURL.GetFirstThumb());
  if (!thumb.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    artwork.insert(make_pair("thumb", thumb));
  }

  // check fanart
  CStdString fanart;
  if (checkLocal)
    fanart = item.GetLocalFanart();
  if (fanart.IsEmpty() && artist)
    fanart = artist->fanart.GetImageURL();
  if (!fanart.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(fanart);
    artwork.insert(make_pair("fanart", fanart));
  }

  return artwork;
}
