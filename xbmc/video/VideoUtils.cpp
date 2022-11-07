/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoUtils.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "filesystem/Directory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "view/GUIViewState.h"

namespace
{
class CAsyncGetItemsForPlaylist : public IRunnable
{
public:
  CAsyncGetItemsForPlaylist(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
    : m_item(item),
      m_resume(item->GetStartOffset() == STARTOFFSET_RESUME),
      m_queuedItems(queuedItems)
  {
  }

  ~CAsyncGetItemsForPlaylist() override = default;

  void Run() override
  {
    // fast lookup is needed here
    m_queuedItems.SetFastLookup(true);

    GetItemsForPlaylist(m_item);
  }

private:
  void GetItemsForPlaylist(const std::shared_ptr<CFileItem>& item);

  const std::shared_ptr<CFileItem> m_item;
  const bool m_resume{false};
  CFileItemList& m_queuedItems;
};

void CAsyncGetItemsForPlaylist::GetItemsForPlaylist(const std::shared_ptr<CFileItem>& item)
{
  if (item->IsParentFolder() || !item->CanQueue() || item->IsRAR() || item->IsZIP())
    return;

  if (item->m_bIsFolder)
  {
    // check if it's a folder with dvd or bluray files, then just add the relevant file
    const std::string mediapath = item->GetOpticalMediaPath();
    if (!mediapath.empty())
    {
      m_queuedItems.Add(std::make_shared<CFileItem>(mediapath, false));
      return;
    }

    // Check if we add a locked share
    if (!item->IsPVR() && item->m_bIsShareOrDrive)
    {
      if (!g_passwordManager.IsItemUnlocked(item.get(), "video"))
        return;
    }

    CFileItemList items;
    XFILE::CDirectory::GetDirectory(item->GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);

    int viewStateWindowId = WINDOW_VIDEO_NAV;
    if (URIUtils::IsPVRRadioRecordingFileOrFolder(item->GetPath()))
      viewStateWindowId = WINDOW_RADIO_RECORDINGS;
    else if (URIUtils::IsPVRTVRecordingFileOrFolder(item->GetPath()))
      viewStateWindowId = WINDOW_TV_RECORDINGS;

    const std::unique_ptr<CGUIViewState> state(
        CGUIViewState::GetViewState(viewStateWindowId, items));
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

      if (items.GetSortMethod() == SortByLabel)
        items.ClearSortState();

      items.Sort(state->GetSortMethod());
    }

    if (m_resume)
    {
      // put last played item at the begin of the playlist; add start offsets for videos
      std::shared_ptr<CFileItem> lastPlayedItem;
      CDateTime lastPlayed;
      for (const auto& i : items)
      {
        if (!i->HasVideoInfoTag())
          continue;

        const auto videoTag = i->GetVideoInfoTag();

        const CBookmark& bookmark = videoTag->GetResumePoint();
        if (bookmark.IsSet())
          i->SetStartOffset(CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));

        const CDateTime& currLastPlayed = videoTag->m_lastPlayed;
        if (currLastPlayed.IsValid() && (!lastPlayed.IsValid() || (lastPlayed < currLastPlayed)))
        {
          lastPlayedItem = i;
          lastPlayed = currLastPlayed;
        }
      }

      if (lastPlayedItem)
      {
        items.Remove(lastPlayedItem.get());
        items.AddFront(lastPlayedItem, 0);
      }
    }

    int watchedMode;
    if (m_resume)
      watchedMode = WatchedModeUnwatched;
    else
      watchedMode = CMediaSettings::GetInstance().GetWatchedMode(items.GetContent());

    const bool unwatchedOnly = watchedMode == WatchedModeUnwatched;
    const bool watchedOnly = watchedMode == WatchedModeWatched;
    for (const auto& i : items)
    {
      if (i->m_bIsFolder)
      {
        std::string path = i->GetPath();
        URIUtils::RemoveSlashAtEnd(path);
        if (StringUtils::EndsWithNoCase(path, "sample")) // skip sample folders
          continue;
      }
      else if (i->HasVideoInfoTag() &&
               ((unwatchedOnly && i->GetVideoInfoTag()->GetPlayCount() > 0) ||
                (watchedOnly && i->GetVideoInfoTag()->GetPlayCount() <= 0)))
        continue;

      GetItemsForPlaylist(i);
    }
  }
  else if (item->IsPlayList())
  {
    const std::unique_ptr<PLAYLIST::CPlayList> playList(PLAYLIST::CPlayListFactory::Create(*item));
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
  else if (item->IsInternetStream())
  {
    // just queue the internet stream, it will be expanded on play
    m_queuedItems.Add(item);
  }
  else if (item->IsPlugin() && item->GetProperty("isplayable").asBoolean())
  {
    // a playable python files
    m_queuedItems.Add(item);
  }
  else if (item->IsVideoDb())
  {
    // this case is needed unless we allow IsVideo() to return true for videodb items,
    // but then we have issues with playlists of videodb items
    const auto itemCopy = std::make_shared<CFileItem>(*item->GetVideoInfoTag());
    itemCopy->SetStartOffset(item->GetStartOffset());
    m_queuedItems.Add(itemCopy);
  }
  else if (!item->IsNFO() && item->IsVideo())
  {
    m_queuedItems.Add(item);
  }
}

} // unnamed namespace

namespace VIDEO_UTILS
{
void PlayItem(const std::shared_ptr<CFileItem>& itemIn)
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

  if (item->m_bIsFolder && !item->IsPlugin())
  {
    // recursively add items to list
    CFileItemList queuedItems;
    GetItemsForPlayList(item, queuedItems);

    auto& player = CServiceBroker::GetPlaylistPlayer();
    player.ClearPlaylist(PLAYLIST::TYPE_VIDEO);
    player.Reset();
    player.Add(PLAYLIST::TYPE_VIDEO, queuedItems);
    player.SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
    player.Play();
  }
  else if (item->HasVideoInfoTag())
  {
    // single item, play it
    CServiceBroker::GetPlaylistPlayer().Play(item, "");
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
  const auto& components = CServiceBroker::GetAppComponents();

  // Determine the proper list to queue this element
  PLAYLIST::Id playlistId = player.GetCurrentPlaylist();
  if (playlistId == PLAYLIST::TYPE_NONE)
    playlistId = components.GetComponent<CApplicationPlayer>()->GetPreferredPlaylist();

  if (playlistId == PLAYLIST::TYPE_NONE)
    playlistId = PLAYLIST::TYPE_VIDEO;

  CFileItemList queuedItems;
  GetItemsForPlayList(item, queuedItems);

  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  if (pos == QueuePosition::POSITION_BEGIN &&
      components.GetComponent<CApplicationPlayer>()->IsPlaying())
    player.Insert(playlistId, queuedItems, player.GetCurrentSong() + 1);
  else
    player.Add(playlistId, queuedItems);

  player.SetCurrentPlaylist(playlistId);

  // Note: video does not auto play on queue like music
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
  if (item.IsParentFolder())
    return false;

  if (item.IsDeleted())
    return false;

  // Include all PVR recordings and recordings folders
  if (URIUtils::IsPVRRecordingFileOrFolder(item.GetPath()))
    return true;

  // Include Live TV
  if (!item.m_bIsFolder && (item.IsLiveTV() || item.IsEPG()))
    return true;

  // Exclude all music library items
  if (item.IsMusicDb() || StringUtils::StartsWithNoCase(item.GetPath(), "library://music/"))
    return false;

  // Exclude other components
  if (item.IsPlugin() || item.IsScript() || item.IsAddonsPath())
    return false;

  // Exclude unwanted windows
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
    return false;

  // Exclude special items
  if (StringUtils::StartsWithNoCase(item.GetPath(), "newsmartplaylist://") ||
      StringUtils::StartsWithNoCase(item.GetPath(), "newplaylist://") ||
      StringUtils::StartsWithNoCase(item.GetPath(), "newtag://"))
    return false;

  // Include playlists located at one of the possible video/mixed playlist locations
  if (item.IsPlayList())
  {
    if (StringUtils::StartsWithNoCase(item.GetPath(), "special://videoplaylists/") ||
        StringUtils::StartsWithNoCase(item.GetPath(), "special://profile/playlists/video/") ||
        StringUtils::StartsWithNoCase(item.GetPath(), "special://profile/playlists/mixed/"))
      return true;

    // Has user changed default playlists location and the list is located there?
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    std::string path = settings->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
    StringUtils::TrimRight(path, "/");
    if (StringUtils::StartsWith(item.GetPath(), StringUtils::Format("{}/video/", path)) ||
        StringUtils::StartsWith(item.GetPath(), StringUtils::Format("{}/mixed/", path)))
      return true;

    // Unknown location. Type cannot be determined.
    return false;
  }

  if (item.m_bIsFolder &&
      (item.IsVideoDb() || StringUtils::StartsWithNoCase(item.GetPath(), "library://video/")))
  {
    // Exclude top level nodes - eg can't play 'genres' just a specific genre etc
    const XFILE::VIDEODATABASEDIRECTORY::NODE_TYPE node =
        XFILE::CVideoDatabaseDirectory::GetDirectoryParentType(item.GetPath());
    if (node == XFILE::VIDEODATABASEDIRECTORY::NODE_TYPE_OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NODE_TYPE_MOVIES_OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NODE_TYPE_TVSHOWS_OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NODE_TYPE_MUSICVIDEOS_OVERVIEW)
      return false;

    return true;
  }

  if (item.HasVideoInfoTag() && item.CanQueue())
  {
    return true;
  }
  else if ((!item.m_bIsFolder && item.IsVideo()) || item.IsDVD() || item.IsCDDA())
  {
    return true;
  }
  else if (!item.m_bIsShareOrDrive && item.m_bIsFolder)
  {
    // Not a video-specific folder (like file:// or nfs://). Allow play if context is Video window.
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_NAV)
      return true;
  }

  return false;
}

} // namespace VIDEO_UTILS
