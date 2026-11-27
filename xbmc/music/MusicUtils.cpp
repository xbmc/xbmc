/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "PartyMode.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "filesystem/LibraryPaths.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/PlaylistDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "jobs/JobManager.h"
#include "media/MediaType.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListEntryRules.h"
#include "playlists/PlayListFileItemClassify.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/ArtTypes.h"
#include "utils/Artwork.h"
#include "utils/FileUtils.h"
#include "utils/ItemProperties.h"
#include "utils/PlaceholderPaths.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "view/GUIViewState.h"

#include <algorithm>
#include <memory>
#include <optional>

using namespace KODI;
using namespace KODI::VIDEO;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace std::chrono_literals;
using KODI::MEDIA::MediaSection;

namespace MUSIC_UTILS
{
class CSetArtJob : public CJob
{
  CFileItemPtr pItem;
  std::string m_artType;
  std::string m_newArt;

public:
  CSetArtJob(const CFileItemPtr& item, const std::string& type, const std::string& newArt)
    : pItem(item), m_artType(type), m_newArt(newArt)
  {
  }

  ~CSetArtJob(void) override = default;

  bool HasSongExtraArtChanged(const CFileItemPtr& pSongItem,
                              MEDIA::TYPE type,
                              const int itemID,
                              const CMusicDatabase& db)
  {
    if (!pSongItem->HasMusicInfoTag())
      return false;
    int idSong = pSongItem->GetMusicInfoTag()->GetDatabaseId();
    if (idSong <= 0)
      return false;
    bool result = false;
    if (type == MEDIA::TYPE::ALBUM)
      // Update art when song is from album
      result = (itemID == pSongItem->GetMusicInfoTag()->GetAlbumId());
    else if (type == MEDIA::TYPE::ARTIST)
    {
      // Update art when artist is song or album artist of the song
      if (pSongItem->HasProperty("artistid"))
      {
        // Check artistid property when we have it
        for (CVariant::const_iterator_array varid =
                 pSongItem->GetProperty("artistid").begin_array();
             varid != pSongItem->GetProperty("artistid").end_array(); ++varid)
        {
          int idArtist = static_cast<int>(varid->asInteger());
          result = (itemID == idArtist);
          if (result)
            break;
        }
      }
      else
      { // Check song artists in database
        result = db.IsSongArtist(idSong, itemID);
      }
      if (!result)
      {
        // Check song album artists
        result = db.IsSongAlbumArtist(idSong, itemID);
      }
    }
    return result;
  }

  // Asynchronously update song, album or artist art in library
  // and trigger update to album & artist art of the currently playing song
  // and songs queued in the current playlist
  bool DoWork(void) override
  {
    int itemID = pItem->GetMusicInfoTag()->GetDatabaseId();
    if (itemID <= 0)
      return false;
    const std::string& type = pItem->GetMusicInfoTag()->GetType();
    const MEDIA::TYPE mediaType = pItem->GetMusicInfoTag()->GetMediaType();
    CMusicDatabase db;
    if (!db.Open())
      return false;
    if (!m_newArt.empty())
      db.SetArtForItem(itemID, type, m_artType, m_newArt);
    else
      db.RemoveArtForItem(itemID, type, m_artType);
    // Artwork changed so set datemodified field for artist, album or song
    db.SetItemUpdated(itemID, mediaType);

    /* Update the art of the songs of the current music playlist.
      Song thumb is often a fallback from the album and fanart is from the artist(s).
      Clear the art if it is a song from the album or by the artist
      (as song or album artist) that has modified artwork. The new artwork gets
      loaded when the playlist is shown.
      */
    bool clearcache(false);
    const auto playLists = CServiceBroker::GetPlayLists();

    for (const auto& entry : playLists->GetPlayList(PLAYLIST::Audio).GetEntries())
    {
      if (HasSongExtraArtChanged(entry.item, mediaType, itemID, db))
      {
        CFileItem songitem(*entry.item);
        songitem.ClearArt();
        playLists->UpdateItem(PLAYLIST::Audio, songitem);
        clearcache = true;
      }
    }
    if (clearcache)
    {
      // Clear the music playlist from cache
      CFileItemList items(XFILE::CPlaylistDirectory::PathOf(PLAYLIST::Audio));
      items.RemoveDiscCache(WINDOW_MUSIC_PLAYLIST);
    }

    // Similarly update the art of the currently playing song so it shows on OSD
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingAudio() && g_application.CurrentFileItem().HasMusicInfoTag())
    {
      CFileItemPtr songitem = std::make_shared<CFileItem>(g_application.CurrentFileItem());
      if (HasSongExtraArtChanged(songitem, mediaType, itemID, db))
        g_application.UpdateCurrentPlayArt();
    }

    db.Close();
    return true;
  }
};

class CSetSongRatingJob : public CJob
{
  std::string strPath;
  int idSong;
  int iUserrating;

public:
  CSetSongRatingJob(const std::string& filePath, int userrating)
    : strPath(filePath), idSong(-1), iUserrating(userrating)
  {
  }

  CSetSongRatingJob(int songId, int userrating) : strPath(), idSong(songId), iUserrating(userrating)
  {
  }

  ~CSetSongRatingJob(void) override = default;

  bool DoWork(void) override
  {
    // Asynchronously update song userrating in library
    CMusicDatabase db;
    if (db.Open())
    {
      if (idSong > 0)
        db.SetSongUserrating(idSong, iUserrating);
      else
        db.SetSongUserrating(strPath, iUserrating);
      db.Close();
    }

    return true;
  }
};

void UpdateArtJob(const std::shared_ptr<CFileItem>& pItem,
                  const std::string& strType,
                  const std::string& strArt)
{
  // Asynchronously update that type of art in the database
  CSetArtJob* job = new CSetArtJob(pItem, strType, strArt);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}

// Add art types required in Kodi and configured by the user
void AddHardCodedAndExtendedArtTypes(std::vector<std::string>& artTypes, const CMusicInfoTag& tag)
{
  for (const auto& artType : GetArtTypesToScan(tag.GetMediaType()))
  {
    if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
      artTypes.push_back(artType);
  }
}

// Add art types currently assigned to the media item
void AddCurrentArtTypes(std::vector<std::string>& artTypes,
                        const CMusicInfoTag& tag,
                        CMusicDatabase& db)
{
  KODI::ART::Artwork currentArt;
  db.GetArtForItem(tag.GetDatabaseId(), tag.GetType(), currentArt);
  for (const auto& art : currentArt)
  {
    if (!art.second.empty() && find(artTypes.begin(), artTypes.end(), art.first) == artTypes.end())
      artTypes.push_back(art.first);
  }
}

// Add art types that exist for other media items of the same type
void AddMediaTypeArtTypes(std::vector<std::string>& artTypes,
                          const CMusicInfoTag& tag,
                          CMusicDatabase& db)
{
  std::vector<std::string> dbArtTypes;
  db.GetArtTypes(tag.GetType(), dbArtTypes);
  for (const auto& artType : dbArtTypes)
  {
    if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
      artTypes.push_back(artType);
  }
}

// Add art types from available but unassigned artwork for this media item
void AddAvailableArtTypes(std::vector<std::string>& artTypes,
                          const CMusicInfoTag& tag,
                          CMusicDatabase& db)
{
  for (const auto& artType : db.GetAvailableArtTypesForItem(tag.GetDatabaseId(), tag.GetMediaType()))
  {
    if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
      artTypes.push_back(artType);
  }
}

bool FillArtTypesList(CFileItem& musicitem, CFileItemList& artlist)
{
  const CMusicInfoTag& tag = *musicitem.GetMusicInfoTag();
  if (tag.GetDatabaseId() < 1 || tag.GetType().empty())
    return false;
  const MEDIA::TYPE type = tag.GetMediaType();
  if (type != MEDIA::TYPE::ARTIST && type != MEDIA::TYPE::ALBUM && type != MEDIA::TYPE::SONG)
    return false;

  artlist.Clear();

  CMusicDatabase db;
  db.Open();

  std::vector<std::string> artTypes;

  AddHardCodedAndExtendedArtTypes(artTypes, tag);
  AddCurrentArtTypes(artTypes, tag, db);
  AddMediaTypeArtTypes(artTypes, tag, db);
  AddAvailableArtTypes(artTypes, tag, db);

  db.Close();

  for (const auto& type : artTypes)
  {
    CFileItemPtr artitem(new CFileItem(type, false));
    // Localise the names of common types of art
    if (type == ART::TYPE::BANNER)
      artitem->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20020));
    else if (type == ART::TYPE::FANART)
      artitem->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20445));
    else if (type == ART::TYPE::POSTER)
      artitem->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20021));
    else if (type == ART::TYPE::THUMB)
      artitem->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21371));
    else
      artitem->SetLabel(type);
    // Set art type as art item property
    artitem->SetProperty("arttype", type);
    // Set current art as art item thumb
    if (musicitem.HasArt(type))
      artitem->SetArt(ART::TYPE::THUMB, musicitem.GetArt(type));
    artlist.Add(artitem);
  }

  return !artlist.IsEmpty();
}

std::string ShowSelectArtTypeDialog(CFileItemList& artitems)
{
  // Prompt for choice
  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!dialog)
    return "";

  dialog->SetHeading(CVariant{13521});
  dialog->Reset();
  dialog->SetUseDetails(true);
  dialog->EnableButton(true, 13516);

  dialog->SetItems(artitems);
  dialog->Open();

  if (dialog->IsButtonPressed())
  {
    // Get the new art type name
    std::string strArtTypeName;
    if (!CGUIKeyboardFactory::ShowAndGetInput(
            strArtTypeName,
            CVariant{CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13516)},
            false))
      return "";
    // Add new type to the list of art types
    CFileItemPtr artitem(new CFileItem(strArtTypeName, false));
    artitem->SetLabel(strArtTypeName);
    artitem->SetProperty("arttype", strArtTypeName);
    artitems.Add(artitem);

    return strArtTypeName;
  }

  return dialog->GetSelectedFileItem()->GetProperty("arttype").asString();
}

int ShowSelectRatingDialog(int iSelected)
{
  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (dialog)
  {
    dialog->SetHeading(CVariant{38023});
    dialog->Add(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(38022));
    for (int i = 1; i <= 10; i++)
      dialog->Add(StringUtils::Format(
          "{}: {}", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(563), i));
    dialog->SetSelected(iSelected);
    dialog->Open();

    int userrating = dialog->GetSelectedItem();
    userrating = std::max(userrating, -1);
    userrating = std::min(userrating, 10);
    return userrating;
  }
  return -1;
}

void UpdateSongRatingJob(const std::shared_ptr<CFileItem>& pItem, int userrating)
{
  // Asynchronously update the song user rating in music library
  const CMusicInfoTag* tag = pItem->GetMusicInfoTag();
  CSetSongRatingJob* job;
  if (tag && tag->GetMediaType() == MEDIA::TYPE::SONG && tag->GetDatabaseId() > 0)
    // Use song ID when known
    job = new CSetSongRatingJob(tag->GetDatabaseId(), userrating);
  else
    job = new CSetSongRatingJob(pItem->GetPath(), userrating);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}

std::vector<std::string> GetArtTypesToScan(MEDIA::TYPE mediaType)
{
  std::vector<std::string> arttypes;
  // Get default types of art that are to be automatically fetched during scanning
  if (mediaType == MEDIA::TYPE::ARTIST)
  {
    arttypes = {ART::TYPE::THUMB, ART::TYPE::FANART};
    for (auto& artType : CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
             CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST))
    {
      if (find(arttypes.begin(), arttypes.end(), artType.asString()) == arttypes.end())
        arttypes.emplace_back(artType.asString());
    }
  }
  else if (mediaType == MEDIA::TYPE::ALBUM)
  {
    arttypes = {ART::TYPE::THUMB};
    for (auto& artType : CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
             CSettings::SETTING_MUSICLIBRARY_ALBUMART_WHITELIST))
    {
      if (find(arttypes.begin(), arttypes.end(), artType.asString()) == arttypes.end())
        arttypes.emplace_back(artType.asString());
    }
  }
  return arttypes;
}

bool IsValidArtType(const std::string& potentialArtType)
{
  // Check length and is ascii
  return potentialArtType.length() <= 25 &&
         std::find_if_not(potentialArtType.begin(), potentialArtType.end(),
                          StringUtils::isasciialphanum) == potentialArtType.end();
}

} // namespace MUSIC_UTILS

namespace
{
class CAsyncGetItemsForPlaylist : public IRunnable, private PLAYLIST::IEntryRules
{
public:
  CAsyncGetItemsForPlaylist(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems,
                            const std::shared_ptr<CFileItem>& startAt)
    : m_item(item), m_queuedItems(queuedItems), m_startAt(startAt)
  {
  }

  ~CAsyncGetItemsForPlaylist() override = default;

  void Run() override
  {
    m_musicDatabase.Open();
    m_startPosition =
        CApplicationPlayLists::ExpandToEntries(m_item, *this, m_startAt, m_queuedItems);
    m_musicDatabase.Close();
  }

  int GetStartPosition() const { return m_startPosition.value_or(-1); }

private:
  std::shared_ptr<CFileItem> Redirect(const std::shared_ptr<CFileItem>& folder) override;
  bool IsUnlocked(CFileItem& source) override;
  void Arrange(const CFileItem& folder,
               CFileItemList& items,
               std::shared_ptr<CFileItem>& startAt) override;
  std::shared_ptr<CFileItem> Accept(const std::shared_ptr<CFileItem>& file,
                                    const CFileItemList& entries) override;

  const std::shared_ptr<CFileItem> m_item;
  CFileItemList& m_queuedItems;
  CMusicDatabase m_musicDatabase;
  const std::shared_ptr<CFileItem> m_startAt;
  std::optional<int> m_startPosition;
};

SortDescription GetSortDescription(const CGUIViewState& state, const CFileItemList& items)
{
  SortDescription sortDescTrackNumber;

  auto sortDescriptions = state.GetSortDescriptions();
  for (auto& sortDescription : sortDescriptions)
  {
    if (sortDescription.sortBy == SortBy::TRACK_NUMBER)
    {
      // check whether at least one item has actually a track number set
      for (const auto& item : items)
      {
        if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetTrackNumber() > 0)
        {
          // First choice for folders containing a single album
          sortDescTrackNumber = sortDescription;
          sortDescTrackNumber.sortOrder = SortOrder::ASCENDING;
          break; // leave items loop. we can still find ByArtistThenYear. so, no return here.
        }
      }
    }
    else if (sortDescription.sortBy == SortBy::ARTIST_THEN_YEAR)
    {
      // check whether songs from at least two different albums are in the list
      int lastAlbumId = -1;
      for (const auto& item : items)
      {
        if (item->HasMusicInfoTag())
        {
          const auto tag = item->GetMusicInfoTag();
          if (lastAlbumId != -1 && tag->GetAlbumId() != lastAlbumId)
          {
            // First choice for folders containing multiple albums
            sortDescription.sortOrder = SortOrder::ASCENDING;
            return sortDescription;
          }
          lastAlbumId = tag->GetAlbumId();
        }
      }
    }
  }

  if (sortDescTrackNumber.sortBy != SortBy::NONE)
    return sortDescTrackNumber;
  else
    return state.GetSortMethod(); // last resort
}

std::shared_ptr<CFileItem> CAsyncGetItemsForPlaylist::Redirect(const std::shared_ptr<CFileItem>& folder)
{
  if (!MUSIC::IsMusicDb(*folder) || folder->IsParentFolder())
    return folder;

  XFILE::CMusicDatabaseDirectory dir;
  if (dir.ContainsSongs(folder->GetPath()))
    return folder;

  // a music database folder above the songs: take the "all" item underneath it

  // Genres will still require 2 lookups, and queuing the entire Genre folder
  // will require 3 lookups (genre, artist, album)
  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(folder->GetPath()))
    return nullptr;
  musicUrl.AppendPath("-1/");
  return std::make_shared<CFileItem>(musicUrl.ToString(), true);
}

bool CAsyncGetItemsForPlaylist::IsUnlocked(CFileItem& source)
{
  return g_passwordManager.IsItemUnlocked(&source, MediaSection::MUSIC);
}

void CAsyncGetItemsForPlaylist::Arrange(const CFileItem& folder,
                                        CFileItemList& items,
                                        std::shared_ptr<CFileItem>& startAt)
{
  const std::unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(WINDOW_MUSIC_NAV, items));
  if (!state)
    return;

  LABEL_MASKS labelMasks;
  state->GetSortMethodLabelMasks(labelMasks);
  CLabelFormatter::FormatItemLabels(items, labelMasks);

  SortDescription sortDesc;
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_MUSIC_NAV)
    sortDesc = state->GetSortMethod();
  else
    sortDesc = GetSortDescription(*state, items);

  if (sortDesc.sortBy == SortBy::LABEL)
    items.ClearSortState();

  items.Sort(sortDesc);
}

std::shared_ptr<CFileItem> CAsyncGetItemsForPlaylist::Accept(const std::shared_ptr<CFileItem>& file,
                                                             const CFileItemList& entries)
{
  if (!PLAYLIST::CanBeEntry(*file) || (!MUSIC::IsAudio(*file) && !IsVideo(*file)))
    return nullptr;

  const auto queued = entries.Get(file->GetPath());
  if (queued && queued->GetStartOffset() == file->GetStartOffset())
    return nullptr;

  m_musicDatabase.SetPropertiesForFileItem(*file);
  return file;
}

void ShowToastNotification(const CFileItem& item, int titleId)
{
  std::string localizedMediaType;
  std::string title;

  if (item.HasMusicInfoTag())
  {
    localizedMediaType = GetCapitalLocalization(item.GetMusicInfoTag()->GetMediaType());
    title = item.GetMusicInfoTag()->GetTitle();
  }

  if (title.empty())
    title = item.GetLabel();
  if (title.empty())
    return; // no meaningful toast possible.

  const std::string message =
      localizedMediaType.empty() ? title : localizedMediaType + ": " + title;

  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Info,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(titleId), message);
}

std::string GetMusicDbItemPath(const CFileItem& item)
{
  std::string path = item.GetPath();
  if (!URIUtils::IsMusicDb(path))
    path = item.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString();

  if (URIUtils::IsMusicDb(path))
    return path;

  return {};
}

void AddItemToPlayListAndPlay(const std::shared_ptr<CFileItem>& itemToQueue,
                              const std::shared_ptr<CFileItem>& itemToPlay,
                              const std::string& player)
{
  CFileItemList queuedItems;
  int start = -1;
  MUSIC_UTILS::GetItemsForPlayList(itemToQueue, queuedItems, itemToPlay, &start);
  CServiceBroker::GetPlayLists()->PlayExpanded(PLAYLIST::Audio, queuedItems, start, itemToPlay,
                                               {.player = player});
}
} // unnamed namespace

namespace MUSIC_UTILS
{
bool IsAutoPlayNextItem(const CFileItem& item)
{
  if (!item.HasMusicInfoTag())
    return false;

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  return settings->GetBool(CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM) &&
         !settings->GetBool(CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT);
}

void PlayItem(const std::shared_ptr<CFileItem>& item,
              const std::string& player,
              ContentUtils::PlayMode mode /* = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM */)
{
  if (item->IsFolder())
  {
    AddItemToPlayListAndPlay(item, nullptr, player);
  }
  else if (MUSIC::IsAudio(*item))
  {
    if (mode == ContentUtils::PlayMode::PLAY_FROM_HERE ||
        (mode == ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM && IsAutoPlayNextItem(*item)))
    {
      // Add item and all its siblings to the playlist and play. Prefer musicdb path if available,
      // because it provides more information than just a plain file system path for example.
      std::string parentPath = item->GetProperty(ITEM::PROPERTY::PARENT_PATH).asString();
      if (parentPath.empty())
      {
        std::string path = GetMusicDbItemPath(*item);
        if (path.empty())
          path = item->GetPath();

        URIUtils::GetParentPath(path, parentPath);

        if (parentPath.empty())
        {
          CLog::LogF(LOGERROR, "Unable to obtain parent path for '{}'", item->GetPath());
          return;
        }
      }

      const auto parentItem = std::make_shared<CFileItem>(parentPath, true);
      if (item->GetStartOffset() == STARTOFFSET_RESUME)
        parentItem->SetStartOffset(STARTOFFSET_RESUME);

      AddItemToPlayListAndPlay(parentItem, item, player);
    }
    else // mode == PlayMode::PLAY_ONLY_THIS
    {
      // song, so just play it
      CServiceBroker::GetPlayLists()->PlayItem(PLAYLIST::Audio, item, {.player = player});
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Unable to play item {}", item->GetPath());
  }
}

void QueueItem(const std::shared_ptr<CFileItem>& item, QueuePosition pos)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto playLists = CServiceBroker::GetPlayLists();

  const PLAYLIST::Type type = playLists->GetQueueType(PLAYLIST::Audio);

  // Check for the partymode playlist item, do nothing when "PartyMode.xsp" does not exist
  if (PLAYLIST::IsSmartPlayList(*item) && !CFileUtils::Exists(item->GetPath()) &&
      item->GetPath() == PARTYMODE::RulesPath(PLAYLIST::Audio))
    return;

  CFileItemList queuedItems;
  GetItemsForPlayList(item, queuedItems);

  const int first = playLists->Queue(type, queuedItems,
                                     pos == QueuePosition::POSITION_BEGIN
                                         ? CApplicationPlayLists::Placement::Next
                                         : CApplicationPlayLists::Placement::End);
  if (first < 0)
    return;

  if (!components.GetComponent<CApplicationPlayer>()->IsPlaying())
    playLists->PlayFrom(type, first);
  else if (pos == QueuePosition::POSITION_END)
    ShowToastNotification(*item, 38082); // Added to end of playlist
  else
    ShowToastNotification(*item, 38083); // Added to playlist to play next
}

bool GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems,
                         const std::shared_ptr<CFileItem>& startAt /* = nullptr */,
                         int* startPosition /* = nullptr */)
{
  CAsyncGetItemsForPlaylist getItems(item, queuedItems, startAt);
  const bool done = CGUIDialogBusy::Wait(&getItems,
                              500, // 500ms before busy dialog appears
                              true); // can be cancelled
  if (startPosition)
    *startPosition = getItems.GetStartPosition();
  return done;
}

namespace
{
bool IsNonExistingUserPartyModePlaylist(const CFileItem& item)
{
  if (!PLAYLIST::IsSmartPlayList(item))
    return false;

  const std::string& path{item.GetPath()};
  return path == PARTYMODE::RulesPath(PLAYLIST::Audio) && !CFileUtils::Exists(path);
}

bool IsEmptyMusicItem(const CFileItem& item)
{
  //! @todo Poor man's way to detect empty music info tags (inspired by CVideoInfoTag::IsEmpty())
  return item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetTitle().empty();
}

} // unnamed namespace

bool IsItemPlayable(const CFileItem& item)
{
  // Exclude all parent folders
  if (item.IsParentFolder())
    return false;

  // Exclude all video library items
  if (IsVideoDb(item) || StringUtils::StartsWithNoCase(item.GetPath(), MEDIA::LIBRARY_PATH::VIDEO))
    return false;

  // Exclude other components
  if (item.IsPVR() || item.IsAddonsPath())
    return false;

  // Exclude special items
  if (ITEM::PLACEHOLDER::IsNewPlaylist(item.GetPath()))
    return false;

  // Include playlists located at one of the possible music playlist locations
  if (PLAYLIST::IsPlayList(item))
  {
    if (StringUtils::StartsWithNoCase(item.GetMimeType(), "audio/"))
      return true;

    if (CUtil::IsInPlaylistsFolder(item.GetPath(), MediaSection::MUSIC))
      return true;

    if (!item.IsFolder() && !item.HasMusicInfoTag())
    {
      // Unknown location. Type cannot be determined for non-folder items.
      return false;
    }
  }

  if (IsNonExistingUserPartyModePlaylist(item))
    return false;

  if (item.IsFolder() &&
      (MUSIC::IsMusicDb(item) ||
       StringUtils::StartsWithNoCase(item.GetPath(), MEDIA::LIBRARY_PATH::MUSIC)))
  {
    // Exclude top level nodes - eg can't play 'genres' just a specific genre etc
    const auto node = XFILE::CMusicDatabaseDirectory::GetDirectoryParentType(item.GetPath());
    if (node == XFILE::MUSICDATABASEDIRECTORY::NodeType::OVERVIEW)
      return false;

    return true;
  }

  if (item.IsPlugin() && MUSIC::IsAudio(item) && !IsEmptyMusicItem(item) &&
      item.GetProperty(ITEM::PROPERTY::IS_PLAYABLE).asBoolean(false))
  {
    return true;
  }
  else if (item.HasMusicInfoTag() && item.CanQueue() && !item.IsPlugin() && !item.IsScript())
  {
    return true;
  }
  else if (!item.IsFolder() && MUSIC::IsAudio(item) && !IsEmptyMusicItem(item))
  {
    return true;
  }
  else if (item.IsFolder() && !item.IsPlugin() && !item.IsScript())
  {
    // Not a music-specific folder (just file:// or nfs://). Allow play if context is Music window.
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_MUSIC_NAV &&
        item.GetPath() != ITEM::PLACEHOLDER::ADD_SOURCE) // Exclude "Add music source" item
      return true;
  }
  return false;
}

} // namespace MUSIC_UTILS
