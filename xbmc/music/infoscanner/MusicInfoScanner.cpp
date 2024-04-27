/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoScanner.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "MusicAlbumInfo.h"
#include "MusicInfoScraper.h"
#include "NfoFile.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Scraper.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "events/EventLog.h"
#include "events/MediaLibraryEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicThumbLoader.h"
#include "music/MusicUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace MUSIC_GRABBER;
using namespace ADDON;
using KODI::UTILITY::CDigest;

CMusicInfoScanner::CMusicInfoScanner()
: m_fileCountReader(this, "MusicFileCounter")
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
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "OnScanStarted");
  try
  {
    if (m_showDialog && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE))
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

    auto tick = std::chrono::steady_clock::now();
    m_musicDatabase.Open();
    m_bCanInterrupt = true;

    if (m_scanType == 0) // load info from files
    {
      CLog::Log(LOGDEBUG, "{} - Starting scan", __FUNCTION__);

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
      for (const auto& it : m_pathsToScan)
      {
        if (!CDirectory::Exists(it) && !m_bClean)
        {
          /*
           * Note that this will skip scanning (if m_bClean is disabled) if the directory really
           * doesn't exist. Since the music scanner is fed with a list of existing paths from the DB
           * and cleans out all songs under that path as its first step before re-adding files, if
           * the entire source is offline we totally empty the music database in one go.
           */
          CLog::Log(LOGWARNING, "{} directory '{}' does not exist - skipping scan.", __FUNCTION__,
                    it);
          m_seenPaths.insert(it);
          continue;
        }

        // Clear list of albums added by this scan
        m_albumsAdded.clear();
        bool scancomplete = DoScan(it);
        if (scancomplete)
        {
          if (m_albumsAdded.size() > 0)
          {
            // Set local art for added album disc sets and primary album artists
            if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                    CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL) !=
                CSettings::MUSICLIBRARY_ARTWORK_LEVEL_NONE)
              RetrieveLocalArt();

            if (m_flags & SCAN_ONLINE)
              // Download additional album and artist information for the recently added albums.
              // This also identifies any local artist art if it exists, and gives it priority,
              // otherwise it is set to the first available from the remote art that was scraped.
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
          m_musicDatabase.CheckArtistLinksChanged();

          if (m_handle)
            m_handle->SetTitle(g_localizeStrings.Get(331));

          m_musicDatabase.Compress(false);
        }
      }

      m_fileCountReader.StopThread();

      m_musicDatabase.EmptyCache();

      auto elapsed =
          std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - tick);
      CLog::Log(LOGINFO,
                "My Music: Scanning for music info using worker thread, operation took {}s",
                elapsed.count());
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
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bMusicLibraryArtistSortOnUpdate)
      m_musicDatabase.UpdateArtistSortNames();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception while scanning.");
  }
  m_musicDatabase.Close();
  CLog::Log(LOGDEBUG, "{} - Finished scan", __FUNCTION__);

  m_bRunning = false;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "OnScanFinished");

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
  m_flags = flags;

  m_musicDatabase.Open();
  // Check db sources match xml file and update if they don't
  m_musicDatabase.UpdateSources();

  if (strDirectory.empty())
  { // Scan all paths in the database.  We do this by scanning all paths in the
    // db, and crossing them off the list as we go.
    m_musicDatabase.GetPaths(m_pathsToScan);
    m_idSourcePath = -1;
  }
  else
  {
    m_pathsToScan.insert(strDirectory);
    m_idSourcePath = m_musicDatabase.GetSourceFromPath(strDirectory);
  }
  m_musicDatabase.Close();

  m_bClean = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bMusicLibraryCleanOnUpdate;

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
        //Add single album (id and path) as item to scan
        CFileItemPtr item(new CFileItem(strDirectory, false));
        item->GetMusicInfoTag()->SetDatabaseId(params.GetAlbumId(), MediaTypeAlbum);
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
    m_musicDatabase.GetArtistsNav("musicdb://artists/", items, !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS), -1);
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
        //Add single artist (id and path) as item to scan
        CFileItemPtr item(new CFileItem(strDirectory, false));
        item->GetMusicInfoTag()->SetDatabaseId(params.GetAlbumId(), MediaTypeArtist);
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
  const std::vector<std::string> &regexps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_audioExcludeFromScanRegExps;

  if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
    return true;

  if (HasNoMedia(strDirectory))
    return true;

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, CServiceBroker::GetFileExtensionProvider().GetMusicExtensions() + "|.jpg|.tbn|.lrc|.cdg", DIR_FLAG_DEFAULTS);

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
      CLog::Log(LOGDEBUG, "{} Scanning dir '{}' as not in the database", __FUNCTION__,
                CURL::GetRedacted(strDirectory));
    else
      CLog::Log(LOGDEBUG, "{} Rescanning dir '{}' due to change", __FUNCTION__,
                CURL::GetRedacted(strDirectory));

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
    CLog::Log(LOGDEBUG, "{} Skipping dir '{}' due to no change", __FUNCTION__,
              CURL::GetRedacted(strDirectory));
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
  std::vector<std::string> regexps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_audioExcludeFromScanRegExps;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (m_bStop)
      return INFO_CANCELLED;

    CFileItemPtr pItem = items[i];

    if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), regexps))
      continue;

    if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsPicture() || MUSIC::IsLyrics(*pItem))
      continue;

    m_currentItem++;

    CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
    if (!tag.Loaded())
    {
      std::unique_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(*pItem));
      if (nullptr != pLoader)
        pLoader->Load(pItem->GetPath(), tag);
    }

    if (m_handle && m_itemCount>0)
      m_handle->SetPercentage(static_cast<float>(m_currentItem * 100) / static_cast<float>(m_itemCount));

    if (!tag.Loaded() && !pItem->HasCueDocument())
    {
      CLog::Log(LOGDEBUG, "{} - No tag found for: {}", __FUNCTION__, pItem->GetPath());
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
      // Match up item to songs in library previously scanned with this path
      MAPSONGS::iterator songlist = songsMap->find(items[i]->GetPath());
      if (songlist != songsMap->end())
      {
        VECSONGS::iterator foundsong;
        if (songlist->second.size() == 1)
          foundsong = songlist->second.begin();
        else
        {
          // When filename mapped to multiple songs it is from cuesheet, match on disc/track number
          int disctrack = tag.GetTrackAndDiscNumber();
          foundsong = std::find_if(songlist->second.begin(), songlist->second.end(),
                                   [&](const CSong& song) { return disctrack == song.iTrack; });
        }
        if (foundsong != songlist->second.end())
        {
          song.idSong = foundsong->idSong; // Reuse ID
          song.dateNew = foundsong->dateNew; // Keep date originally created
          song.iTimesPlayed = foundsong->iTimesPlayed;
          song.lastPlayed = foundsong->lastPlayed;
          if (song.rating == 0)
            song.rating = foundsong->rating;
          if (song.userrating == 0)
            song.userrating = foundsong->userrating;
          if (song.strThumb.empty())
            song.strThumb = foundsong->strThumb;
        }
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
  for (auto& songsByAlbumName : songsByAlbumNames)
  {
    VECSONGS& songs = songsByAlbumName.second;
    // sort the songs by tracknumber to identify duplicate track numbers
    sort(songs.begin(), songs.end(), SortSongsByTrack);

    // map the songs to their primary artists
    bool tracksOverlap = false;
    bool hasAlbumArtist = false;
    bool isCompilation = true;
    std::string old_DiscSubtitle;

    std::map<std::string, std::vector<CSong *> > artists;
    for (VECSONGS::iterator song = songs.begin(); song != songs.end(); ++song)
    {
      // test for song overlap
      if (song != songs.begin() && song->iTrack == (song - 1)->iTrack)
        tracksOverlap = true;

      if (!song->bCompilation)
        isCompilation = false;

      if (song->strDiscSubtitle != old_DiscSubtitle)
        old_DiscSubtitle = song->strDiscSubtitle;

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
    bool compilation =
        !songsByAlbumName.first.empty() && (isCompilation || !tracksOverlap); // 1+2b+2a
    if (artists.size() == 1)
    {
      std::string artist = artists.begin()->first; StringUtils::ToLower(artist);
      if (!StringUtils::EqualsNoCase(artist, "various") &&
        !StringUtils::EqualsNoCase(artist, "various artists") &&
        !StringUtils::EqualsNoCase(artist, various)) // 3a
        compilation = false;
      else
        // Grab name for use in "various artist" artist
        various = artists.begin()->first;
    }
    else if (hasAlbumArtist) // 3b
      compilation = false;

    // Such a compilation album is stored under a unique artist that matches on Musicbrainz ID
    // the "various artists" artist for music tagged with mbids.
    if (compilation)
    {
      CLog::Log(LOGDEBUG, "Album '{}' is a compilation as there's no overlapping tracks and {}",
                songsByAlbumName.first,
                hasAlbumArtist ? "the album artist is 'Various'"
                               : "there is more than one unique artist");
      // Clear song artists from artists map, put songs under "various artists" mbid entry
      artists.clear();
      for (auto& song : songs)
        artists[VARIOUSARTISTS_MBID].push_back(&song);
    }

    /*
    We also have a compilation album when album name is non-empty and ALL tracks are marked as part of
    a compilation even if an album artist is given, or all songs have the same primary artist. For
    example an anthology - a collection of recordings from various old sources
    combined together such as a "best of", retrospective or rarities type release.

    Such an anthology compilation will not have been caught by the previous tests as it fails 3a and 3b.
    The album artist can be determined just like any normal album.
    */
    if (!compilation && !songsByAlbumName.first.empty() && isCompilation)
    {
      compilation = true;
      CLog::Log(LOGDEBUG,
                "Album '{}' is a compilation as all songs are marked as part of a compilation",
                songsByAlbumName.first);
    }

    /*
     Step 3: Find the common albumartist for each song and assign
     albumartist to those tracks that don't have it set.
     */
    for (auto& j : artists)
    {
      /* Find the common artist(s) for these songs (grouped under primary artist).
      Various artist compilations already under the unique "various artists" mbid.
      Take from albumartist tag when present, or use artist tag.
      When from albumartist tag also check albumartistsort tag and take first non-empty value
      */
      std::vector<CSong*>& artistSongs = j.second;
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
      if (j.first == VARIOUSARTISTS_MBID)
      {
        common.clear();
        common.emplace_back(VARIOUSARTISTS_MBID);
      }

      /*
       Step 4: Assign the album artist for each song that doesn't have it set
       and add to the album vector
       */
      CAlbum album;
      album.strAlbum = songsByAlbumName.first;

      //Split the albumartist sort string to try and get sort names for individual artists
      std::vector<std::string> sortnames = StringUtils::Split(albumartistsort, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
      if (sortnames.size() != common.size())
          // Split artist sort names further using multiple possible delimiters, over single separator applied in Tag loader
        sortnames = StringUtils::SplitMulti(sortnames, { ";", ":", "|", "#" });

      for (size_t i = 0; i < common.size(); i++)
      {
        if (common[i] == VARIOUSARTISTS_MBID)
          /* Treat "various", "various artists" and the localized equivalent name as the same
          album artist as the artist with Musicbrainz ID 89ad4ac3-39f7-470e-963a-56509c546377.
          If adding this artist for the first time then the name will be set to either the primary
          artist read from tags when 3a, or the localized value for "various artists" when not 3a.
          This means that tag values are no longer translated into the current language.
          */
          album.artistCredits.emplace_back(various, VARIOUSARTISTS_MBID);
        else
        {
          album.artistCredits.emplace_back(StringUtils::Trim(common[i]));
          // Set artist sort name providing we have as many as we have artists,
          // otherwise something is wrong with them so ignore rather than guess.
          if (sortnames.size() == common.size())
            album.artistCredits.back().SetSortName(StringUtils::Trim(sortnames[i]));
        }
      }
      album.bCompilation = compilation;
      for (auto& k : artistSongs)
      {
        if (k->GetAlbumArtist().empty())
          k->SetAlbumArtist(common);
        //! @todo in future we may wish to union up the genres, for now we assume they're the same
        album.genre = k->genre;
        album.strArtistSort = k->GetAlbumArtistSort();
        // in addition, we may want to use release date as discriminating between albums
        album.strReleaseDate = k->strReleaseDate,
        album.strLabel = k->strRecordLabel;
        album.strType = k->strAlbumType;
        album.songs.push_back(*k);
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

  /*
  Set thumb for songs and, if only one album in folder, store the thumb for
  the album (music db) and the folder path (in Textures db) too.
  The album and path thumb is either set to the folder art, or failing that to
  the art embedded in the first music file.
  Song thumb is only set when it varies, otherwise it is cleared so that it will
  fallback to the album art (that may be from the first file, or that of the
  folder or set later by scraping from NFO files or remote sources). Clearing
  saves caching repeats of the same image.

  However even if all songs are from one album this may not be the album
  folder. It could be just a subfolder containing some of the songs from a disc
  set e.g. CD1, CD2 etc., or the album could spread across many folders.  In
  this case the album art gets reset every time a folder with songs from just
  that album is processed, and needs to be corrected later once all the parts
  of the album have been scanned.
  */
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
  for (auto& album : albums)
  {
    if (m_bStop)
      break;

    // mark albums without a title as singles
    if (album.strAlbum.empty())
      album.releaseType = CAlbum::Single;

    album.strPath = strDirectory;
    m_musicDatabase.AddAlbum(album, m_idSourcePath);
    m_albumsAdded.insert(album.idAlbum);

    numAdded += static_cast<int>(album.songs.size());
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
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::SCRAPER_ALBUMS, addon))
    albumScraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);

  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::SCRAPER_ARTISTS,
                                                           addon))
    artistScraper = std::dynamic_pointer_cast<ADDON::CScraper>(addon);

  bool albumartistsonly = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS);

  if (!albumScraper || !artistScraper)
    return;

  int i = 0;
  std::set<int> artists;
  for (auto albumId : m_albumsAdded)
  {
    i++;
    if (m_bStop)
      break;
    // Scrape album data
    CAlbum album;
    if (!m_musicDatabase.HasAlbumBeenScraped(albumId))
    {
      if (m_handle)
      {
        m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
        m_handle->SetProgress(i, static_cast<int>(m_albumsAdded.size()));
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
}

/*
  Set thumb for songs and the album(if only one album in folder).
  The album thumb is either set to the folder art, or failing that to the art
  embedded in the first music file. However this does not allow for there being
  other folders with more songs from the album e.g. this was a subfolder CD1
  and there is CD2 etc. yet to be processed
  Song thumb is only set when it varies, otherwise it is cleared so that it will
  fallback to the album art(that may be from the first file, or that of the
  folder or set later by scraping from NFO files or remote sources).Clearing
  saves caching repeats of the same image.
*/
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
    /*
     If we are scanning a directory served over http(s) the root directory for an album will set
     IsInternetStream to true which prevents scanning it for art.  As we can't reach this point
     without having read some tags (and tags are not read from streams) we can safely check for
     that case and set the IsHTTPDirectory property to enable scanning for art.
    */
    if (StringUtils::StartsWithNoCase(path, "http") && StringUtils::EndsWith(path, "/"))
      album.SetProperty("IsHTTPDirectory", true);
    albumArt = album.GetUserMusicThumb(true);
    if (!albumArt.empty())
      albums[0].art["thumb"] = albumArt;
  }
  for (auto& album : albums)
  {
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
    for (auto& song : album.songs)
    {
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
      for (auto& k : album.songs)
        k.strThumb.clear();
    }
    else
    { // more than one piece of art was found for these songs, so cache per song
      for (auto& k : album.songs)
      {
        if (k.strThumb.empty() && !k.embeddedArt.Empty())
          k.strThumb = CTextureUtils::GetWrappedImageURL(k.strFileName, "music");
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

void MUSIC_INFO::CMusicInfoScanner::RetrieveLocalArt()
{
  if (m_handle)
  {
    m_handle->SetTitle(g_localizeStrings.Get(506)); //"Checking media files..."
   //!@todo: title = Checking for local art
  }

  std::set<int> artistsArtDone; // artists processed to avoid unsuccessful repeats
  int count = 0;
  for (auto albumId : m_albumsAdded)
  {
    count++;
    if (m_bStop)
      break;
    CAlbum album;
    m_musicDatabase.GetAlbum(albumId, album, false);
    if (m_handle)
    {
      m_handle->SetText(album.GetAlbumArtistString() + " - " + album.strAlbum);
      m_handle->SetProgress(count, static_cast<int>(m_albumsAdded.size()));
    }

    /*
    Automatically fetch local art from album folder and any disc sets subfolders

    Providing all songs from an album are are under a unique common album
    folder (no songs from other albums) then thumb has been set to local art,
    or failing that to embedded art, during scanning by FindArtForAlbums().
    But when songs are also spread over multiple subfolders within it e.g. disc
    sets, it will have been set to either the art of the last subfolder that was
    processed (if there is any), or from the first song in that subfolder with
    embedded art (if there is any). To correct this and find any thumb in the
    (common) album folder add "thumb" to those missing.
    */
    AddAlbumArtwork(album);

    /*
    Local album artist art

    Look in the nominated "Artist Information Folder" for thumbs and fanart.
    Failing that, for backward compatibility, fallback to the folder immediately
    above the album folder.
    It can only fallback if the album has a unique folder, and can only do so
    for the first album artist if the album is a collaboration e.g. composer,
    conductor, orchestra, or by several pop artists in their own right.
    Avoids repeatedly processing the same artist by maintaining a set.

    Adding the album may have added new artists, or provide art for an existing
    (song) artist, but does not replace any artwork already set. Hence once art
    has been found for an album artist, art is not searched for in other folders.

    It will find art for "various artists", if artwork is located above the
    folder containing compilatons.
    */
    for (auto artistCredit = album.artistCredits.begin(); artistCredit != album.artistCredits.end(); ++artistCredit)
    {
      if (m_bStop)
        break;
      int idArtist = artistCredit->GetArtistId();
      if (artistsArtDone.find(idArtist) == artistsArtDone.end())
      {
        artistsArtDone.insert(idArtist); // Artist processed

        // Get artist and subfolder within the Artist Information Folder
        CArtist artist;
        m_musicDatabase.GetArtist(idArtist, artist);
        m_musicDatabase.GetArtistPath(artist, artist.strPath);
        // Location of local art
        std::string artfolder;
        if (CDirectory::Exists(artist.strPath))
          // When subfolder exists that is only place we look for local art
          artfolder = artist.strPath;
        else if (!album.strPath.empty() && artistCredit == album.artistCredits.begin())
        {
          // If no individual artist subfolder has been found, for primary
          // album artist only look in the folder immediately above the album
          // folder. Not using GetOldArtistPath here because may not have not
          // have scanned all the albums yet.
          artfolder = URIUtils::GetParentPath(album.strPath);
        }
        AddArtistArtwork(artist, artfolder);
      }
    }
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
    KODI::TIME::FileTime time = pItem->m_dateTime;
    digest.Update((unsigned char*)&time, sizeof(KODI::TIME::FileTime));
    if (MUSIC::IsAudio(*pItem) && !pItem->IsPlayList() && !pItem->IsNFO())
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
  INFO_RET albumDownloadStatus(INFO_CANCELLED);
  std::string origArtist(album.GetAlbumArtistString());
  std::string origAlbum(album.strAlbum);

  bool stop(false);
  while (!stop)
  {
    stop = true;
    CLog::Log(LOGDEBUG, "{} downloading info for: {}", __FUNCTION__, album.strAlbum);
    albumDownloadStatus = DownloadAlbumInfo(album, scraper, albumInfo, !bAllowSelection, pDialog);
    if (albumDownloadStatus == INFO_NOT_FOUND)
    {
      if (pDialog && bAllowSelection)
      {
        std::string strTempAlbum(album.strAlbum);
        if (!CGUIKeyboardFactory::ShowAndGetInput(strTempAlbum, CVariant{ g_localizeStrings.Get(16011) }, false))
          albumDownloadStatus = INFO_CANCELLED;
        else
        {
          std::string strTempArtist(album.GetAlbumArtistString());
          if (!CGUIKeyboardFactory::ShowAndGetInput(strTempArtist, CVariant{ g_localizeStrings.Get(16025) }, false))
            albumDownloadStatus = INFO_CANCELLED;
          else
          {
            album.strAlbum = strTempAlbum;
            album.strArtistDesc = strTempArtist;
            stop = false;
          }
        }
      }
      else
      {
        auto eventLog = CServiceBroker::GetEventLog();
        if (eventLog)
          eventLog->Add(EventPtr(new CMediaLibraryEvent(
              MediaTypeAlbum, album.strPath, 24146,
              StringUtils::Format(g_localizeStrings.Get(24147), MediaTypeAlbum, album.strAlbum),
              CScraperUrl::GetThumbUrl(album.thumbURL.GetFirstUrlByType()),
              CURL::GetRedacted(album.strPath), EventLevel::Warning)));
      }
    }
  }

  // Restore original album and artist name, possibly changed by manual entry
  // to get info but should not change outside merge
  album.strAlbum = origAlbum;
  album.strArtistDesc = origArtist;

  if (albumDownloadStatus == INFO_ADDED)
  {
    bool overridetags = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS);
    // Remove art accidentally set by the Python scraper, it only provides URLs of possible artwork
    // Art is selected later applying whitelist and other art preferences
    albumInfo.GetAlbum().art.clear();
    album.MergeScrapedAlbum(albumInfo.GetAlbum(), overridetags);
    m_musicDatabase.UpdateAlbum(album);
    albumInfo.SetLoaded(true);
  }

  // Check album art.
  // Fill any gaps with local art files or use first available from scraped URL list (when it has
  // been successfully scraped) as controlled by whitelist. Do this even when no info added
  // (cancelled, not found or error), there may be new local art files.
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL) !=
          CSettings::MUSICLIBRARY_ARTWORK_LEVEL_NONE &&
      AddAlbumArtwork(album))
    albumDownloadStatus = INFO_ADDED; // Local art added

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
  INFO_RET artistDownloadStatus(INFO_CANCELLED);
  std::string origArtist(artist.strArtist);

  bool stop(false);
  while (!stop)
  {
    stop = true;
    CLog::Log(LOGDEBUG, "{} downloading info for: {}", __FUNCTION__, artist.strArtist);
    artistDownloadStatus = DownloadArtistInfo(artist, scraper, artistInfo, !bAllowSelection, pDialog);
    if (artistDownloadStatus == INFO_NOT_FOUND)
    {
      if (pDialog && bAllowSelection)
      {
        if (!CGUIKeyboardFactory::ShowAndGetInput(artist.strArtist, CVariant{ g_localizeStrings.Get(16025) }, false))
          artistDownloadStatus = INFO_CANCELLED;
        else
          stop = false;
      }
      else
      {
        auto eventLog = CServiceBroker::GetEventLog();
        if (eventLog)
          eventLog->Add(EventPtr(new CMediaLibraryEvent(
              MediaTypeArtist, artist.strPath, 24146,
              StringUtils::Format(g_localizeStrings.Get(24147), MediaTypeArtist, artist.strArtist),
              CScraperUrl::GetThumbUrl(artist.thumbURL.GetFirstUrlByType()),
              CURL::GetRedacted(artist.strPath), EventLevel::Warning)));
      }
    }
  }

  // Restore original artist name, possibly changed by manual entry to get info
  // but should not change outside merge
  artist.strArtist = origArtist;

  if (artistDownloadStatus == INFO_ADDED)
  {
    artist.MergeScrapedArtist(artistInfo.GetArtist(), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS));
    m_musicDatabase.UpdateArtist(artist);
    artistInfo.SetLoaded();
  }

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL) ==
      CSettings::MUSICLIBRARY_ARTWORK_LEVEL_NONE)
    return artistDownloadStatus;

  // Check artist art.
  // Fill any gaps with local art files, or use first available from scraped
  // list (when it has been successfully scraped). Do this even when no info
  // added (cancelled, not found or error), there may be new local art files.
  // Get individual artist subfolder within the Artist Information Folder
  m_musicDatabase.GetArtistPath(artist, artist.strPath);
  // Location of local art
  std::string artfolder;
  if (CDirectory::Exists(artist.strPath))
    // When subfolder exists that is only place we look for art
    artfolder = artist.strPath;
  else
  {
    // Fallback to the old location local to music files (when there is a
    // unique folder). If there is no folder for the artist, and *only* the
    // artist, this will be blank
    m_musicDatabase.GetOldArtistPath(artist.idArtist, artfolder);
  }
  if (AddArtistArtwork(artist, artfolder))
    artistDownloadStatus = INFO_ADDED; // Local art added

  return artistDownloadStatus; // Added, cancelled or not found
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
    m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20321), info->Name()));
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
  bool existsNFO = false;
  std::string path = album.strPath;
  if (path.empty())
    m_musicDatabase.GetAlbumPath(album.idAlbum, path);

  std::string strNfo = URIUtils::AddFileToFolder(path, "album.nfo");
  CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
  CNfoFile nfoReader;
  existsNFO = CFileUtils::Exists(strNfo);
  // When on GUI ask user if they want to ignore nfo and refresh from Internet
  if (existsNFO && pDialog && CGUIDialogYesNo::ShowAndGetInput(10523, 20446))
  {
    existsNFO = false;
    CLog::Log(LOGDEBUG, "Ignoring nfo file: {}", CURL::GetRedacted(strNfo));
  }
  if (existsNFO)
  {
    CLog::Log(LOGDEBUG, "Found matching nfo file: {}", CURL::GetRedacted(strNfo));
    result = nfoReader.Create(strNfo, info);
    if (result == CInfoScanner::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "{} Got details from nfo", __FUNCTION__);
      nfoReader.GetDetails(albumInfo.GetAlbum());
      return INFO_ADDED;
    }
    else if (result == CInfoScanner::URL_NFO ||
             result == CInfoScanner::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicAlbumInfo albumNfo("nfo",scrUrl);
      ADDON::ScraperPtr nfoReaderScraper = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG, "-- nfo-scraper: {}", nfoReaderScraper->Name());
      CLog::Log(LOGDEBUG, "-- nfo url: {}", scrUrl.GetFirstThumbUrl());
      scraper.SetScraperInfo(nfoReaderScraper);
      scraper.GetAlbums().clear();
      scraper.GetAlbums().push_back(albumNfo);
    }
    else if (result != CInfoScanner::OVERRIDE_NFO)
      CLog::Log(LOGERROR, "Unable to find an url in nfo file: {}", strNfo);
  }

  if (!scraper.CheckValidOrFallback(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER)))
  { // the current scraper is invalid, as is the default - bail
    CLog::Log(LOGERROR, "{} - current and default scrapers are invalid.  Pick another one",
              __FUNCTION__);
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
      double minRelevance = static_cast<double>(THRESHOLD);
      if (pDialog || scraper.GetAlbumCount() > 1) // score the matches
      {
        //show dialog with all albums found
        if (pDialog)
        {
          pDlg = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
          pDlg->SetHeading(CVariant{g_localizeStrings.Get(181)});
          pDlg->Reset();
          pDlg->EnableButton(true, 413); // manual
          pDlg->SetUseDetails(true);
        }

        CFileItemList items;
        for (int i = 0; i < scraper.GetAlbumCount(); ++i)
        {
          CMusicAlbumInfo& info = scraper.GetAlbum(i);
          double relevance = static_cast<double>(info.GetRelevance());
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
            std::string strTemp = StringUtils::Format("[{:0.2f}]  {}", relevance, info.GetTitle2());
            CFileItemPtr item(new CFileItem("", false));
            item->SetLabel(strTemp);

            std::string strTemp2;
            if (!scraper.GetAlbum(i).GetAlbum().strType.empty())
              strTemp2 += scraper.GetAlbum(i).GetAlbum().strType;
            if (!scraper.GetAlbum(i).GetAlbum().strReleaseDate.empty())
              strTemp2 += " - " + scraper.GetAlbum(i).GetAlbum().strReleaseDate;
            if (!scraper.GetAlbum(i).GetAlbum().strReleaseStatus.empty())
              strTemp2 += " - " + scraper.GetAlbum(i).GetAlbum().strReleaseStatus;
            if (!scraper.GetAlbum(i).GetAlbum().strLabel.empty())
              strTemp2 += " - " + scraper.GetAlbum(i).GetAlbum().strLabel;
            item->SetLabel2(strTemp2);

            item->SetArt(scraper.GetAlbum(i).GetAlbum().art);

            items.Add(item);
          }
          if (!pDialog && relevance > 0.999) // we're so close, no reason to search further
            break;
        }

        if (pDialog)
        {
          pDlg->Sort(false);
          pDlg->SetItems(items);
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
          iSelectedAlbum = pDlg->GetSelectedItem();
        }
      }
      else
      {
        CMusicAlbumInfo& info = scraper.GetAlbum(0);
        double relevance = static_cast<double>(info.GetRelevance());
        if (relevance < 0)
          relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum,
                                            album.strAlbum,
                                            info.GetAlbum().GetAlbumArtistString(),
                                            album.GetAlbumArtistString());
        if (relevance < static_cast<double>(THRESHOLD))
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

CInfoScanner::INFO_RET
CMusicInfoScanner::DownloadArtistInfo(const CArtist& artist,
                                      const ADDON::ScraperPtr& info,
                                      MUSIC_GRABBER::CMusicArtistInfo& artistInfo,
                                      bool bUseScrapedMBID,
                                      CGUIDialogProgress* pDialog)
{
  if (m_handle)
  {
    m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20320), info->Name()));
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
    existsNFO = CFileUtils::Exists(strNfo);
  }

  // If not there fall back local to music files (historic location for those album artists with a unique folder)
  if (!existsNFO)
  {
    artistpathfound = m_musicDatabase.GetOldArtistPath(artist.idArtist, path);
    if (artistpathfound)
    {
      strNfo = URIUtils::AddFileToFolder(path, "artist.nfo");
      existsNFO = CFileUtils::Exists(strNfo);
    }
    else
      CLog::Log(LOGDEBUG, "{} not have path, nfo file not possible", artist.strArtist);
  }

  // When on GUI ask user if they want to ignore nfo and refresh from Internet
  if (existsNFO && pDialog && CGUIDialogYesNo::ShowAndGetInput(21891, 20446))
  {
    existsNFO = false;
    CLog::Log(LOGDEBUG, "Ignoring nfo file: {}", CURL::GetRedacted(strNfo));
  }

  if (existsNFO)
  {
    CLog::Log(LOGDEBUG, "Found matching nfo file: {}", CURL::GetRedacted(strNfo));
    result = nfoReader.Create(strNfo, info);
    if (result == CInfoScanner::FULL_NFO)
    {
      CLog::Log(LOGDEBUG, "{} Got details from nfo", __FUNCTION__);
      nfoReader.GetDetails(artistInfo.GetArtist());
      return INFO_ADDED;
    }
    else if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
    {
      CScraperUrl scrUrl(nfoReader.ScraperUrl());
      CMusicArtistInfo artistNfo("nfo", scrUrl);
      ADDON::ScraperPtr nfoReaderScraper = nfoReader.GetScraperInfo();
      CLog::Log(LOGDEBUG, "-- nfo-scraper: {}", nfoReaderScraper->Name());
      CLog::Log(LOGDEBUG, "-- nfo url: {}", scrUrl.GetFirstThumbUrl());
      scraper.SetScraperInfo(nfoReaderScraper);
      scraper.GetArtists().push_back(artistNfo);
    }
    else
      CLog::Log(LOGERROR, "Unable to find an url in nfo file: {}", strNfo);
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
            std::string strTemp = scraper.GetArtist(i).GetArtist().strArtist;
            if (!scraper.GetArtist(i).GetArtist().strBorn.empty())
              strTemp += " ("+scraper.GetArtist(i).GetArtist().strBorn+")";
            if (!scraper.GetArtist(i).GetArtist().strDisambiguation.empty())
              strTemp += " - " + scraper.GetArtist(i).GetArtist().strDisambiguation;
            if (!scraper.GetArtist(i).GetArtist().genre.empty())
            {
              std::string genres = StringUtils::Join(scraper.GetArtist(i).GetArtist().genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
              if (!genres.empty())
                strTemp = StringUtils::Format("[{}] {}", genres, strTemp);
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

  if (musicBrainzURL.HasUrls())
  {
    CLog::Log(LOGDEBUG, "-- nfo-scraper: {}", preferredScraper->Name());
    CLog::Log(LOGDEBUG, "-- nfo url: {}", musicBrainzURL.GetFirstThumbUrl());
    bMusicBrainz = true;
  }

  return bMusicBrainz;
}

void CMusicInfoScanner::ScannerWait(unsigned int milliseconds)
{
  if (milliseconds > 10)
  {
    CEvent m_StopEvent;
    m_StopEvent.Wait(std::chrono::milliseconds(milliseconds));
  }
  else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

bool CMusicInfoScanner::AddArtistArtwork(CArtist& artist, const std::string& artfolder)
{
  if (!artist.thumbURL.HasUrls() && artfolder.empty())
    return false; // No local or scraped possible art to process

  if (artist.art.empty())
    m_musicDatabase.GetArtForItem(artist.idArtist, MediaTypeArtist, artist.art);

  std::map<std::string, std::string> addedart;
  std::string strArt;

  // Handle thumb separately, can be from multiple confgurable file names
  if (artist.art.find("thumb") == artist.art.end())
  {
    if (!artfolder.empty())
    { // Local music thumbnail images named by "musiclibrary.musicthumbs"
      CFileItem item(artfolder, true);
      strArt = item.GetUserMusicThumb(true);
    }
    if (strArt.empty())
      strArt = CScraperUrl::GetThumbUrl(artist.thumbURL.GetFirstUrlByType("thumb"));
    if (!strArt.empty())
      addedart.insert(std::make_pair("thumb", strArt));
  }

  // Process additional art types in artist folder
  AddLocalArtwork(addedart, MediaTypeArtist, artist.strArtist, artfolder);

  // Process remote artist art filling gaps with first of scraped art URLs
  AddRemoteArtwork(addedart, MediaTypeArtist, artist.thumbURL);

  int iArtLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL);

  for (const auto& it : addedart)
  {
    // Cache thumb, fanart and other whitelisted artwork immediately
    // (other art types will be cached when first displayed)
    if (iArtLevel != CSettings::MUSICLIBRARY_ARTWORK_LEVEL_ALL || it.first == "thumb" ||
        it.first == "fanart")
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(it.second);
    auto ret = artist.art.insert(it);
    if (ret.second)
      m_musicDatabase.SetArtForItem(artist.idArtist, MediaTypeArtist, it.first, it.second);
  }
  return addedart.size() > 0;
}

bool CMusicInfoScanner::AddAlbumArtwork(CAlbum& album)
{
  // Fetch album path and any subfolders (disc sets).
  // No paths found when songs from different albums are in one folder
  std::vector<std::pair<std::string, int>> paths;
  m_musicDatabase.GetAlbumPaths(album.idAlbum, paths);
  for (const auto& pathpair : paths)
  {
    if (album.strPath.empty())
      album.strPath = pathpair.first.c_str();
    else
      // When more than one album path is the common path
      URIUtils::GetCommonPath(album.strPath, pathpair.first.c_str());
  }

  if (!album.thumbURL.HasUrls() && album.strPath.empty())
    return false; // No local or scraped possible art to process

  if (album.art.empty())
    m_musicDatabase.GetArtForItem(album.idAlbum, MediaTypeAlbum, album.art);
  auto thumb = album.art.find("thumb"); // Find "thumb", may want to replace it

  bool replaceThumb = paths.size() > 1;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MUSICLIBRARY_PREFERONLINEALBUMART))
  {
    // When "prefer online album art" enabled and we have a thumb as embedded art
    // then replace it if we find a scraped cover
    if (thumb != album.art.end() && StringUtils::StartsWith(thumb->second, "image://"))
      replaceThumb = true;
  }

  std::map<std::string, std::string> addedart;
  std::string strArt;

  // Fetch local art from album folder
  // Handle thumbs separately, can be from multiple confgurable file names
  if (replaceThumb || thumb == album.art.end())
  {
    if (!album.strPath.empty())
    { // Local music thumbnail images named by "musiclibrary.musicthumbs"
      CFileItem item(album.strPath, true);
      strArt = item.GetUserMusicThumb(true);
    }
    if (strArt.empty())
      strArt = CScraperUrl::GetThumbUrl(album.thumbURL.GetFirstUrlByType("thumb"));
    if (!strArt.empty())
    {
      if (thumb != album.art.end())
        album.art.erase(thumb);
      addedart.insert(std::make_pair("thumb", strArt));
    }
  }
  // Process additional art types in album folder
  AddLocalArtwork(addedart, MediaTypeAlbum, album.strAlbum, album.strPath);

  // Fetch local art from disc subfolders
  if (paths.size() > 1)
  {
    CMusicThumbLoader loader;
    std::string firstDiscThumb;
    int iDiscThumb = 10000;
    for (const auto& pathpair : paths)
    {
      strArt.clear();

      int discnum = m_musicDatabase.GetDiscnumberForPathID(pathpair.second);
      if (discnum > 0)
      {
        // Handle thumbs separately. Get thumb for path from textures db cached during scan
        // (could be embedded or local file from multiple confgurable file names)
        CFileItem item(pathpair.first.c_str(), true);
        std::string strArtType = StringUtils::Format("{}{}", "thumb", discnum);
        strArt = loader.GetCachedImage(item, "thumb");
        if (strArt.empty())
          strArt = CScraperUrl::GetThumbUrl(album.thumbURL.GetFirstUrlByType(strArtType));
        if (!strArt.empty())
        {
          addedart.insert(std::make_pair(strArtType, strArt));
          // Store thumb of first disc with a thumb
          if (discnum < iDiscThumb)
          {
            iDiscThumb = discnum;
            firstDiscThumb = strArt;
          }
        }
      }
      // Process additional art types in disc subfolder
      AddLocalArtwork(addedart, MediaTypeAlbum, album.strAlbum, pathpair.first, discnum);
    }
    // Finally if we still don't have album thumb then use the art from the
    // first disc in the set with a thumb
    if (!firstDiscThumb.empty() && album.art.find("thumb") == album.art.end())
    {
      m_musicDatabase.SetArtForItem(album.idAlbum, MediaTypeAlbum, "thumb", firstDiscThumb);
      // Assign art as folder thumb (in textures db) as well

      CFileItem albumItem(album.strPath, true);
      loader.SetCachedImage(albumItem, "thumb", firstDiscThumb);
    }
  }

  // Process remote album art filling gaps with first of scraped art URLs
  AddRemoteArtwork(addedart, MediaTypeAlbum, album.thumbURL);

  int iArtLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL);
  for (const auto& it : addedart)
  {
    // Cache thumb, fanart and whitelisted artwork immediately
    // (other art types will be cached when first displayed)
    if (iArtLevel != CSettings::MUSICLIBRARY_ARTWORK_LEVEL_ALL || it.first == "thumb" ||
        it.first == "fanart")
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(it.second);

    auto ret = album.art.insert(it);
    if (ret.second)
      m_musicDatabase.SetArtForItem(album.idAlbum, MediaTypeAlbum, it.first, it.second);
  }
  return addedart.size() > 0;
}

std::vector<CVariant> CMusicInfoScanner::GetArtWhitelist(const MediaType& mediaType, int iArtLevel)
{
  std::vector<CVariant> whitelistarttypes;
  if (iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_BASIC)
  {
    // Basic artist artwork = thumb + fanart (but not "family" fanart1, fanart2 etc.)
    // Basic album artwork = thumb only, thumb handled separately not in whitelist
    if (mediaType == MediaTypeArtist)
      whitelistarttypes.emplace_back("fanart");
  }
  else
  {
    if (mediaType == MediaTypeArtist)
      whitelistarttypes = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
          CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST);
    else
      whitelistarttypes = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
          CSettings::SETTING_MUSICLIBRARY_ALBUMART_WHITELIST);
  }

  return whitelistarttypes;
}

bool CMusicInfoScanner::AddLocalArtwork(std::map<std::string, std::string>& art,
                                        const std::string& mediaType,
                                        const std::string& mediaName,
                                        const std::string& artfolder,
                                        int discnum)
{
  if (artfolder.empty())
    return false;

  int iArtLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL);

  std::vector<CVariant> whitelistarttypes = GetArtWhitelist(mediaType, iArtLevel);
  bool bUseAll = (iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_ALL) ||
                 ((iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM) &&
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                      CSettings::SETTING_MUSICLIBRARY_USEALLLOCALART));

  // Not useall and empty whitelist means no extra art is picked up from either place
  if (!bUseAll && whitelistarttypes.empty())
    return false;

  // Image files used as thumbs
  std::vector<CVariant> thumbs = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
      CSettings::SETTING_MUSICLIBRARY_MUSICTHUMBS);

  // Find local art
  CFileItemList availableArtFiles;
  CDirectory::GetDirectory(artfolder, availableArtFiles,
                           CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);

  for (const auto& artFile : availableArtFiles)
  {
    if (artFile->m_bIsFolder)
      continue;
    std::string strCandidate = URIUtils::GetFileName(artFile->GetPath());
    // Strip media name
    if (!mediaName.empty() && StringUtils::StartsWith(strCandidate, mediaName))
      strCandidate.erase(0, mediaName.length());
    StringUtils::ToLower(strCandidate);
    // Skip files already used as "thumb"
    // Typically folder.jpg but can be from multiple confgurable file names
    if (std::find(thumbs.begin(), thumbs.end(), strCandidate) != thumbs.end())
      continue;
    // Grab and strip file extension
    std::string strExt;
    size_t period = strCandidate.find_last_of("./\\");
    if (period != std::string::npos && strCandidate[period] == '.')
    {
      strExt = strCandidate.substr(period); // ".jpg", ".png" etc.
      strCandidate.erase(period); // "abc16" for file Abc16.jpg
    }
    if (strCandidate.empty())
      continue;
    // Validate art type name
    size_t last_index = strCandidate.find_last_not_of("0123456789");
    std::string strDigits = strCandidate.substr(last_index + 1);
    std::string strFamily = strCandidate.substr(0, last_index + 1); // "abc" of "abc16"
    if (strFamily.empty())
      continue;
    if (!MUSIC_UTILS::IsValidArtType(strCandidate))
      continue;
    // Disc specific art from disc subfolder
    // Skip art where digits of filename do not match disc number
    if (discnum > 0 && !strDigits.empty() && (atoi(strDigits.c_str()) != discnum))
      continue;

    // Use all art, or check for basic level art in whitelist exactly allowing for disc number,
    // or for custom art check whitelist contains art type family (strip trailing digits)
    // e.g. 'fanart', 'fanart1', 'fanart2' etc. all match whitelist entry 'fanart'
    std::string strCheck = strCandidate;
    if (discnum > 0 || iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM)
      strCheck = strFamily;
    if (bUseAll || std::find(whitelistarttypes.begin(), whitelistarttypes.end(), strCheck) !=
                       whitelistarttypes.end())
    {
      if (!strDigits.empty())
      {
        // Catch any variants of music thumbs e.g. folder2.jpg as "thumb2"
        // Used for disc sets when files all in one album folder
        if (std::find(thumbs.begin(), thumbs.end(), strFamily + strExt) != thumbs.end())
          strCandidate = "thumb" + strDigits;
      }
      else if (discnum > 0)
        // Append disc number when candidate art type (and file) not have it
        strCandidate += std::to_string(discnum);

      if (art.find(strCandidate) == art.end())
        art.insert(std::make_pair(strCandidate, artFile->GetPath()));
    }
  }

  return art.size() > 0;
}

bool CMusicInfoScanner::AddRemoteArtwork(std::map<std::string, std::string>& art,
                                         const std::string& mediaType,
                                         const CScraperUrl& thumbURL)
{
  int iArtLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL);

  std::vector<CVariant> whitelistarttypes = GetArtWhitelist(mediaType, iArtLevel);
  bool bUseAll = (iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_ALL) ||
                 ((iArtLevel == CSettings::MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM) &&
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                      CSettings::SETTING_MUSICLIBRARY_USEALLREMOTEART));

  // not useall and empty whitelist means no extra art is picked up from either place
  if (!bUseAll && whitelistarttypes.empty())
    return false;

  // Add online art
  // Done for artists and albums, so not need repeating at disc level
  for (const auto& url : thumbURL.GetUrls())
  {
    // Art type is encoded into the scraper XML held in thumbURL as optional "aspect=" field.
    // Those URL without aspect are also returned for all other type values.
    // Loop through all the first URLS of each type except "thumb" and add if art missing
    if (url.m_aspect.empty() || url.m_aspect == "thumb")
      continue;
    if (!bUseAll)
    { // Check whitelist for art type family e.g. "discart" for aspect="discart2"
      std::string strName = url.m_aspect;
      if (iArtLevel != CSettings::MUSICLIBRARY_ARTWORK_LEVEL_BASIC)
        StringUtils::TrimRight(strName, "0123456789");
      if (std::find(whitelistarttypes.begin(), whitelistarttypes.end(), strName) ==
          whitelistarttypes.end())
        continue;
    }
    if (art.find(url.m_aspect) == art.end())
    {
      std::string strArt = CScraperUrl::GetThumbUrl(url);
      if (!strArt.empty())
        art.insert(std::make_pair(url.m_aspect, strArt));
    }
  }

  return art.size() > 0;
}

// This function is the Run() function of the IRunnable
// CFileCountReader and runs in a separate thread.
void CMusicInfoScanner::Run()
{
  int count = 0;
  for (auto& it : m_pathsToScan)
  {
    count += CountFilesRecursively(it);
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
    else if (MUSIC::IsAudio(*pItem) && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  return count;
}
