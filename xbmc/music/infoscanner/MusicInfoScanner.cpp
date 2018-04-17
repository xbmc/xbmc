/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MusicInfoScanner.h"

#include <algorithm>
#include <utility>

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Scraper.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "events/EventLog.h"
#include "events/MediaLibraryEvent.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/SmartPlaylistDirectory.h" 
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "interfaces/AnnouncementManager.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "NfoFile.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "threads/SystemClock.h"
#include "Util.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace MUSIC_GRABBER;
using namespace ADDON;
using KODI::UTILITY::CDigest;

CMusicInfoScanner::CMusicInfoScanner()
: m_needsCleanup(false),
  m_scanType(0),
  m_fileCountReader(this, "MusicFileCounter")
{
  m_bStop = false;
  m_currentItem=0;
  m_itemCount=0;
  m_flags = 0;
}

CMusicInfoScanner::~CMusicInfoScanner() = default;

void CMusicInfoScanner::Process()
{
  m_bStop = false;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanStarted");
  try
  {
    if (m_showDialog && !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE))
    {
      CGUIDialogExtendedProgressBar* dialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
      if (dialog)
        m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
    }

    // check if we only need to perform a cleaning
    if (m_bClean && m_pathsToScan.empty())
    {
      CMusicLibraryQueue::GetInstance().CleanLibrary(false);
      m_handle = NULL;
      m_bRunning = false;

      return;
    }
    
    unsigned int tick = XbmcThreads::SystemClockMillis();
    m_musicDatabase.Open();
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
      if (m_handle)
        m_fileCountReader.Create();

      // Database operations should not be canceled
      // using Interrupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;
      m_needsCleanup = false;

      bool commit = true;
      for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); ++it)
      {
        if (!CDirectory::Exists(*it) && !m_bClean)
        {
          /*
           * Note that this will skip scanning (if m_bClean is disabled) if the directory really
           * doesn't exist. Since the music scanner is fed with a list of existing paths from the DB
           * and cleans out all songs under that path as its first step before re-adding files, if 
           * the entire source is offline we totally empty the music database in one go.
           */
          CLog::Log(LOGWARNING, "%s directory '%s' does not exist - skipping scan.", __FUNCTION__, it->c_str());
          m_seenPaths.insert(*it);
          continue;
        }

        bool scancomplete = DoScan(*it);
        if (scancomplete)
        { 
          if (m_albumsAdded.size() > 0)
          {
            if (m_flags & SCAN_ONLINE)
              // Download additional album and artist information for the recently added albums.
              // This also identifies any local artist thumb and fanart if it exitsts, and gives it priority, 
              // otherwise it is set to the first available from the remote thumbs and fanart that was scraped.
              ScrapeInfoAddedAlbums();
          }
        }
        else 
        {
          commit = false;
          break;
        }
      }

      if (commit)
      {
        CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();

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

      m_fileCountReader.StopThread();

      m_musicDatabase.EmptyCache();
      
      tick = XbmcThreads::SystemClockMillis() - tick;
      CLog::Log(LOGNOTICE, "My Music: Scanning for music info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
    }
    if (m_scanType == 1) // load album info
    {
      for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); ++it)
      {
        CQueryParams params;
        CDirectoryNode::GetDatabaseInfo(*it, params);
        // Only scrape information for albums that have not been scraped before
        // For refresh of information the lastscraped date is optionally clearered elsewhere
        if (m_musicDatabase.HasAlbumBeenScraped(params.GetAlbumId()))
          continue;

        CAlbum album;
        m_musicDatabase.GetAlbum(params.GetAlbumId(), album);
        if (m_handle)
        {
          float percentage = static_cast<float>(std::distance(m_pathsToScan.begin(), it) * 100) / static_cast<float>(m_pathsToScan.size());
          m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
          m_handle->SetPercentage(percentage);
        }

        // find album info
        ADDON::ScraperPtr scraper;
        if (!m_musicDatabase.GetScraper(album.idAlbum, CONTENT_ALBUMS, scraper))
          continue;

        UpdateDatabaseAlbumInfo(album, scraper, false);

        if (m_bStop)
          break;
      }
    }
    if (m_scanType == 2) // load artist info
    {
      for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); ++it)
      {
        CQueryParams params;
        CDirectoryNode::GetDatabaseInfo(*it, params);
        // Only scrape information for artists that have not been scraped before
        // For refresh of information the lastscraped date is optionally clearered elsewhere
        if (m_musicDatabase.HasArtistBeenScraped(params.GetArtistId())) 
            continue;

        CArtist artist;
        m_musicDatabase.GetArtist(params.GetArtistId(), artist);
        m_musicDatabase.GetArtistPath(artist, artist.strPath);

        if (m_handle)
        {
          float percentage = static_cast<float>(std::distance(m_pathsToScan.begin(), it) * 100) / static_cast<float>(m_pathsToScan.size());
          m_handle->SetText(artist.strArtist);
          m_handle->SetPercentage(percentage);
        }

        // find album info
        ADDON::ScraperPtr scraper;
        if (!m_musicDatabase.GetScraper(artist.idArtist, CONTENT_ARTISTS, scraper) || !scraper)
          continue;

        UpdateDatabaseArtistInfo(artist, scraper, false);

        if (m_bStop)
          break;
      }
    }
    //propagate artist sort names to albums and songs
    if (g_advancedSettings.m_bMusicLibraryArtistSortOnUpdate)
      m_musicDatabase.UpdateArtistSortNames();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception while scanning.");
  }
  m_musicDatabase.Close();
  CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);
  
  m_bRunning = false;
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanFinished");
  
  // we need to clear the musicdb cache and update any active lists
  CUtil::DeleteMusicDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  
  if (m_handle)
    m_handle->MarkFinished();
  m_handle = NULL;
}

void CMusicInfoScanner::Start(const std::string& strDirectory, int flags)
{
  m_fileCountReader.StopThread();
  
  m_pathsToScan.clear();
  m_seenPaths.clear();
  m_albumsAdded.clear();
  m_artistsArt.clear();
  m_flags = flags;

  if (strDirectory.empty())
  { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
    // we go.
    m_musicDatabase.Open();
    m_musicDatabase.GetPaths(m_pathsToScan);
    m_musicDatabase.Close();
  }
  else
    m_pathsToScan.insert(strDirectory);
  
  m_bClean = g_advancedSettings.m_bMusicLibraryCleanOnUpdate;

  m_scanType = 0;
  m_bRunning = true;
  Process();
}

void CMusicInfoScanner::FetchAlbumInfo(const std::string& strDirectory,
                                       bool refresh)
{
  m_fileCountReader.StopThread();
  m_pathsToScan.clear();

  CFileItemList items;
  if (strDirectory.empty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetAlbumsNav("musicdb://albums/", items);
    m_musicDatabase.Close();
  }
  else
  {
    CURL pathToUrl(strDirectory);

    if (pathToUrl.IsProtocol("musicdb"))
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strDirectory, params);
      if (params.GetAlbumId() != -1)
      {
        //Add single album as item to scan
        CFileItemPtr item(new CFileItem(strDirectory, false));
        items.Add(item);
      }
      else
      {
        CMusicDatabaseDirectory dir;
        NODE_TYPE childtype = dir.GetDirectoryChildType(strDirectory);
        if (childtype == NODE_TYPE_ALBUM)
          dir.GetDirectory(pathToUrl, items);
      }
    }
    else if (StringUtils::EndsWith(strDirectory, ".xsp"))
    {
      CSmartPlaylistDirectory dir;
      dir.GetDirectory(pathToUrl, items);
    }
  }

  m_musicDatabase.Open();
  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
      continue;

    m_pathsToScan.insert(items[i]->GetPath());
    if (refresh)
    {
      m_musicDatabase.ClearAlbumLastScrapedTime(items[i]->GetMusicInfoTag()->GetDatabaseId());
    }
  }
  m_musicDatabase.Close();

  m_scanType = 1;
  m_bRunning = true;
  Process();
}

void CMusicInfoScanner::FetchArtistInfo(const std::string& strDirectory,
                                        bool refresh)
{
  m_fileCountReader.StopThread();
  m_pathsToScan.clear();
  CFileItemList items;

  if (strDirectory.empty())
  {
    m_musicDatabase.Open();
    m_musicDatabase.GetArtistsNav("musicdb://artists/", items, !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS), -1);
    m_musicDatabase.Close();
  }
  else
  {
    CURL pathToUrl(strDirectory);

    if (pathToUrl.IsProtocol("musicdb"))
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strDirectory, params);
      if (params.GetArtistId() != -1)
      {
        //Add single artist as item to scan
        CFileItemPtr item(new CFileItem(strDirectory, false));
        items.Add(item);
      }
      else
      {
        CMusicDatabaseDirectory dir;
        NODE_TYPE childtype = dir.GetDirectoryChildType(strDirectory);
        if (childtype == NODE_TYPE_ARTIST)
          dir.GetDirectory(pathToUrl, items);
      }
    }
    else if (StringUtils::EndsWith(strDirectory, ".xsp"))
    {
      CSmartPlaylistDirectory dir;
      dir.GetDirectory(pathToUrl, items);
    }
  }

  m_musicDatabase.Open();
  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
      continue;

    m_pathsToScan.insert(items[i]->GetPath());
    if (refresh)
    {
      m_musicDatabase.ClearArtistLastScrapedTime(items[i]->GetMusicInfoTag()->GetDatabaseId());
    }
  }
  m_musicDatabase.Close();

  m_scanType = 2;
  m_bRunning = true;
  Process();
}

void CMusicInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_musicDatabase.Interrupt();

  m_bStop = true;
}

static void OnDirectoryScanned(const std::string& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

static std::string Prettify(const std::string& strDirectory)
{
  CURL url(strDirectory);

  return CURL::Decode(url.GetWithoutUserDetails());
}

bool CMusicInfoScanner::DoScan(const std::string& strDirectory)
{
  if (m_handle)
  {
    m_handle->SetTitle(g_localizeStrings.Get(506)); //"Checking media files..."
    m_handle->SetText(Prettify(strDirectory));
  }

  std::set<std::string>::const_iterator it = m_seenPaths.find(strDirectory);
  if (it != m_seenPaths.end())
    return true;

  m_seenPaths.insert(strDirectory);

  // Discard all excluded files defined by m_musicExcludeRegExps
  const std::vector<std::string> &regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;

  if (IsExcluded(strDirectory, regexps))
    return true;

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, CServiceBroker::GetFileExtensionProvider().GetMusicExtensions() + "|.jpg|.tbn|.lrc|.cdg");

  // sort and get the path hash.  Note that we don't filter .cue sheet items here as we want
  // to detect changes in the .cue sheet as well.  The .cue sheet items only need filtering
  // if we have a changed hash.
  items.Sort(SortByLabel, SortOrderAscending);
  std::string hash;
  GetPathHash(items, hash);

  // check whether we need to rescan or not
  std::string dbHash;
  if ((m_flags & SCAN_RESCAN) || !m_musicDatabase.GetPathHash(strDirectory, dbHash) || !StringUtils::EqualsNoCase(dbHash, hash))
  { // path has changed - rescan
    if (dbHash.empty())
      CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, CURL::GetRedacted(strDirectory).c_str());
    else
      CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, CURL::GetRedacted(strDirectory).c_str());

    if (m_handle)
      m_handle->SetTitle(g_localizeStrings.Get(505)); //"Loading media information from files..."

    // filter items in the sub dir (for .cue sheet support)
    items.FilterCueItems();
    items.Sort(SortByLabel, SortOrderAscending);

    // and then scan in the new information from tags
    if (RetrieveMusicInfo(strDirectory, items) > 0)
    {
      if (m_handle)
        OnDirectoryScanned(strDirectory);
    }

    // save information about this folder
    m_musicDatabase.SetPathHash(strDirectory, hash);
  }
  else
  { // path is the same - no need to rescan
    CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, CURL::GetRedacted(strDirectory).c_str());
    m_currentItem += CountFiles(items, false);  // false for non-recursive

    // updated the dialog with our progress
    if (m_handle)
    {
      if (m_itemCount>0)
        m_handle->SetPercentage(static_cast<float>(m_currentItem * 100) / static_cast<float>(m_itemCount));
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
      std::string strPath=pItem->GetPath();
      if (!DoScan(strPath))
      {
        m_bStop = true;
      }
    }
  }
  return !m_bStop;
}

CInfoScanner::INFO_RET CMusicInfoScanner::ScanTags(const CFileItemList& items,
                                                   CFileItemList& scannedItems)
{
  std::vector<std::string> regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (m_bStop)
      return INFO_CANCELLED;

    CFileItemPtr pItem = items[i];

    if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), regexps))
      continue;

    if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsPicture() || pItem->IsLyrics())
      continue;

    m_currentItem++;

    CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
    if (!tag.Loaded())
    {
      std::unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(*pItem));
      if (NULL != pLoader.get())
        pLoader->Load(pItem->GetPath(), tag);
    }

    if (m_handle && m_itemCount>0)
      m_handle->SetPercentage(static_cast<float>(m_currentItem * 100) / static_cast<float>(m_itemCount));

    if (!tag.Loaded() && !pItem->HasCueDocument())
    {
      CLog::Log(LOGDEBUG, "%s - No tag found for: %s", __FUNCTION__, pItem->GetPath().c_str());
      continue;
    }
    else
    {
      if (!tag.GetCueSheet().empty())
        pItem->LoadEmbeddedCue();
    }

    if (pItem->HasCueDocument())
      pItem->LoadTracksFromCueDocument(scannedItems);
    else
      scannedItems.Add(pItem);
  }
  return INFO_ADDED;
}

static bool SortSongsByTrack(const CSong& song, const CSong& song2)
{
  return song.iTrack < song2.iTrack;
}

void CMusicInfoScanner::FileItemsToAlbums(CFileItemList& items, VECALBUMS& albums, MAPSONGS* songsMap /* = NULL */)
{
  /*
   * Step 1: Convert the FileItems into Songs. 
   * If they're MB tagged, create albums directly from the FileItems.
   * If they're non-MB tagged, index them by album name ready for step 2.
   */
  std::map<std::string, VECSONGS> songsByAlbumNames;
  for (int i = 0; i < items.Size(); ++i)
  {
    CMusicInfoTag& tag = *items[i]->GetMusicInfoTag();
    CSong song(*items[i]);

    // keep the db-only fields intact on rescan...
    if (songsMap != NULL)
    {
      MAPSONGS::iterator it = songsMap->find(items[i]->GetPath());
      if (it != songsMap->end())
      {
        song.iTimesPlayed = it->second.iTimesPlayed;
        song.lastPlayed = it->second.lastPlayed;
        if (song.rating == 0)    song.rating = it->second.rating;
        if (song.userrating == 0)    song.userrating = it->second.userrating;
        if (song.strThumb.empty()) song.strThumb = it->second.strThumb;
      }
    }

    if (!tag.GetMusicBrainzAlbumID().empty())
    {
      VECALBUMS::iterator it;
      for (it = albums.begin(); it != albums.end(); ++it)
        if (it->strMusicBrainzAlbumID == tag.GetMusicBrainzAlbumID())
          break;

      if (it == albums.end())
      {
        CAlbum album(*items[i]);
        album.songs.push_back(song);
        albums.push_back(album);
      }
      else
        it->songs.push_back(song);
    }
    else
      songsByAlbumNames[tag.GetAlbum()].push_back(song);
  }

  /*
   Step 2: Split into unique albums based on album name and album artist
   In the case where the album artist is unknown, we use the primary artist
   (i.e. first artist from each song).
   */
  for (std::map<std::string, VECSONGS>::iterator songsByAlbumName = songsByAlbumNames.begin(); songsByAlbumName != songsByAlbumNames.end(); ++songsByAlbumName)
  {
    VECSONGS &songs = songsByAlbumName->second;
    // sort the songs by tracknumber to identify duplicate track numbers
    sort(songs.begin(), songs.end(), SortSongsByTrack);

    // map the songs to their primary artists
    bool tracksOverlap = false;
    bool hasAlbumArtist = false;
    bool isCompilation = true;

    std::map<std::string, std::vector<CSong *> > artists;
    for (VECSONGS::iterator song = songs.begin(); song != songs.end(); ++song)
    {
      // test for song overlap
      if (song != songs.begin() && song->iTrack == (song - 1)->iTrack)
        tracksOverlap = true;

      if (!song->bCompilation)
        isCompilation = false;

      // get primary artist
      std::string primary;
      if (!song->GetAlbumArtist().empty())
      {
        primary = song->GetAlbumArtist()[0];
        hasAlbumArtist = true;
      }
      else if (!song->artistCredits.empty())
        primary = song->artistCredits.begin()->GetArtist();

      // add to the artist map
      artists[primary].push_back(&(*song));
    }

    /*
    We have a Various Artists compilation if
    1. album name is non-empty AND
    2a. no tracks overlap OR
    2b. all tracks are marked as part of compilation AND
    3a. a unique primary artist is specified as "various", "various artists" or the localized value
    OR
    3b. we have at least two primary artists and no album artist specified.
    */
    std::string various = g_localizeStrings.Get(340); // Various Artists
    bool compilation = !songsByAlbumName->first.empty() && (isCompilation || !tracksOverlap); // 1+2b+2a
    if (artists.size() == 1)
    {
      std::string artist = artists.begin()->first; StringUtils::ToLower(artist);
      if (!StringUtils::EqualsNoCase(artist, "various") &&
        !StringUtils::EqualsNoCase(artist, "various artists") &&
        !StringUtils::EqualsNoCase(artist, various)) // 3a
        compilation = false;
    }
    else if (hasAlbumArtist) // 3b
      compilation = false;

    //Such a compilation album is stored with the localized value for "various artists" as the album artist
    if (compilation)
    {
      CLog::Log(LOGDEBUG, "Album '%s' is a compilation as there's no overlapping tracks and %s", songsByAlbumName->first.c_str(), hasAlbumArtist ? "the album artist is 'Various'" : "there is more than one unique artist");
      artists.clear();
      std::vector<std::string> va; va.push_back(various);
      for (VECSONGS::iterator song = songs.begin(); song != songs.end(); ++song)
      {
        song->SetAlbumArtist(va);
        artists[various].push_back(&(*song));
      }
    }

    /*
    We also have a compilation album when album name is non-empty and ALL tracks are marked as part of
    a compilation even if an album artist is given, or all songs have the same primary artist. For
    example an anthology - a collection of recordings from various old sources
    combined together such as a "best of", retrospective or rarities type release.

    Such an anthology compilation will not have been caught by the previous tests as it fails 3a and 3b.
    The album artist can be determined just like any normal album.
    */
    if (!compilation && !songsByAlbumName->first.empty() && isCompilation)
    {
      compilation = true;
      CLog::Log(LOGDEBUG, "Album '%s' is a compilation as all songs are marked as part of a compilation", songsByAlbumName->first.c_str());
    }

    /*
     Step 3: Find the common albumartist for each song and assign
     albumartist to those tracks that don't have it set.
     */
    for (std::map<std::string, std::vector<CSong *> >::iterator j = artists.begin(); j != artists.end(); ++j)
    {
      /*
       Find the common artist(s) for these songs. Take from albumartist tag when present, or use artist tag.
       When from albumartist tag also check albumartistsort tag and take first non-empty value
      */
      std::vector<CSong *> &artistSongs = j->second;
      std::vector<std::string> common;
      std::string albumartistsort;
      if (artistSongs.front()->GetAlbumArtist().empty())
        common = artistSongs.front()->GetArtist();
      else
      {
        common = artistSongs.front()->GetAlbumArtist();
        albumartistsort = artistSongs.front()->GetAlbumArtistSort();
      }
      for (std::vector<CSong *>::iterator k = artistSongs.begin() + 1; k != artistSongs.end(); ++k)
      {
        unsigned int match = 0;
        std::vector<std::string> compare;
        if ((*k)->GetAlbumArtist().empty())
          compare = (*k)->GetArtist();
        else
        {
          compare = (*k)->GetAlbumArtist();
          if (albumartistsort.empty())
            albumartistsort = (*k)->GetAlbumArtistSort();
        }
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
      album.strAlbum = songsByAlbumName->first;

      //Split the albumartist sort string to try and get sort names for individual artists
      std::vector<std::string> sortnames = StringUtils::Split(albumartistsort, g_advancedSettings.m_musicItemSeparator);
      if (sortnames.size() != common.size())
          // Split artist sort names further using multiple possible delimiters, over single separator applied in Tag loader
        sortnames = StringUtils::SplitMulti(sortnames, { ";", ":", "|", "#" });
      
      for (size_t i = 0; i < common.size(); i++)
      {
        album.artistCredits.emplace_back(StringUtils::Trim(common[i]));
        // Set artist sort name providing we have as many as we have artists, 
        // otherwise something is wrong with them so ignore rather than guess.
        if (sortnames.size() == common.size())
          album.artistCredits.back().SetSortName(StringUtils::Trim(sortnames[i]));
      }
      album.bCompilation = compilation;
      for (std::vector<CSong *>::iterator k = artistSongs.begin(); k != artistSongs.end(); ++k)
      {
        if ((*k)->GetAlbumArtist().empty())
          (*k)->SetAlbumArtist(common);
        //! @todo in future we may wish to union up the genres, for now we assume they're the same
        album.genre = (*k)->genre;
        album.strArtistSort = (*k)->GetAlbumArtistSort();
        //       in addition, we may want to use year as discriminating for albums
        album.iYear = (*k)->iYear;
        album.strLabel = (*k)->strRecordLabel;
        album.strType = (*k)->strAlbumType;
        album.songs.push_back(**k);
      }
      albums.push_back(album);
    }
  }
}

CInfoScanner::INFO_RET
CMusicInfoScanner::UpdateAlbumInfo(CAlbum& album,
                                   const ADDON::ScraperPtr& scraper,
                                   bool bAllowSelection,
                                   CGUIDialogProgress* pDialog)
{
  m_musicDatabase.Open();  
  INFO_RET result = UpdateDatabaseAlbumInfo(album, scraper, bAllowSelection, pDialog);
  m_musicDatabase.Close();
  return result;
}

CInfoScanner::INFO_RET
CMusicInfoScanner::UpdateArtistInfo(CArtist& artist,
                                    const ADDON::ScraperPtr& scraper,
                                    bool bAllowSelection,
                                    CGUIDialogProgress* pDialog)
{
  m_musicDatabase.Open();
  INFO_RET result = UpdateDatabaseArtistInfo(artist, scraper, bAllowSelection, pDialog);
  m_musicDatabase.Close();
  return result;
}

int CMusicInfoScanner::RetrieveMusicInfo(const std::string& strDirectory, CFileItemList& items)
{
  MAPSONGS songsMap;

  // get all information for all files in current directory from database, and remove them
  if (m_musicDatabase.RemoveSongsFromPath(strDirectory, songsMap))
    m_needsCleanup = true;

  CFileItemList scannedItems;
  if (ScanTags(items, scannedItems) == INFO_CANCELLED || scannedItems.Size() == 0)
    return 0;

  VECALBUMS albums;
  FileItemsToAlbums(scannedItems, albums, &songsMap);
  FindArtForAlbums(albums, items.GetPath());

  /* Strategy: Having scanned tags and made a list of albums, add them to the library. Only then try
  to scrape additional album and artist information. Music is often tagged to a mixed standard
  - some albums have mbid tags, some don't. Once all the music files have been added to the library,
  the mbid for an artist will be known even if it was only tagged on one song. The artist is best
  scraped with an mbid, so scrape after all the files that may provide that tag have been scanned.
  That artist mbid can then be used to improve the accuracy of scraping other albums by that artist
  even when it was not in the tagging for that album.

  Doing scraping, generally the slower activity, in the background after scanning has fully populated
  the library also means that the user can use their library to select music to play sooner.
  */

  int numAdded = 0;

  // Add all albums to the library, and hence any new song or album artists or other contributors
  for (VECALBUMS::iterator album = albums.begin(); album != albums.end(); ++album)
  {
    if (m_bStop)
      break;

    // mark albums without a title as singles
    if (album->strAlbum.empty())
      album->releaseType = CAlbum::Single;

    album->strPath = strDirectory;
    m_musicDatabase.AddAlbum(*album);
    m_albumsAdded.emplace_back(album->idAlbum);

    /* 
      Make the first attempt (during scanning) to get local album artist art looking for thumbs and 
      fanart in the folder immediately above the album folder. This is for backwards compatibility.
      It can only do this if the folder being processed contains only one album, and can only do so for 
      the first album artist if the album is a collaboration e.g. composer, conductor, orchestra, or by
      several pop artists in their own right.
      It avoids repeatedly processing the same artist by maintaining a set. Adding the album may have added 
      new artists, or provide art for an existing (song) artist, but does not replace any artwork already set.
      Hence once art has been found for an album artist, art is not searched for in other folders.

      It will find art for "various artists", if artwork is located above the folder containing compilatons.
    */
    if (albums.size() == 1 && !album->artistCredits.empty())
    {
      if (m_artistsArt.find(album->artistCredits[0].GetArtistId()) == m_artistsArt.end())
      {
        m_artistsArt.insert(album->artistCredits[0].GetArtistId()); // Artist processed
        std::map<std::string, std::string> art;
        if (!m_musicDatabase.GetArtForItem(album->artistCredits[0].GetArtistId(), MediaTypeArtist, art))
        {
          // Artist does not already have art, so try to find some. 
          // Do not have URL of other available art before scraping, so only ID and path needed
          CArtist artist;
          artist.idArtist = album->artistCredits[0].GetArtistId();
          artist.strPath = URIUtils::GetParentPath(album->strPath);
          m_musicDatabase.SetArtForItem(album->artistCredits[0].GetArtistId(), MediaTypeArtist, GetArtistArtwork(artist, 1));
        }
      }
    }
 
    numAdded += album->songs.size();
  }
  return numAdded;
}

void MUSIC_INFO::CMusicInfoScanner::ScrapeInfoAddedAlbums()
{
  /* Strategy: Having scanned tags, make a list of albums and add them to the library, only then try
  to scrape additional album and artist information. Music is often tagged to a mixed standard
  - some albums have mbid tags, some don't. Once all the music files have been added to the library,
  the mbid for an artist will be known even if it was only tagged on one song. The artist is best
  scraped with an mbid, so scrape after all the files that may provide that tag have been scanned.
  That artist mbid can then be used to improve the accuracy of scraping other albums by that artist
  even when it was not in the tagging for that album.

  Doing scraping, generally the slower activity, in the background after scanning has fully populated
  the library also means that the user can use their library to select music to play sooner.
  */

  /* Scrape additional album and artist data.
  For albums and artists without mbids, matching on album-artist pair can
  be used to identify artist with greater accuracy than artist name alone.
  Artist mbid returned by album scraper is used if we do not already have it.
  Hence scrape album then related artists.
  */
  ADDON::AddonPtr addon;

  ADDON::ScraperPtr albumScraper;
  ADDON::ScraperPtr artistScraper;
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::ADDON_SCRAPER_ALBUMS, addon))
    albumScraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);

  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::ADDON_SCRAPER_ARTISTS, addon))
    artistScraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);

  bool albumartistsonly = !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS);

  if (!albumScraper || !artistScraper)
    return;

  std::set<int> artists;
  for (auto i = 0u; i < m_albumsAdded.size(); ++i)
  {
    if (m_bStop)
      break;
    // Scrape album data
    int albumId = m_albumsAdded[i];
    CAlbum album;
    if (!m_musicDatabase.HasAlbumBeenScraped(albumId))
    {
      if (m_handle)
      {
        float percentage = static_cast<float>(i * 100) / static_cast<float>(m_albumsAdded.size());
        m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
        m_handle->SetPercentage(percentage);
      }

      // Fetch any artist mbids for album artist(s) and song artists when scraping those too.
      m_musicDatabase.GetAlbum(albumId, album, !albumartistsonly);
      UpdateDatabaseAlbumInfo(album, albumScraper, false);

      // Scrape information for artists that have not been scraped before, avoiding repeating
      // unsuccessful attempts for every album and song.
      for (const auto &artistCredit : album.artistCredits)
      {
        if (m_bStop)
          break;

        if (!m_musicDatabase.HasArtistBeenScraped(artistCredit.GetArtistId()) &&
          artists.find(artistCredit.GetArtistId()) == artists.end())
        {
          artists.insert(artistCredit.GetArtistId()); // Artist scraping attempted
          CArtist artist;
          m_musicDatabase.GetArtist(artistCredit.GetArtistId(), artist);
          UpdateDatabaseArtistInfo(artist, artistScraper, false);
        }
      }
      // Only scrape song artists if they are being displayed in artists node by default
      if (!albumartistsonly)
      {
        for (auto &song : album.songs)
        {
          if (m_bStop)
            break;
          for (const auto &artistCredit : song.artistCredits)
          {
            if (m_bStop)
              break;

            CMusicArtistInfo musicArtistInfo;
            if (!m_musicDatabase.HasArtistBeenScraped(artistCredit.GetArtistId()) &&
              artists.find(artistCredit.GetArtistId()) == artists.end())
            {
              artists.insert(artistCredit.GetArtistId()); // Artist scraping attempted
              CArtist artist;
              m_musicDatabase.GetArtist(artistCredit.GetArtistId(), artist);
              UpdateDatabaseArtistInfo(artist, artistScraper, false);
            }
          }
        }
      }
    }
  }
  // Clear list of albums added to prevent them being scraped again
  m_albumsAdded.clear();
}

void MUSIC_INFO::CMusicInfoScanner::RetrieveArtistArt()
{
  bool albumartistsonly = !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS);
  std::set<int> artists;
  for (auto i = 0u; i < m_albumsAdded.size(); ++i)
  {
    if (m_bStop)
      break;
    int albumId = m_albumsAdded[i];
    CAlbum album;
    // Fetch album artist(s) ids
    m_musicDatabase.GetAlbum(albumId, album, false);
    if (m_handle)
    {
      float percentage = static_cast<float>(i * 100) / static_cast<float>(m_albumsAdded.size());
      m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
      m_handle->SetPercentage(percentage);
    }

    // Set art for album artists that have not been processed before, avoiding repeating
    // unsuccessful attempts for every album by that artist.
    for (const auto &artistCredit : album.artistCredits)
    {
      if (m_bStop)
        break;
      if (artists.find(artistCredit.GetArtistId()) == artists.end())
      {
        artists.insert(artistCredit.GetArtistId()); // Artist processed
        std::map<std::string, std::string> art;
        if (!m_musicDatabase.GetArtForItem(artistCredit.GetArtistId(), MediaTypeArtist, art))
        { 
          CArtist artist;
          //Get artist details including available art returned by scraping
          m_musicDatabase.GetArtist(artistCredit.GetArtistId(), artist);
          // Get path for artist in the artists folder (not done in GetArtist)
          m_musicDatabase.GetArtistPath(artist, artist.strPath);
          m_musicDatabase.SetArtForItem(artist.idArtist, MediaTypeArtist, GetArtistArtwork(artist, 1));
        }
      }
    }
    // Only fetch song artist art if they are being displayed in artists node by default
    if (!albumartistsonly)
    {
      for (auto &song : album.songs)
      {
        if (m_bStop)
          break;
        for (const auto &artistCredit : song.artistCredits)
        {
          if (m_bStop)
            break;

          std::map<std::string, std::string> art;
          if (artists.find(artistCredit.GetArtistId()) == artists.end())
          {
            artists.insert(artistCredit.GetArtistId()); // Artist processed
            if (!m_musicDatabase.GetArtForItem(artistCredit.GetArtistId(), MediaTypeArtist, art))
            {
              CArtist artist;
              //Get artist details including available art returned by scraping
              m_musicDatabase.GetArtist(artistCredit.GetArtistId(), artist);
              m_musicDatabase.GetArtistPath(artist, artist.strPath);
              m_musicDatabase.SetArtForItem(artist.idArtist, MediaTypeArtist, GetArtistArtwork(artist, 1));
            }
          }
        }
      }
    }

  }
}

void CMusicInfoScanner::FindArtForAlbums(VECALBUMS &albums, const std::string &path)
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
    if (!albumArt.empty())
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
        albumArt = CTextureUtils::GetWrappedImageURL(art->strFileName, "music");
    }

    if (!albumArt.empty())
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
        if (k->strThumb.empty() && !k->embeddedArt.Empty())
          k->strThumb = CTextureUtils::GetWrappedImageURL(k->strFileName, "music");
      }
    }
  }
  if (albums.size() == 1 && !albumArt.empty())
  {
    // assign to folder thumb as well
    CFileItem albumItem(path, true);
    CMusicThumbLoader loader;
    loader.SetCachedImage(albumItem, "thumb", albumArt);
  }
}

int CMusicInfoScanner::GetPathHash(const CFileItemList &items, std::string &hash)
{
  // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
  if (0 == items.Size()) return 0;
  CDigest digest{CDigest::Type::MD5};
  int count = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr pItem = items[i];
    digest.Update(pItem->GetPath());
    digest.Update((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
    FILETIME time = pItem->m_dateTime;
    digest.Update((unsigned char *)&time, sizeof(FILETIME));
    if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  hash = digest.Finalize();
  return count;
}

CInfoScanner::INFO_RET
CMusicInfoScanner::UpdateDatabaseAlbumInfo(CAlbum& album,
                                           const ADDON::ScraperPtr& scraper,
                                           bool bAllowSelection,
                                           CGUIDialogProgress* pDialog /* = NULL */)
{
  if (!scraper)
    return INFO_ERROR;

  CMusicAlbumInfo albumInfo;

loop:
  CLog::Log(LOGDEBUG, "%s downloading info for: %s", __FUNCTION__, album.strAlbum.c_str());
  INFO_RET albumDownloadStatus = DownloadAlbumInfo(album, scraper, albumInfo, !bAllowSelection, pDialog);
  if (albumDownloadStatus == INFO_NOT_FOUND)
  {
    if (pDialog && bAllowSelection)
    {
      if (!CGUIKeyboardFactory::ShowAndGetInput(album.strAlbum, CVariant{g_localizeStrings.Get(16011)}, false))
        return INFO_CANCELLED;

      std::string strTempArtist(album.GetAlbumArtistString());
      if (!CGUIKeyboardFactory::ShowAndGetInput(strTempArtist, CVariant{g_localizeStrings.Get(16025)}, false))
        return INFO_CANCELLED;

      album.strArtistDesc = strTempArtist;
      goto loop;
    }
    else
    {
      CServiceBroker::GetEventLog().Add(EventPtr(new CMediaLibraryEvent(
        MediaTypeAlbum, album.strPath, 24146,
        StringUtils::Format(g_localizeStrings.Get(24147).c_str(), MediaTypeAlbum, album.strAlbum.c_str()),
        CScraperUrl::GetThumbURL(album.thumbURL.GetFirstThumb()), CURL::GetRedacted(album.strPath), EventLevel::Warning)));
    }
  }
  else if (albumDownloadStatus == INFO_ADDED)
  {
    bool overridetags = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS);
    album.MergeScrapedAlbum(albumInfo.GetAlbum(), overridetags);
    m_musicDatabase.Open();
    m_musicDatabase.UpdateAlbum(album);
    GetAlbumArtwork(album.idAlbum, album);
    m_musicDatabase.Close();
    albumInfo.SetLoaded(true);
  }
  return albumDownloadStatus;
}

CInfoScanner::INFO_RET
CMusicInfoScanner::UpdateDatabaseArtistInfo(CArtist& artist,
                                            const ADDON::ScraperPtr& scraper,
                                            bool bAllowSelection,
                                            CGUIDialogProgress* pDialog /* = NULL */)
{
  if (!scraper)
    return INFO_ERROR;

  CMusicArtistInfo artistInfo;    
loop:
  CLog::Log(LOGDEBUG, "%s downloading info for: %s", __FUNCTION__, artist.strArtist.c_str());
  INFO_RET artistDownloadStatus = DownloadArtistInfo(artist, scraper, artistInfo, !bAllowSelection, pDialog);
  if (artistDownloadStatus == INFO_NOT_FOUND)
  {
    if (pDialog && bAllowSelection)
    {
      if (!CGUIKeyboardFactory::ShowAndGetInput(artist.strArtist, CVariant{g_localizeStrings.Get(16025)}, false))
        return INFO_CANCELLED;
      goto loop;
    }
    else
    {
      CServiceBroker::GetEventLog().Add(EventPtr(new CMediaLibraryEvent(
        MediaTypeArtist, artist.strPath, 24146,
        StringUtils::Format(g_localizeStrings.Get(24147).c_str(), MediaTypeArtist, artist.strArtist.c_str()),
        CScraperUrl::GetThumbURL(artist.thumbURL.GetFirstThumb()), CURL::GetRedacted(artist.strPath), EventLevel::Warning)));
    }
  }
  else if (artistDownloadStatus == INFO_ADDED)
  {
    artist.MergeScrapedArtist(artistInfo.GetArtist(), CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS));
    m_musicDatabase.Open();
    m_musicDatabase.UpdateArtist(artist);
    // If artist art has not been set from <art> tag then look in path or use first available from scraped list
    if (artist.art.empty())
    {
      m_musicDatabase.GetArtistPath(artist, artist.strPath);
      m_musicDatabase.SetArtForItem(artist.idArtist, MediaTypeArtist, GetArtistArtwork(artist, 1));
    }
    m_musicDatabase.Close();
    artistInfo.SetLoaded();
  }
  return artistDownloadStatus;
}

#define THRESHOLD .95f

CInfoScanner::INFO_RET
CMusicInfoScanner::DownloadAlbumInfo(const CAlbum& album,
                                     const ADDON::ScraperPtr& info,
                                     CMusicAlbumInfo& albumInfo,
                                     bool bUseScrapedMBID,
                                     CGUIDialogProgress* pDialog)
{
  if (m_handle)
  {
    m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20321).c_str(), info->Name().c_str()));
    m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
  }

  // clear our scraper cache
  info->ClearCache();

  CMusicInfoScraper scraper(info);
  bool bMusicBrainz = false;
  /*
  When the mbid is derived from tags scraping of album information is done directly
  using that ID, otherwise the lookup is based on album and artist names and can mis-identify the
  album (i.e. classical music has many "Symphony No. 5"). To be able to correct any mistakes a 
  manual refresh of artist information uses either the mbid if derived from tags or the album
  and artist names, not any previously scraped mbid.
  */
  if (!album.strMusicBrainzAlbumID.empty() && (!album.bScrapedMBID || bUseScrapedMBID))
  {
    CScraperUrl musicBrainzURL;
    if (ResolveMusicBrainz(album.strMusicBrainzAlbumID, info, musicBrainzURL))
    {
      CMusicAlbumInfo albumNfo("nfo", musicBrainzURL);
      scraper.GetAlbums().clear();
      scraper.GetAlbums().push_back(albumNfo);
      bMusicBrainz = true;
    }
  }

  // handle nfo files
  std::string path = album.strPath;
  if (path.empty())
    m_musicDatabase.GetAlbumPath(album.idAlbum, path);

  std::string strNfo = URIUtils::AddFileToFolder(path, "album.nfo");
  CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
  CNfoFile nfoReader;
  if (XFILE::CFile::Exists(strNfo))
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", CURL::GetRedacted(strNfo).c_str());
    result = nfoReader.Create(strNfo, info);
    if (result == CInfoScanner::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
      nfoReader.GetDetails(albumInfo.GetAlbum());
      return INFO_ADDED;
    }
    else if (result == CInfoScanner::URL_NFO ||
             result == CInfoScanner::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicAlbumInfo albumNfo("nfo",scrUrl);
      ADDON::ScraperPtr nfoReaderScraper = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG,"-- nfo-scraper: %s", nfoReaderScraper->Name().c_str());
      CLog::Log(LOGDEBUG,"-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
      scraper.SetScraperInfo(nfoReaderScraper);
      scraper.GetAlbums().clear();
      scraper.GetAlbums().push_back(albumNfo);
    }
    else if (result != CInfoScanner::OVERRIDE_NFO)
      CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.CheckValidOrFallback(CServiceBroker::GetSettings().GetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER)))
  { // the current scraper is invalid, as is the default - bail
    CLog::Log(LOGERROR, "%s - current and default scrapers are invalid.  Pick another one", __FUNCTION__);
    return INFO_ERROR;
  }

  if (!scraper.GetAlbumCount())
  {
    scraper.FindAlbumInfo(album.strAlbum, album.GetAlbumArtistString());

    while (!scraper.Completed())
    {
      if (m_bStop)
      {
        scraper.Cancel();
        return INFO_CANCELLED;
      }
      ScannerWait(1);
    }
    /*
    Finding album using xml scraper may request data from Musicbrainz.
    MusicBrainz rate-limits queries to 1 per sec, once we hit the rate-limiter the server
    returns 503 errors for all calls from that IP address.
    To stay below the rate-limit threshold wait 1s before proceeding
    */
    if (!info->IsPython())
      ScannerWait(1000);
  }

  CGUIDialogSelect *pDlg = NULL;
  int iSelectedAlbum=0;
  if ((result == CInfoScanner::NO_NFO || result == CInfoScanner::OVERRIDE_NFO)
      && !bMusicBrainz)
  {
    iSelectedAlbum = -1; // set negative so that we can detect a failure
    if (scraper.Succeeded() && scraper.GetAlbumCount() >= 1)
    {
      double bestRelevance = 0;
      double minRelevance = THRESHOLD;
      if (scraper.GetAlbumCount() > 1) // score the matches
      {
        //show dialog with all albums found
        if (pDialog)
        {
          pDlg = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
          pDlg->SetHeading(CVariant{g_localizeStrings.Get(181)});
          pDlg->Reset();
          pDlg->EnableButton(true, 413); // manual
        }

        for (int i = 0; i < scraper.GetAlbumCount(); ++i)
        {
          CMusicAlbumInfo& info = scraper.GetAlbum(i);
          double relevance = info.GetRelevance();
          if (relevance < 0)
            relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, album.strAlbum, 
                        info.GetAlbum().GetAlbumArtistString(),
                        album.GetAlbumArtistString());

          // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
          // otherwise, perfect matches only
          if (relevance >= std::max(minRelevance, bestRelevance))
          { // we auto-select the best of these
            bestRelevance = relevance;
            iSelectedAlbum = i;
          }
          if (pDialog)
          {
            // set the label to [relevance]  album - artist
            std::string strTemp = StringUtils::Format("[%0.2f]  %s", relevance, info.GetTitle2().c_str());
            CFileItem item(strTemp);
            item.m_idepth = i; // use this to hold the index of the album in the scraper
            pDlg->Add(item);
          }
          if (relevance > .99f) // we're so close, no reason to search further
            break;
        }

        if (pDialog && bestRelevance < THRESHOLD)
        {
          pDlg->Sort(false);
          pDlg->Open();

          // and wait till user selects one
          if (pDlg->GetSelectedItem() < 0)
          { // none chosen
            if (!pDlg->IsButtonPressed())
              return INFO_CANCELLED;

            // manual button pressed
            std::string strNewAlbum = album.strAlbum;
            if (!CGUIKeyboardFactory::ShowAndGetInput(strNewAlbum, CVariant{g_localizeStrings.Get(16011)}, false))
              return INFO_CANCELLED;
            if (strNewAlbum == "")
              return INFO_CANCELLED;

            std::string strNewArtist = album.GetAlbumArtistString();
            if (!CGUIKeyboardFactory::ShowAndGetInput(strNewArtist, CVariant{g_localizeStrings.Get(16025)}, false))
              return INFO_CANCELLED;

            pDialog->SetLine(0, CVariant{strNewAlbum});
            pDialog->SetLine(1, CVariant{strNewArtist});
            pDialog->Progress();

            CAlbum newAlbum = album;
            newAlbum.strAlbum = strNewAlbum;
            newAlbum.strArtistDesc = strNewArtist;

            return DownloadAlbumInfo(newAlbum, info, albumInfo, bUseScrapedMBID, pDialog);
          }
          iSelectedAlbum = pDlg->GetSelectedFileItem()->m_idepth;
        }
      }
      else
      {
        CMusicAlbumInfo& info = scraper.GetAlbum(0);
        double relevance = info.GetRelevance();
        if (relevance < 0)
          relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum,
                                            album.strAlbum,
                                            info.GetAlbum().GetAlbumArtistString(),
                                            album.GetAlbumArtistString());
        if (relevance < THRESHOLD)
          return INFO_NOT_FOUND;

        iSelectedAlbum = 0;
      }
    }

    if (iSelectedAlbum < 0)
      return INFO_NOT_FOUND;

  }

  scraper.LoadAlbumInfo(iSelectedAlbum);
  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      return INFO_CANCELLED;
    }
    ScannerWait(1);
  }
  if (!scraper.Succeeded())
    return INFO_ERROR;
  /*
  Fetching album details using xml scraper may makes requests for data from Musicbrainz.
  MusicBrainz rate-limits queries to 1 per sec, once we hit the rate-limiter the server
  returns 503 errors for all calls from that IP address.
  To stay below the rate-limit threshold wait 1s before proceeding incase next action is
  to scrape another album or artist
  */
  if (!info->IsPython())
    ScannerWait(1000);

  albumInfo = scraper.GetAlbum(iSelectedAlbum);
  
  if (result == CInfoScanner::COMBINED_NFO || result == CInfoScanner::OVERRIDE_NFO)
    nfoReader.GetDetails(albumInfo.GetAlbum(), NULL, true);
  
  return INFO_ADDED;
}

void CMusicInfoScanner::GetAlbumArtwork(long id, const CAlbum &album)
{
  if (album.thumbURL.m_url.size())
  {
    if (m_musicDatabase.GetArtForItem(id, MediaTypeAlbum, "thumb").empty())
    {
      std::string thumb = CScraperUrl::GetThumbURL(album.thumbURL.GetFirstThumb());
      if (!thumb.empty())
      {
        CTextureCache::GetInstance().BackgroundCacheImage(thumb);
        m_musicDatabase.SetArtForItem(id, MediaTypeAlbum, "thumb", thumb);
      }
    }
  }
}

CInfoScanner::INFO_RET
CMusicInfoScanner::DownloadArtistInfo(const CArtist& artist,
                                      const ADDON::ScraperPtr& info,
                                      MUSIC_GRABBER::CMusicArtistInfo& artistInfo,
                                      bool bUseScrapedMBID,
                                      CGUIDialogProgress* pDialog)
{
  if (m_handle)
  {
    m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20320).c_str(), info->Name().c_str()));
    m_handle->SetText(artist.strArtist);
  }

  // clear our scraper cache
  info->ClearCache();

  CMusicInfoScraper scraper(info);
  bool bMusicBrainz = false;
  /*
  When the mbid is derived from tags scraping of artist information is done directly
  using that ID, otherwise the lookup is based on name and can mis-identify the artist
  (many have same name). To be able to correct any mistakes a manual refresh of artist 
  information uses either the mbid if derived from tags or the artist name, not any previously
  scraped mbid.
  */
  if (!artist.strMusicBrainzArtistID.empty() && (!artist.bScrapedMBID || bUseScrapedMBID))
  {
    CScraperUrl musicBrainzURL;
    if (ResolveMusicBrainz(artist.strMusicBrainzArtistID, info, musicBrainzURL))
    {
      CMusicArtistInfo artistNfo("nfo", musicBrainzURL);
      scraper.GetArtists().clear();
      scraper.GetArtists().push_back(artistNfo);
      bMusicBrainz = true;
    }
  }

  // Handle nfo files 
  CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
  CNfoFile nfoReader;
  std::string strNfo;
  std::string path;
  bool existsNFO = false;
  // First look for nfo in the artists folder, the primary location
  path = artist.strPath;
  // Get path when don't already have it.
  bool artistpathfound = !path.empty();
  if (!artistpathfound)
    artistpathfound = m_musicDatabase.GetArtistPath(artist, path);
  if (artistpathfound)
  {
    strNfo = URIUtils::AddFileToFolder(path, "artist.nfo");
    existsNFO = XFILE::CFile::Exists(strNfo);
  }  

  // If not there fall back local to music files (historic location for those album artists with a unique folder)
  if (!existsNFO)
  {
    artistpathfound = m_musicDatabase.GetOldArtistPath(artist.idArtist, path);
    if (artistpathfound)
    {
      strNfo = URIUtils::AddFileToFolder(path, "artist.nfo");
      existsNFO = XFILE::CFile::Exists(strNfo);
    }
    else
      CLog::Log(LOGDEBUG, "%s not have path, nfo file not possible", artist.strArtist.c_str());
  }

  if (existsNFO)
  {
    CLog::Log(LOGDEBUG, "Found matching nfo file: %s", CURL::GetRedacted(strNfo).c_str());
    result = nfoReader.Create(strNfo, info);
    if (result == CInfoScanner::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
      nfoReader.GetDetails(artistInfo.GetArtist());
      return INFO_ADDED;
    }
    else if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicArtistInfo artistNfo("nfo", scrUrl);
      ADDON::ScraperPtr nfoReaderScraper = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG, "-- nfo-scraper: %s", nfoReaderScraper->Name().c_str());
      CLog::Log(LOGDEBUG, "-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
      scraper.SetScraperInfo(nfoReaderScraper);
      scraper.GetArtists().push_back(artistNfo);
    }
    else
      CLog::Log(LOGERROR, "Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.GetArtistCount())
  {
    scraper.FindArtistInfo(artist.strArtist);

    while (!scraper.Completed())
    {
      if (m_bStop)
      {
        scraper.Cancel();
        return INFO_CANCELLED;
      }
      ScannerWait(1);
    }
    /*
    Finding artist using xml scraper makes a request for data from Musicbrainz.
    MusicBrainz rate-limits queries to 1 per sec, once we hit the rate-limiter
    the server returns 503 errors for all calls from that IP address. To stay 
    below the rate-limit threshold wait 1s before proceeding
    */
    if (!info->IsPython())
      ScannerWait(1000);
  }

  int iSelectedArtist = 0;
  if (result == CInfoScanner::NO_NFO && !bMusicBrainz)
  {
    if (scraper.GetArtistCount() >= 1)
    {
      // now load the first match
      if (pDialog && scraper.GetArtistCount() > 1)
      {
        // if we found more then 1 album, let user choose one
        CGUIDialogSelect *pDlg = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
        if (pDlg)
        {
          pDlg->SetHeading(CVariant{g_localizeStrings.Get(21890)});
          pDlg->Reset();
          pDlg->EnableButton(true, 413); // manual

          for (int i = 0; i < scraper.GetArtistCount(); ++i)
          {
            // set the label to artist
            CFileItem item(scraper.GetArtist(i).GetArtist());
            std::string strTemp=scraper.GetArtist(i).GetArtist().strArtist;
            if (!scraper.GetArtist(i).GetArtist().strBorn.empty())
              strTemp += " ("+scraper.GetArtist(i).GetArtist().strBorn+")";
            if (!scraper.GetArtist(i).GetArtist().genre.empty())
            {
              std::string genres = StringUtils::Join(scraper.GetArtist(i).GetArtist().genre, g_advancedSettings.m_musicItemSeparator);
              if (!genres.empty())
                strTemp = StringUtils::Format("[%s] %s", genres.c_str(), strTemp.c_str());
            }
            item.SetLabel(strTemp);
            item.m_idepth = i; // use this to hold the index of the album in the scraper
            pDlg->Add(item);
          }
          pDlg->Open();

          // and wait till user selects one
          if (pDlg->GetSelectedItem() < 0)
          { // none chosen
            if (!pDlg->IsButtonPressed())
              return INFO_CANCELLED;

            // manual button pressed
            std::string strNewArtist = artist.strArtist;
            if (!CGUIKeyboardFactory::ShowAndGetInput(strNewArtist, CVariant{g_localizeStrings.Get(16025)}, false))
              return INFO_CANCELLED;

            if (pDialog)
            {
              pDialog->SetLine(0, CVariant{strNewArtist});
              pDialog->Progress();
            }

            CArtist newArtist;
            newArtist.strArtist = strNewArtist;
            return DownloadArtistInfo(newArtist, info, artistInfo, bUseScrapedMBID, pDialog);
          }
          iSelectedArtist = pDlg->GetSelectedFileItem()->m_idepth;
        }
      }
    }
    else
      return INFO_NOT_FOUND;
  }
  /*
  Fetching artist details using xml scraper makes requests for data from Musicbrainz.
  MusicBrainz rate-limits queries to 1 per sec, once we hit the rate-limiter the server
  returns 503 errors for all calls from that IP address.
  To stay below the rate-limit threshold wait 1s before proceeding incase next action is
  to scrape another album or artist
  */
  if (!info->IsPython())
    ScannerWait(1000);

  scraper.LoadArtistInfo(iSelectedArtist, artist.strArtist);
  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      return INFO_CANCELLED;
    }
    ScannerWait(1);
  }

  if (!scraper.Succeeded())
    return INFO_ERROR;

  artistInfo = scraper.GetArtist(iSelectedArtist);

  if (result == CInfoScanner::COMBINED_NFO)
    nfoReader.GetDetails(artistInfo.GetArtist(), NULL, true);

  return INFO_ADDED;
}

bool CMusicInfoScanner::ResolveMusicBrainz(const std::string &strMusicBrainzID, const ScraperPtr &preferredScraper, CScraperUrl &musicBrainzURL)
{
  // We have a MusicBrainz ID
  // Get a scraper that can resolve it to a MusicBrainz URL & force our
  // search directly to the specific album.
  bool bMusicBrainz = false;
  try
  {
    musicBrainzURL = preferredScraper->ResolveIDToUrl(strMusicBrainzID);
  }
  catch (const ADDON::CScraperError &sce)
  {
    if (sce.FAborted())
      return false;
  }

  if (!musicBrainzURL.m_url.empty())
  {
    CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",preferredScraper->Name().c_str());
    CLog::Log(LOGDEBUG,"-- nfo url: %s", musicBrainzURL.m_url[0].m_url.c_str());
    bMusicBrainz = true;
  }

  return bMusicBrainz;
}

void CMusicInfoScanner::ScannerWait(unsigned int milliseconds)
{
  if (milliseconds > 10)
  {
    CEvent m_StopEvent;
    m_StopEvent.WaitMSec(milliseconds);
  }
  else
    XbmcThreads::ThreadSleep(milliseconds);
}


std::map<std::string, std::string> CMusicInfoScanner::GetArtistArtwork(const CArtist& artist, unsigned int level /* = 3*/)
{
  std::map<std::string, std::string> artwork;
  std::string strFolder;
  if (level > 3)
    level = 3; //Don't go up more than 2 levels of folders

  // check thumb
  std::string thumb;
  if (!artist.strPath.empty())
  {
    strFolder = artist.strPath;
    for (unsigned int i = 0; i < level && thumb.empty(); ++i)
    {
      CFileItem item(strFolder, true);
      thumb = item.GetUserMusicThumb(true);
      strFolder = URIUtils::GetParentPath(strFolder);
    }
  }
  if (thumb.empty())
    thumb = CScraperUrl::GetThumbURL(artist.thumbURL.GetFirstThumb());
  if (!thumb.empty())
  {
    CTextureCache::GetInstance().BackgroundCacheImage(thumb);
    artwork.insert(make_pair("thumb", thumb));
  }

  // check fanart
  std::string fanart;
  if (!artist.strPath.empty())
  {
    strFolder = artist.strPath;
    for (unsigned int i = 0; i < level && fanart.empty(); ++i)
    {
      CFileItem item(strFolder, true);
      fanart = item.GetLocalFanart();
      strFolder = URIUtils::GetParentPath(strFolder);
    }
  }
  if (fanart.empty())
    fanart = artist.fanart.GetImageURL();
  if (!fanart.empty())
  {
    CTextureCache::GetInstance().BackgroundCacheImage(fanart);
    artwork.insert(make_pair("fanart", fanart));
  }

  return artwork;
}

// This function is the Run() function of the IRunnable
// CFileCountReader and runs in a separate thread.
void CMusicInfoScanner::Run()
{
  int count = 0;
  for (std::set<std::string>::iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end() && !m_bStop; ++it)
  {
    count+=CountFilesRecursively(*it);
  }
  m_itemCount = count;
}

// Recurse through all folders we scan and count files
int CMusicInfoScanner::CountFilesRecursively(const std::string& strPath)
{
  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, CServiceBroker::GetFileExtensionProvider().GetMusicExtensions(), DIR_FLAG_NO_FILE_DIRS);

  if (m_bStop)
    return 0;

  // true for recursive counting
  int count = CountFiles(items, true);
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
