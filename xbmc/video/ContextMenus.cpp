/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "Autorun.h"
#include "ContextMenuManager.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicFileItemClassify.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/ExecString.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"
#include "video/guilib/VideoVersionHelper.h"

#include <utility>

using namespace KODI;

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
  return !item.m_bIsFolder && VIDEO::UTILS::GetItemResumeInformation(item).isResumable;
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
      return VIDEO::IsVideoDb(item);
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
      return VIDEO::IsVideoDb(item);
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
  return ((item.m_bIsFolder || item.IsFileFolder(EFILEFOLDER_MASK_ONBROWSE)) &&
          VIDEO::UTILS::IsItemPlayable(item));
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

  // For file directory browsing, we need item's dyn path, for everything else the path.
  const std::string path{item->IsFileFolder(EFILEFOLDER_MASK_ONBROWSE) ? item->GetDynPath()
                                                                       : item->GetPath()};

  if (target == windowMgr.GetActiveWindow())
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, target, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(path);
    windowMgr.SendMessage(msg);
  }
  else
  {
    windowMgr.ActivateWindow(target, {path, "return"});
  }
  return true;
}

namespace
{
bool ExecuteAction(const CExecString& execute)
{
  const std::string execStr{execute.GetExecString()};
  if (!execStr.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execStr);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}

class CVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoSelectActionProcessorBase(item)
  {
  }

protected:
  bool OnPlayPartSelected(unsigned int part) override
  {
    // part numbers are 1-based
    ExecuteAction({"PlayMedia", *m_item, StringUtils::Format("playoffset={}", part - 1)});
    return true;
  }

  bool OnResumeSelected() override
  {
    ExecuteAction({"PlayMedia", *m_item, "resume"});
    return true;
  }

  bool OnPlaySelected() override
  {
    ExecuteAction({"PlayMedia", *m_item, "noresume"});
    return true;
  }

  bool OnQueueSelected() override
  {
    ExecuteAction({"QueueMedia", *m_item, ""});
    return true;
  }

  bool OnInfoSelected() override
  {
    CGUIDialogVideoInfo::ShowFor(*m_item);
    return true;
  }

  bool OnMoreSelected() override
  {
    CONTEXTMENU::ShowFor(m_item, CContextMenuManager::MAIN);
    return true;
  }
};
} // unnamed namespace

bool CVideoChooseVersion::IsVisible(const CFileItem& item) const
{
  return item.HasVideoVersions() &&
         !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
         !VIDEO::IsVideoAssetFile(item);
}

bool CVideoChooseVersion::Execute(const std::shared_ptr<CFileItem>& item) const
{
  // force selection dialog, regardless of any settings like 'Select default video version'
  item->SetProperty("needs_resolved_video_asset", true);
  item->SetProperty("video_asset_type", static_cast<int>(VideoAssetType::VERSION));
  CVideoSelectActionProcessor proc{item};
  const bool ret = proc.ProcessDefaultAction();
  item->ClearProperty("needs_resolved_video_asset");
  item->ClearProperty("video_asset_type");
  return ret;
}

std::string CVideoResume::GetLabel(const CFileItem& item) const
{
  return VIDEO::UTILS::GetResumeString(item.GetItemToPlay());
}

bool CVideoResume::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return VIDEO::UTILS::GetItemResumeInformation(item).isResumable;
}

namespace
{
std::vector<std::string> GetPlayers(const CPlayerCoreFactory& playerCoreFactory,
                                    const CFileItem& item)
{
  std::vector<std::string> players;
  if (VIDEO::IsVideoDb(item))
  {
    //! @todo CPlayerCoreFactory and classes called from there do not handle dyn path correctly.
    CFileItem item2{item};
    item2.SetPath(item.GetDynPath());
    playerCoreFactory.GetPlayers(item2, players);
  }
  else
    playerCoreFactory.GetPlayers(item, players);

  return players;
}

class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item, bool choosePlayer)
    : CVideoPlayActionProcessorBase(item), m_choosePlayer(choosePlayer)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    Play();
    return true;
  }

  bool OnPlaySelected() override
  {
    std::string player;
    if (m_choosePlayer)
    {
      const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
      const std::vector<std::string> players{GetPlayers(playerCoreFactory, *m_item)};
      player = playerCoreFactory.SelectPlayerDialog(players);
      if (player.empty())
      {
        m_userCancelled = true;
        return true; // User cancelled player selection. We're done.
      }
    }

    Play(player);
    return true;
  }

private:
  void Play(const std::string& player = "")
  {
    m_item->SetProperty("playlist_type_hint", PLAYLIST::TYPE_VIDEO);
    const ContentUtils::PlayMode mode{m_item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                          ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                          : ContentUtils::PlayMode::PLAY_ONLY_THIS};
    VIDEO::UTILS::PlayItem(m_item, player, mode);
  }

  const bool m_choosePlayer{false};
};

enum class PlayMode
{
  PLAY,
  PLAY_USING,
  PLAY_VERSION_USING,
  RESUME,
};
void SetPathAndPlay(const std::shared_ptr<CFileItem>& item, PlayMode mode)
{
  item->SetProperty("check_resume", false);

  if (item->IsLiveTV()) // pvr tv or pvr radio?
  {
    g_application.PlayMedia(*item, "", PLAYLIST::TYPE_VIDEO);
  }
  else
  {
    const auto itemCopy{std::make_shared<CFileItem>(*item)};
    if (VIDEO::IsVideoDb(*itemCopy))
    {
      if (!itemCopy->m_bIsFolder)
      {
        itemCopy->SetProperty("original_listitem_url", item->GetPath());
        itemCopy->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
      }
      else if (itemCopy->HasVideoInfoTag() && itemCopy->GetVideoInfoTag()->IsDefaultVideoVersion())
      {
        //! @todo get rid of "videos with versions as folder" hack!
        itemCopy->m_bIsFolder = false;
      }
    }

    if (mode == PlayMode::PLAY_VERSION_USING)
    {
      // force video version selection dialog
      itemCopy->SetProperty("needs_resolved_video_asset", true);
    }
    else
    {
      // play the given/default video version, if multiple versions are available
      itemCopy->SetProperty("has_resolved_video_asset", true);
    }

    const bool choosePlayer{mode == PlayMode::PLAY_USING || mode == PlayMode::PLAY_VERSION_USING};
    CVideoPlayActionProcessor proc{itemCopy, choosePlayer};
    if (mode == PlayMode::RESUME && (itemCopy->GetStartOffset() == STARTOFFSET_RESUME ||
                                     VIDEO::UTILS::GetItemResumeInformation(*item).isResumable))
      proc.ProcessAction(VIDEO::GUILIB::ACTION_RESUME);
    else // all other modes are actually PLAY
      proc.ProcessAction(VIDEO::GUILIB::ACTION_PLAY_FROM_BEGINNING);
  }
}
} // unnamed namespace

bool CVideoResume::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  const auto item{std::make_shared<CFileItem>(itemIn->GetItemToPlay())};
#ifdef HAS_OPTICAL_DRIVE
  if (item->IsDVD() || MUSIC::IsCDDA(*item))
    return MEDIA_DETECT::CAutorun::PlayDisc(item->GetPath(), true, false);
#endif

  item->SetStartOffset(STARTOFFSET_RESUME);
  SetPathAndPlay(item, PlayMode::RESUME);
  return true;
};

std::string CVideoPlay::GetLabel(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsLiveTV())
    return g_localizeStrings.Get(19000); // Switch to channel
  if (VIDEO::UTILS::GetItemResumeInformation(item).isResumable)
    return g_localizeStrings.Get(12021); // Play from beginning
  return g_localizeStrings.Get(208); // Play
}

bool CVideoPlay::IsVisible(const CFileItem& item) const
{
  return VIDEO::UTILS::IsItemPlayable(item);
}

bool CVideoPlay::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  const auto item{std::make_shared<CFileItem>(itemIn->GetItemToPlay())};
#ifdef HAS_OPTICAL_DRIVE
  if (item->IsDVD() || MUSIC::IsCDDA(*item))
    return MEDIA_DETECT::CAutorun::PlayDisc(item->GetPath(), true, true);
#endif
  SetPathAndPlay(item, PlayMode::PLAY);
  return true;
};

bool CVideoPlayUsing::IsVisible(const CFileItem& item) const
{
  if (item.HasVideoVersions() &&
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
      !VIDEO::IsVideoAssetFile(item))
    return false;

  if (item.IsLiveTV())
    return false;

  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  return (GetPlayers(playerCoreFactory, item).size() > 1) && VIDEO::UTILS::IsItemPlayable(item);
}

bool CVideoPlayUsing::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  const auto item{std::make_shared<CFileItem>(itemIn->GetItemToPlay())};
  SetPathAndPlay(item, PlayMode::PLAY_USING);
  return true;
}

bool CVideoPlayVersionUsing::IsVisible(const CFileItem& item) const
{
  return item.HasVideoVersions() &&
         !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
         !VIDEO::IsVideoAssetFile(item);
}

bool CVideoPlayVersionUsing::Execute(const std::shared_ptr<CFileItem>& itemIn) const
{
  const auto item{std::make_shared<CFileItem>(itemIn->GetItemToPlay())};
  item->SetProperty("video_asset_type", static_cast<int>(VideoAssetType::VERSION));
  SetPathAndPlay(item, PlayMode::PLAY_VERSION_USING);
  return true;
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

  return VIDEO::UTILS::IsItemPlayable(item);
}

bool CVideoQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  VIDEO::UTILS::QueueItem(item, VIDEO::UTILS::QueuePosition::POSITION_END);

  // Set selection to next item in active window's view.
  const int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  SelectNextItem(windowID);

  return true;
};

bool CVideoPlayNext::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return VIDEO::UTILS::IsItemPlayable(item);
}

bool CVideoPlayNext::Execute(const std::shared_ptr<CFileItem>& item) const
{
  VIDEO::UTILS::QueueItem(item, VIDEO::UTILS::QueuePosition::POSITION_BEGIN);
  return true;
};

std::string CVideoPlayAndQueue::GetLabel(const CFileItem& item) const
{
  if (VIDEO::UTILS::IsAutoPlayNextItem(item))
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
    const ContentUtils::PlayMode mode = VIDEO::UTILS::IsAutoPlayNextItem(*item)
                                            ? ContentUtils::PlayMode::PLAY_ONLY_THIS
                                            : ContentUtils::PlayMode::PLAY_FROM_HERE;
    VIDEO::UTILS::PlayItem(item, "", mode);
    return true;
  }

  return true; //! @todo implement
};

}
