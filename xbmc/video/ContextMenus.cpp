/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "Autorun.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"

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
  return VIDEO_UTILS::GetResumeString(item.GetItemToPlay());
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
void SetPathAndPlay(CFileItem& item, const std::string& player)
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

    const ContentUtils::PlayMode mode = item.GetProperty("CheckAutoPlayNextItem").asBoolean()
                                            ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                            : ContentUtils::PlayMode::PLAY_ONLY_THIS;
    VIDEO_UTILS::PlayItem(std::make_shared<CFileItem>(item), player, mode);
  }
}

std::vector<std::string> GetPlayers(const CPlayerCoreFactory& playerCoreFactory,
                                    const CFileItem& item)
{
  std::vector<std::string> players;
  if (item.IsVideoDb())
  {
    const CFileItem item2{item.GetVideoInfoTag()->m_strFileNameAndPath, false};
    playerCoreFactory.GetPlayers(item2, players);
  }
  else
    playerCoreFactory.GetPlayers(item, players);

  return players;
}
} // unnamed namespace

bool CVideoResume::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_OPTICAL_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, false);
#endif

  item.SetStartOffset(STARTOFFSET_RESUME);
  SetPathAndPlay(item, "");
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
#ifdef HAS_OPTICAL_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, true);
#endif
  SetPathAndPlay(item, "");
  return true;
};

bool CVideoPlayUsing::IsVisible(const CFileItem& item) const
{
  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  return (GetPlayers(playerCoreFactory, item).size() > 1) && VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlayUsing::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  CFileItem item{itemIn->GetItemToPlay()};

  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  const std::vector<std::string> players{GetPlayers(playerCoreFactory, item)};
  const std::string player{playerCoreFactory.SelectPlayerDialog(players)};
  if (!player.empty())
  {
    SetPathAndPlay(item, player);
    return true;
  }
  return false;
}

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

bool CanQueue(const CFileItem& item)
{
  if (!item.CanQueue())
    return false;

  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowId == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  return true;
}
} // unnamed namespace

bool CVideoQueue::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::QueuePosition::POSITION_END);

  // Set selection to next item in active window's view.
  const int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  SelectNextItem(windowID);

  return true;
};

bool CVideoPlayNext::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlayNext::Execute(const std::shared_ptr<CFileItem>& item) const
{
  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::QueuePosition::POSITION_BEGIN);
  return true;
};

std::string CVideoPlayAndQueue::GetLabel(const CFileItem& item) const
{
  if (VIDEO_UTILS::IsAutoPlayNextItem(item))
    return g_localizeStrings.Get(13434); // Play only this
  else
    return g_localizeStrings.Get(13412); // Play from here
}

bool CVideoPlayAndQueue::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if ((windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS) &&
      item.IsUsablePVRRecording())
    return true;

  return false; //! @todo implement
}

bool CVideoPlayAndQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if ((windowId == WINDOW_TV_RECORDINGS || windowId == WINDOW_RADIO_RECORDINGS) &&
      item->IsUsablePVRRecording())
  {
    const ContentUtils::PlayMode mode = VIDEO_UTILS::IsAutoPlayNextItem(*item)
                                            ? ContentUtils::PlayMode::PLAY_ONLY_THIS
                                            : ContentUtils::PlayMode::PLAY_FROM_HERE;
    VIDEO_UTILS::PlayItem(item, "", mode);
    return true;
  }

  return true; //! @todo implement
};

}
