/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "Autorun.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "playlists/PlayList.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "view/GUIViewState.h"

#include <utility>

namespace CONTEXTMENU
{

CVideoInfo::CVideoInfo(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(std::move(mediaType))
{
}

bool CVideoInfo::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;

  if (item.IsPVRRecording())
    return false; // pvr recordings have its own implementation for this

  return item.GetVideoInfoTag()->m_type == m_mediaType;
}

bool CVideoInfo::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CGUIDialogVideoInfo::ShowFor(*item);
  return true;
}

bool CVideoRemoveResumePoint::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  // Folders don't have a resume point
  return !item.m_bIsFolder && VIDEO_UTILS::GetItemResumeInformation(item).isResumable;
}

bool CVideoRemoveResumePoint::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().ResetResumePoint(item);
  return true;
}

bool CVideoMarkWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder && item.IsPlugin()) // we cannot manage plugin folder's watched state
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return !item.IsParentFolder() && URIUtils::IsPVRRecordingFileOrFolder(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() == 0;
}

bool CVideoMarkWatched::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, true);
  return true;
}

bool CVideoMarkUnWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder && item.IsPlugin()) // we cannot manage plugin folder's watched state
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return !item.IsParentFolder() && URIUtils::IsPVRRecordingFileOrFolder(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() > 0;
}

bool CVideoMarkUnWatched::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, false);
  return true;
}

bool CVideoBrowse::IsVisible(const CFileItem& item) const
{
  if (item.IsFileFolder(EFILEFOLDER_MASK_ONBROWSE))
    return false; // handled by CMediaWindow

  return item.m_bIsFolder && VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoBrowse::Execute(const std::shared_ptr<CFileItem>& item) const
{
  int target = WINDOW_INVALID;
  if (URIUtils::IsPVRRadioRecordingFileOrFolder(item->GetPath()))
    target = WINDOW_RADIO_RECORDINGS;
  else if (URIUtils::IsPVRTVRecordingFileOrFolder(item->GetPath()))
    target = WINDOW_TV_RECORDINGS;
  else
    target = WINDOW_VIDEO_NAV;

  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();

  if (target == windowMgr.GetActiveWindow())
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, target, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(item->GetPath());
    windowMgr.SendMessage(msg);
  }
  else
  {
    windowMgr.ActivateWindow(target, {item->GetPath(), "return"});
  }
  return true;
}

std::string CVideoResume::GetLabel(const CFileItem& item) const
{
  return CGUIWindowVideoBase::GetResumeString(item.GetItemToPlay());
}

bool CVideoResume::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return VIDEO_UTILS::GetItemResumeInformation(item).isResumable;
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
      std::unique_ptr<CGUIViewState> viewState(CGUIViewState::GetViewState(windowId, queuedItems));
      if (viewState)
        queuedItems.Sort(viewState->GetSortMethod());
    }
  }
}

void PlayAndQueueRecordings(const std::shared_ptr<CFileItem>& item, int windowId)
{
  const std::shared_ptr<CFileItem> parentFolderItem =
      std::make_shared<CFileItem>(URIUtils::GetParentPath(item->GetPath()), true);

  // add all items of given item's directory to a temporary playlist, start playback of given item
  CFileItemList queuedItems;
  AddRecordingsToPlayListAndSort(parentFolderItem, queuedItems);

  PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();

  player.ClearPlaylist(PLAYLIST::TYPE_VIDEO);
  player.Reset();
  player.Add(PLAYLIST::TYPE_VIDEO, queuedItems);

  // figure out where to start playback
  PLAYLIST::CPlayList& playList = player.GetPlaylist(PLAYLIST::TYPE_VIDEO);
  int itemToPlay = 0;

  for (int i = 0; i < queuedItems.Size(); ++i)
  {
    if (item->IsSamePath(queuedItems.Get(i).get()))
    {
      itemToPlay = i;
      break;
    }
  }

  if (player.IsShuffled(PLAYLIST::TYPE_VIDEO))
  {
    playList.Swap(0, playList.FindOrder(itemToPlay));
    itemToPlay = 0;
  }

  player.SetCurrentPlaylist(PLAYLIST::TYPE_VIDEO);
  player.Play(itemToPlay, "");
}

void SetPathAndPlay(CFileItem& item)
{
  if (!item.m_bIsFolder && item.IsVideoDb())
  {
    item.SetProperty("original_listitem_url", item.GetPath());
    item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  item.SetProperty("check_resume", false);

  if (item.IsLiveTV()) // pvr tv or pvr radio?
  {
    g_application.PlayMedia(item, "", PLAYLIST::TYPE_VIDEO);
  }
  else
  {
    item.SetProperty("playlist_type_hint", PLAYLIST::TYPE_VIDEO);
    VIDEO_UTILS::PlayItem(std::make_shared<CFileItem>(item));
  }
}

} // unnamed namespace

bool CVideoResume::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, false);
#endif

  item.SetStartOffset(STARTOFFSET_RESUME);
  SetPathAndPlay(item);
  return true;
};

std::string CVideoPlay::GetLabel(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsLiveTV())
    return g_localizeStrings.Get(19000); // Switch to channel
  if (VIDEO_UTILS::GetItemResumeInformation(item).isResumable)
    return g_localizeStrings.Get(12021); // Play from beginning
  return g_localizeStrings.Get(208); // Play
}

bool CVideoPlay::IsVisible(const CFileItem& item) const
{
  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlay::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, true);
#endif
  SetPathAndPlay(item);
  return true;
};

namespace
{
void SelectNextItem(int windowID)
{
  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  CGUIWindow* window = windowMgr.GetWindow(windowID);
  if (window)
  {
    const int viewContainerID = window->GetViewContainerID();
    if (viewContainerID > 0)
    {
      CGUIMessage msg1(GUI_MSG_ITEM_SELECTED, windowID, viewContainerID);
      windowMgr.SendMessage(msg1, windowID);

      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, windowID, viewContainerID, msg1.GetParam1() + 1);
      windowMgr.SendMessage(msg2, windowID);
    }
  }
}
} // unnamed namespace

bool CVideoQueue::IsVisible(const CFileItem& item) const
{
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  if (!item.CanQueue())
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowID == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::QueuePosition::POSITION_END);

  // Set selection to next item in active window's view.
  SelectNextItem(windowID);

  return true;
};

bool CVideoPlayNext::IsVisible(const CFileItem& item) const
{
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  if (!item.CanQueue())
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlayNext::Execute(const std::shared_ptr<CFileItem>& item) const
{
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::QueuePosition::POSITION_BEGIN);
  return true;
};

bool CVideoPlayAndQueue::IsVisible(const CFileItem& item) const
{
  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowId == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  if ((windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS) &&
      item.IsUsablePVRRecording())
    return true;

  return false; //! @todo implement
}

bool CVideoPlayAndQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowId == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  if ((windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS) &&
      item->IsUsablePVRRecording())
  {
    // recursively add items located in the same folder as item to play list, starting with item
    PlayAndQueueRecordings(item, windowId);
    return true;
  }

  return true; //! @todo implement
};

}
