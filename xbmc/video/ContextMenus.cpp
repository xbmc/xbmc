/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "Application.h"
#include "Autorun.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/MediaSettings.h"
#include "utils/URIUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "view/GUIViewState.h"

namespace CONTEXTMENU
{

CVideoInfo::CVideoInfo(MediaType mediaType)
    : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CVideoInfo::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;

  if (item.IsPVRRecording())
    return false; // pvr recordings have its own implementation for this

  return item.GetVideoInfoTag()->m_type == m_mediaType;
}

bool CVideoInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogVideoInfo::ShowFor(*item);
  return true;
}

bool CRemoveResumePoint::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return CGUIWindowVideoBase::HasResumeItemOffset(&item);
}

bool CRemoveResumePoint::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().ResetResumePoint(item);
  return true;
}

bool CMarkWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return URIUtils::IsPVRRecordingFileOrFolder(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() == 0;
}

bool CMarkWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, true);
  return true;
}

bool CMarkUnWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return URIUtils::IsPVRRecordingFileOrFolder(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() > 0;
}

bool CMarkUnWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, false);
  return true;
}

std::string CResume::GetLabel(const CFileItem& item) const
{
  return CGUIWindowVideoBase::GetResumeString(item.GetItemToPlay());
}

bool CResume::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return CGUIWindowVideoBase::HasResumeItemOffset(&item);
}

namespace
{

void AddRecordingsToPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
{
  if (item->m_bIsFolder)
  {
    CFileItemList items;
    XFILE::CDirectory::GetDirectory(item->GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);

    const int watchedMode = CMediaSettings::GetInstance().GetWatchedMode("recordings");
    const bool unwatchedOnly = watchedMode == WatchedModeUnwatched;
    const bool watchedOnly = watchedMode == WatchedModeWatched;
    for (const auto& currItem : items)
    {
      if (currItem->HasVideoInfoTag() &&
          ((unwatchedOnly && currItem->GetVideoInfoTag()->GetPlayCount() > 0) ||
           (watchedOnly && currItem->GetVideoInfoTag()->GetPlayCount() <= 0)))
        continue;

      AddRecordingsToPlayList(currItem, queuedItems);
    }
  }
  else
  {
    queuedItems.Add(item);
  }
}

void AddRecordingsToPlayListAndSort(const std::shared_ptr<CFileItem>& item,
                                    CFileItemList& queuedItems)
{
  queuedItems.SetPath(item->GetPath());
  AddRecordingsToPlayList(item, queuedItems);

  if (!queuedItems.IsEmpty())
  {
    const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    if (windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS)
    {
      const CGUIViewState* viewState = CGUIViewState::GetViewState(windowId, queuedItems);
      if (viewState)
        queuedItems.Sort(viewState->GetSortMethod());
    }
  }
}

void QueueRecordings(const std::shared_ptr<CFileItem>& item, bool bPlayNext)
{
  CFileItemList queuedItems;
  AddRecordingsToPlayListAndSort(item, queuedItems);

  PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();

  // Determine the proper list to queue this element
  int playlist = player.GetCurrentPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = g_application.GetAppPlayer().GetPreferredPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = PLAYLIST_VIDEO;

  if (bPlayNext && g_application.GetAppPlayer().IsPlaying())
    player.Insert(playlist, queuedItems, player.GetCurrentSong() + 1);
  else
    player.Add(playlist, queuedItems);

  player.SetCurrentPlaylist(playlist);
}

void SetPathAndPlay(CFileItem& item)
{
  if (item.IsVideoDb())
  {
    item.SetProperty("original_listitem_url", item.GetPath());
    item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  item.SetProperty("check_resume", false);

  if (item.IsLiveTV()) // pvr tv or pvr radio?
  {
    g_application.PlayMedia(item, "", PLAYLIST_NONE);
  }
  else if (item.m_bIsFolder && StringUtils::StartsWith(item.GetPath(), "pvr://recordings/"))
  {
    // recursively add items to play list
    CFileItemList queuedItems;
    AddRecordingsToPlayListAndSort(std::make_shared<CFileItem>(item), queuedItems);

    PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();

    player.ClearPlaylist(PLAYLIST_VIDEO);
    player.Reset();
    player.Add(PLAYLIST_VIDEO, queuedItems);
    player.SetCurrentPlaylist(PLAYLIST_VIDEO);

    player.Play();
  }
  else
  {
    CServiceBroker::GetPlaylistPlayer().Play(std::make_shared<CFileItem>(item), "");
  }
}

} // unnamed namespace

bool CResume::Execute(const CFileItemPtr& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, false);
#endif

  item.m_lStartOffset = STARTOFFSET_RESUME;
  SetPathAndPlay(item);
  return true;
};

std::string CPlay::GetLabel(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsLiveTV())
    return g_localizeStrings.Get(19000); // Switch to channel
  if (CGUIWindowVideoBase::HasResumeItemOffset(&item))
    return g_localizeStrings.Get(12021); // Play from beginning
  return g_localizeStrings.Get(208); // Play
}

bool CPlay::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder)
  {
    if (StringUtils::StartsWith(item.GetPath(), "pvr://recordings/"))
    {
      // Note: Recordings contained in the folder must be sorted properly, thus this
      //       item is only available if one of the recordings windows is active.
      const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
      return windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS;
    }
    else
      return false; //! @todo implement
  }

  return item.IsVideo() || item.IsLiveTV() || item.IsDVD() || item.IsCDDA();
}

bool CPlay::Execute(const CFileItemPtr& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, true);
#endif
  SetPathAndPlay(item);
  return true;
};

bool CQueue::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder && StringUtils::StartsWith(item.GetPath(), "pvr://recordings/"))
    return true;

  return false; //! @todo implement
}

bool CQueue::Execute(const CFileItemPtr& item) const
{
  if (item->m_bIsFolder && StringUtils::StartsWith(item->GetPath(), "pvr://recordings/"))
  {
    // recursively add items to play list
    QueueRecordings(item, false);
    return true;
  }

  return true; //! @todo implement
};

bool CPlayNext::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder && StringUtils::StartsWith(item.GetPath(), "pvr://recordings/"))
    return true;

  return false; //! @todo implement
}

bool CPlayNext::Execute(const CFileItemPtr& item) const
{
  if (item->m_bIsFolder && StringUtils::StartsWith(item->GetPath(), "pvr://recordings/"))
  {
    // recursively add items to play list
    QueueRecordings(item, true);
    return true;
  }

  return true; //! @todo implement
};

}

