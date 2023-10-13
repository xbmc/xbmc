/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicUtils.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaType.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/FileUtils.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "view/GUIViewState.h"

#include <memory>

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace std::chrono_literals;

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
                              const std::string& type,
                              const int itemID,
                              CMusicDatabase& db)
  {
    if (!pSongItem->HasMusicInfoTag())
      return false;
    int idSong = pSongItem->GetMusicInfoTag()->GetDatabaseId();
    if (idSong <= 0)
      return false;
    bool result = false;
    if (type == MediaTypeAlbum)
      // Update art when song is from album
      result = (itemID == pSongItem->GetMusicInfoTag()->GetAlbumId());
    else if (type == MediaTypeArtist)
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
    std::string type = pItem->GetMusicInfoTag()->GetType();
    CMusicDatabase db;
    if (!db.Open())
      return false;
    if (!m_newArt.empty())
      db.SetArtForItem(itemID, type, m_artType, m_newArt);
    else
      db.RemoveArtForItem(itemID, type, m_artType);
    // Artwork changed so set datemodified field for artist, album or song
    db.SetItemUpdated(itemID, type);

    /* Update the art of the songs of the current music playlist.
      Song thumb is often a fallback from the album and fanart is from the artist(s).
      Clear the art if it is a song from the album or by the artist
      (as song or album artist) that has modified artwork. The new artwork gets
      loaded when the playlist is shown.
      */
    bool clearcache(false);
    const PLAYLIST::CPlayList& playlist =
        CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC);

    for (int i = 0; i < playlist.size(); ++i)
    {
      CFileItemPtr songitem = playlist[i];
      if (HasSongExtraArtChanged(songitem, type, itemID, db))
      {
        songitem->ClearArt(); // Art gets reloaded when the current playist is shown
        clearcache = true;
      }
    }
    if (clearcache)
    {
      // Clear the music playlist from cache
      CFileItemList items("playlistmusic://");
      items.RemoveDiscCache(WINDOW_MUSIC_PLAYLIST);
    }

    // Similarly update the art of the currently playing song so it shows on OSD
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingAudio() && g_application.CurrentFileItem().HasMusicInfoTag())
    {
      CFileItemPtr songitem = std::make_shared<CFileItem>(g_application.CurrentFileItem());
      if (HasSongExtraArtChanged(songitem, type, itemID, db))
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
  for (const auto& artType : GetArtTypesToScan(tag.GetType()))
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
  std::map<std::string, std::string> currentArt;
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
  for (const auto& artType : db.GetAvailableArtTypesForItem(tag.GetDatabaseId(), tag.GetType()))
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
  if (tag.GetType() != MediaTypeArtist && tag.GetType() != MediaTypeAlbum &&
      tag.GetType() != MediaTypeSong)
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
    if (type == "banner")
      artitem->SetLabel(g_localizeStrings.Get(20020));
    else if (type == "fanart")
      artitem->SetLabel(g_localizeStrings.Get(20445));
    else if (type == "poster")
      artitem->SetLabel(g_localizeStrings.Get(20021));
    else if (type == "thumb")
      artitem->SetLabel(g_localizeStrings.Get(21371));
    else
      artitem->SetLabel(type);
    // Set art type as art item property
    artitem->SetProperty("arttype", type);
    // Set current art as art item thumb
    if (musicitem.HasArt(type))
      artitem->SetArt("thumb", musicitem.GetArt(type));
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
    if (!CGUIKeyboardFactory::ShowAndGetInput(strArtTypeName,
                                              CVariant{g_localizeStrings.Get(13516)}, false))
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
    dialog->Add(g_localizeStrings.Get(38022));
    for (int i = 1; i <= 10; i++)
      dialog->Add(StringUtils::Format("{}: {}", g_localizeStrings.Get(563), i));
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
  if (tag && tag->GetType() == MediaTypeSong && tag->GetDatabaseId() > 0)
    // Use song ID when known
    job = new CSetSongRatingJob(tag->GetDatabaseId(), userrating);
  else
    job = new CSetSongRatingJob(pItem->GetPath(), userrating);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}

std::vector<std::string> GetArtTypesToScan(const MediaType& mediaType)
{
  std::vector<std::string> arttypes;
  // Get default types of art that are to be automatically fetched during scanning
  if (mediaType == MediaTypeArtist)
  {
    arttypes = {"thumb", "fanart"};
    for (auto& artType : CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
             CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST))
    {
      if (find(arttypes.begin(), arttypes.end(), artType.asString()) == arttypes.end())
        arttypes.emplace_back(artType.asString());
    }
  }
  else if (mediaType == MediaTypeAlbum)
  {
    arttypes = {"thumb"};
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
class CAsyncGetItemsForPlaylist : public IRunnable
{
public:
  CAsyncGetItemsForPlaylist(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
    : m_item(item), m_queuedItems(queuedItems)
  {
  }

  ~CAsyncGetItemsForPlaylist() override = default;

  void Run() override
  {
    // fast lookup is needed here
    m_queuedItems.SetFastLookup(true);

    m_musicDatabase.Open();
    GetItemsForPlaylist(m_item);
    m_musicDatabase.Close();
  }

private:
  void GetItemsForPlaylist(const std::shared_ptr<CFileItem>& item);

  const std::shared_ptr<CFileItem> m_item;
  CFileItemList& m_queuedItems;
  CMusicDatabase m_musicDatabase;
};

SortDescription GetSortDescription(const CGUIViewState& state, const CFileItemList& items)
{
  SortDescription sortDescTrackNumber;

  auto sortDescriptions = state.GetSortDescriptions();
  for (auto& sortDescription : sortDescriptions)
  {
    if (sortDescription.sortBy == SortByTrackNumber)
    {
      // check whether at least one item has actually a track number set
      for (const auto& item : items)
      {
        if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetTrackNumber() > 0)
        {
          // First choice for folders containing a single album
          sortDescTrackNumber = sortDescription;
          sortDescTrackNumber.sortOrder = SortOrderAscending;
          break; // leave items loop. we can still find ByArtistThenYear. so, no return here.
        }
      }
    }
    else if (sortDescription.sortBy == SortByArtistThenYear)
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
            sortDescription.sortOrder = SortOrderAscending;
            return sortDescription;
          }
          lastAlbumId = tag->GetAlbumId();
        }
      }
    }
  }

  if (sortDescTrackNumber.sortBy != SortByNone)
    return sortDescTrackNumber;
  else
    return state.GetSortMethod(); // last resort
}

void CAsyncGetItemsForPlaylist::GetItemsForPlaylist(const std::shared_ptr<CFileItem>& item)
{
  if (item->IsParentFolder() || !item->CanQueue() || item->IsRAR() || item->IsZIP())
    return;

  if (item->IsMusicDb() && item->m_bIsFolder && !item->IsParentFolder())
  {
    // we have a music database folder, just grab the "all" item underneath it
    XFILE::CMusicDatabaseDirectory dir;

    if (!dir.ContainsSongs(item->GetPath()))
    {
      // grab the ALL item in this category
      // Genres will still require 2 lookups, and queuing the entire Genre folder
      // will require 3 lookups (genre, artist, album)
      CMusicDbUrl musicUrl;
      if (musicUrl.FromString(item->GetPath()))
      {
        musicUrl.AppendPath("-1/");

        const auto allItem = std::make_shared<CFileItem>(musicUrl.ToString(), true);
        allItem->SetCanQueue(true); // workaround for CanQueue() check above
        GetItemsForPlaylist(allItem);
      }
      return;
    }
  }

  if (item->m_bIsFolder)
  {
    // Check if we add a locked share
    if (item->m_bIsShareOrDrive)
    {
      if (!g_passwordManager.IsItemUnlocked(item.get(), "music"))
        return;
    }

    CFileItemList items;
    XFILE::CDirectory::GetDirectory(item->GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);

    const std::unique_ptr<CGUIViewState> state(
        CGUIViewState::GetViewState(WINDOW_MUSIC_NAV, items));
    if (state)
    {
      LABEL_MASKS labelMasks;
      state->GetSortMethodLabelMasks(labelMasks);

      const CLabelFormatter fileFormatter(labelMasks.m_strLabelFile, labelMasks.m_strLabel2File);
      const CLabelFormatter folderFormatter(labelMasks.m_strLabelFolder,
                                            labelMasks.m_strLabel2Folder);
      for (const auto& i : items)
      {
        if (i->IsLabelPreformatted())
          continue;

        if (i->m_bIsFolder)
          folderFormatter.FormatLabels(i.get());
        else
          fileFormatter.FormatLabels(i.get());
      }

      SortDescription sortDesc;
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_MUSIC_NAV)
        sortDesc = state->GetSortMethod();
      else
        sortDesc = GetSortDescription(*state, items);

      if (sortDesc.sortBy == SortByLabel)
        items.ClearSortState();

      items.Sort(sortDesc);
    }

    for (const auto& i : items)
    {
      GetItemsForPlaylist(i);
    }
  }
  else
  {
    if (item->IsPlayList())
    {
      const std::unique_ptr<PLAYLIST::CPlayList> playList(
          PLAYLIST::CPlayListFactory::Create(*item));
      if (!playList)
      {
        CLog::Log(LOGERROR, "{} failed to create playlist {}", __FUNCTION__, item->GetPath());
        return;
      }

      if (!playList->Load(item->GetPath()))
      {
        CLog::Log(LOGERROR, "{} failed to load playlist {}", __FUNCTION__, item->GetPath());
        return;
      }

      for (int i = 0; i < playList->size(); ++i)
      {
        GetItemsForPlaylist((*playList)[i]);
      }
    }
    else if (item->IsInternetStream() && !item->IsMusicDb())
    {
      // just queue the internet stream, it will be expanded on play
      m_queuedItems.Add(item);
    }
    else if (item->IsPlugin() && item->GetProperty("isplayable").asBoolean())
    {
      // python files can be played
      m_queuedItems.Add(item);
    }
    else if (!item->IsNFO() && (item->IsAudio() || item->IsVideo()))
    {
      const auto itemCheck = m_queuedItems.Get(item->GetPath());
      if (!itemCheck || itemCheck->GetStartOffset() != item->GetStartOffset())
      {
        // add item
        m_musicDatabase.SetPropertiesForFileItem(*item);
        m_queuedItems.Add(item);
      }
    }
  }
}

void ShowToastNotification(const CFileItem& item, int titleId)
{
  std::string localizedMediaType;
  std::string title;

  if (item.HasMusicInfoTag())
  {
    localizedMediaType = CMediaTypes::GetCapitalLocalization(item.GetMusicInfoTag()->GetType());
    title = item.GetMusicInfoTag()->GetTitle();
  }

  if (title.empty())
    title = item.GetLabel();
  if (title.empty())
    return; // no meaningful toast possible.

  const std::string message =
      localizedMediaType.empty() ? title : localizedMediaType + ": " + title;

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(titleId),
                                        message);
}

std::string GetMusicDbItemPath(const CFileItem& item)
{
  std::string path = item.GetPath();
  if (!URIUtils::IsMusicDb(path))
    path = item.GetProperty("original_listitem_url").asString();

  if (URIUtils::IsMusicDb(path))
    return path;

  return {};
}

void AddItemToPlayListAndPlay(const std::shared_ptr<CFileItem>& itemToQueue,
                              const std::shared_ptr<CFileItem>& itemToPlay,
                              const std::string& player)
{
  // recursively add items to list
  CFileItemList queuedItems;
  MUSIC_UTILS::GetItemsForPlayList(itemToQueue, queuedItems);

  auto& playlistPlayer = CServiceBroker::GetPlaylistPlayer();
  playlistPlayer.ClearPlaylist(PLAYLIST::TYPE_MUSIC);
  playlistPlayer.Reset();
  playlistPlayer.Add(PLAYLIST::TYPE_MUSIC, queuedItems);

  // figure out where to start playback
  PLAYLIST::CPlayList& playList = playlistPlayer.GetPlaylist(PLAYLIST::TYPE_MUSIC);
  int pos = 0;
  if (itemToPlay)
  {
    for (const std::shared_ptr<CFileItem>& queuedItem : queuedItems)
    {
      if (queuedItem->IsSamePath(itemToPlay.get()))
        break;

      pos++;
    }
  }

  if (playlistPlayer.IsShuffled(PLAYLIST::TYPE_MUSIC))
  {
    playList.Swap(0, playList.FindOrder(pos));
    pos = 0;
  }

  playlistPlayer.SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);
  playlistPlayer.Play(pos, player);
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

void PlayItem(const std::shared_ptr<CFileItem>& itemIn,
              const std::string& player,
              ContentUtils::PlayMode mode /* = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM */)
{
  auto item = itemIn;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!itemIn->CanQueue())
  {
    // make a copy to not alter the original item
    item = std::make_shared<CFileItem>(*itemIn);
    item->SetCanQueue(true);
  }

  if (item->m_bIsFolder)
  {
    AddItemToPlayListAndPlay(item, nullptr, player);
  }
  else if (item->HasMusicInfoTag())
  {
    if (mode == ContentUtils::PlayMode::PLAY_FROM_HERE ||
        (mode == ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM && IsAutoPlayNextItem(*item)))
    {
      // Add item and all its siblings to the playlist and play. Prefer musicdb path if available,
      // because it provides more information than just a plain file system path for example.
      std::string parentPath = item->GetProperty("ParentPath").asString();
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
      auto& playlistPlayer = CServiceBroker::GetPlaylistPlayer();
      playlistPlayer.Reset();
      playlistPlayer.SetCurrentPlaylist(PLAYLIST::TYPE_NONE);
      playlistPlayer.Play(item, player);
    }
  }
}

void QueueItem(const std::shared_ptr<CFileItem>& itemIn, QueuePosition pos)
{
  auto item = itemIn;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!itemIn->CanQueue())
  {
    // make a copy to not alter the original item
    item = std::make_shared<CFileItem>(*itemIn);
    item->SetCanQueue(true);
  }

  auto& player = CServiceBroker::GetPlaylistPlayer();

  PLAYLIST::Id playlistId = player.GetCurrentPlaylist();
  if (playlistId == PLAYLIST::TYPE_NONE)
  {
    const auto& components = CServiceBroker::GetAppComponents();
    playlistId = components.GetComponent<CApplicationPlayer>()->GetPreferredPlaylist();
  }

  if (playlistId == PLAYLIST::TYPE_NONE)
    playlistId = PLAYLIST::TYPE_MUSIC;

  // Check for the partymode playlist item, do nothing when "PartyMode.xsp" not exists
  if (item->IsSmartPlayList() && !CFileUtils::Exists(item->GetPath()))
  {
    const auto profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
    if (item->GetPath() == profileManager->GetUserDataItem("PartyMode.xsp"))
      return;
  }

  const int oldSize = player.GetPlaylist(playlistId).size();

  CFileItemList queuedItems;
  GetItemsForPlayList(item, queuedItems);

  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled())
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (pos == QueuePosition::POSITION_BEGIN && appPlayer->IsPlaying())
    player.Insert(playlistId, queuedItems,
                  CServiceBroker::GetPlaylistPlayer().GetCurrentSong() + 1);
  else
    player.Add(playlistId, queuedItems);

  bool playbackStarted = false;

  if (!appPlayer->IsPlaying() && player.GetPlaylist(playlistId).size())
  {
    const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    if (winID == WINDOW_MUSIC_NAV)
    {
      CGUIViewState* viewState = CGUIViewState::GetViewState(winID, queuedItems);
      if (viewState)
        viewState->SetPlaylistDirectory("playlistmusic://");
    }

    player.Reset();
    player.SetCurrentPlaylist(playlistId);
    player.Play(oldSize, ""); // start playing at the first new item

    playbackStarted = true;
  }

  if (!playbackStarted)
  {
    if (pos == QueuePosition::POSITION_END)
      ShowToastNotification(*item, 38082); // Added to end of playlist
    else
      ShowToastNotification(*item, 38083); // Added to playlist to play next
  }
}

bool GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
{
  CAsyncGetItemsForPlaylist getItems(item, queuedItems);
  return CGUIDialogBusy::Wait(&getItems,
                              500, // 500ms before busy dialog appears
                              true); // can be cancelled
}

bool IsItemPlayable(const CFileItem& item)
{
  // Exclude all parent folders
  if (item.IsParentFolder())
    return false;

  // Exclude all video library items
  if (item.IsVideoDb() || StringUtils::StartsWithNoCase(item.GetPath(), "library://video/"))
    return false;

  // Exclude other components
  if (item.IsPVR() || item.IsPlugin() || item.IsScript() || item.IsAddonsPath())
    return false;

  // Exclude special items
  if (StringUtils::StartsWithNoCase(item.GetPath(), "newsmartplaylist://") ||
      StringUtils::StartsWithNoCase(item.GetPath(), "newplaylist://"))
    return false;

  // Include playlists located at one of the possible music playlist locations
  if (item.IsPlayList())
  {
    if (StringUtils::StartsWithNoCase(item.GetMimeType(), "audio/"))
      return true;

    if (StringUtils::StartsWithNoCase(item.GetPath(), "special://musicplaylists/") ||
        StringUtils::StartsWithNoCase(item.GetPath(), "special://profile/playlists/music/"))
      return true;

    // Has user changed default playlists location and the list is located there?
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    std::string path = settings->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
    StringUtils::TrimRight(path, "/");
    if (StringUtils::StartsWith(item.GetPath(), StringUtils::Format("{}/music/", path)))
      return true;

    if (!item.m_bIsFolder)
    {
      // Unknown location. Type cannot be determined for non-folder items.
      return false;
    }
  }

  if (item.m_bIsFolder &&
      (item.IsMusicDb() || StringUtils::StartsWithNoCase(item.GetPath(), "library://music/")))
  {
    // Exclude top level nodes - eg can't play 'genres' just a specific genre etc
    const XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE node =
        XFILE::CMusicDatabaseDirectory::GetDirectoryParentType(item.GetPath());
    if (node == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_OVERVIEW)
      return false;

    return true;
  }

  if (item.HasMusicInfoTag() && item.CanQueue())
    return true;
  else if (!item.m_bIsFolder && item.IsAudio())
    return true;
  else if (item.m_bIsFolder)
  {
    // Not a music-specific folder (just file:// or nfs://). Allow play if context is Music window.
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_MUSIC_NAV &&
        item.GetPath() != "add") // Exclude "Add music source" item
      return true;
  }
  return false;
}

} // namespace MUSIC_UTILS
