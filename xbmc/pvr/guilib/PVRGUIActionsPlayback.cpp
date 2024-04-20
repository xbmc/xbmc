/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsPlayback.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/ApplicationEnums.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/PVRStreamProperties.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoSelectActionProcessor.h"

#include <memory>
#include <string>
#include <vector>

using namespace KODI;
using namespace PVR;
using namespace KODI::MESSAGING;

CPVRGUIActionsPlayback::CPVRGUIActionsPlayback()
  : m_settings({CSettings::SETTING_LOOKANDFEEL_STARTUPACTION,
                CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES})
{
}

bool CPVRGUIActionsPlayback::CheckResumeRecording(const CFileItem& item) const
{
  bool bPlayIt(true);

  const VIDEO::GUILIB::Action action =
      VIDEO::GUILIB::CVideoSelectActionProcessorBase::ChoosePlayOrResume(item);
  if (action == VIDEO::GUILIB::ACTION_RESUME)
  {
    const_cast<CFileItem*>(&item)->SetStartOffset(STARTOFFSET_RESUME);
  }
  else if (action == VIDEO::GUILIB::ACTION_PLAY_FROM_BEGINNING)
  {
    const_cast<CFileItem*>(&item)->SetStartOffset(0);
  }
  else
  {
    // The Resume dialog was closed without any choice
    bPlayIt = false;
  }

  return bPlayIt;
}

bool CPVRGUIActionsPlayback::ResumePlayRecording(const CFileItem& item, bool bFallbackToPlay) const
{
  if (VIDEO::UTILS::GetItemResumeInformation(item).isResumable)
  {
    const_cast<CFileItem*>(&item)->SetStartOffset(STARTOFFSET_RESUME);
  }
  else
  {
    if (bFallbackToPlay)
      const_cast<CFileItem*>(&item)->SetStartOffset(0);
    else
      return false;
  }

  return PlayRecording(item, false /* skip resume check */);
}

void CPVRGUIActionsPlayback::CheckAndSwitchToFullscreen(bool bFullscreen) const
{
  CMediaSettings::GetInstance().SetMediaStartWindowed(!bFullscreen);

  if (bFullscreen)
  {
    CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                    CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
}

bool CPVRGUIActionsPlayback::PlayRecording(const CFileItem& item, bool bCheckResume) const
{
  const std::shared_ptr<CPVRRecording> recording(CPVRItem(item).GetRecording());
  if (!recording)
    return false;

  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording))
  {
    CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                    CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    return true;
  }

  if (!bCheckResume || CheckResumeRecording(item))
  {
    if (!item.m_bIsFolder && VIDEO::UTILS::IsAutoPlayNextItem(item))
    {
      // recursively add items located in the same folder as item to play list, starting with item
      std::string parentPath = item.GetProperty("ParentPath").asString();
      if (parentPath.empty())
        URIUtils::GetParentPath(item.GetPath(), parentPath);

      if (parentPath.empty())
      {
        CLog::LogF(LOGERROR, "Unable to obtain parent path for '{}'", item.GetPath());
        return false;
      }

      const auto parentItem = std::make_shared<CFileItem>(parentPath, true);
      if (item.GetStartOffset() == STARTOFFSET_RESUME)
        parentItem->SetStartOffset(STARTOFFSET_RESUME);

      auto queuedItems = std::make_unique<CFileItemList>();
      VIDEO::UTILS::GetItemsForPlayList(parentItem, *queuedItems);

      // figure out where to start playback
      int pos = 0;
      for (const std::shared_ptr<CFileItem>& queuedItem : *queuedItems)
      {
        if (queuedItem->IsSamePath(&item))
          break;

        pos++;
      }

      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, pos, -1,
                                                 static_cast<void*>(queuedItems.release()));
    }
    else
    {
      CFileItem* itemToPlay = new CFileItem(recording);
      itemToPlay->SetStartOffset(item.GetStartOffset());
      CServiceBroker::GetPVRManager().PlaybackState()->StartPlayback(itemToPlay);
    }
    CheckAndSwitchToFullscreen(true);
  }
  return true;
}

bool CPVRGUIActionsPlayback::PlayRecordingFolder(const CFileItem& item, bool bCheckResume) const
{
  if (!item.m_bIsFolder)
    return false;

  if (!bCheckResume || CheckResumeRecording(item))
  {
    // recursively add items to list
    const auto itemToQueue = std::make_shared<CFileItem>(item);
    auto queuedItems = std::make_unique<CFileItemList>();
    VIDEO::UTILS::GetItemsForPlayList(itemToQueue, *queuedItems);

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, -1,
                                               static_cast<void*>(queuedItems.release()));
    CheckAndSwitchToFullscreen(true);
  }
  return true;
}

bool CPVRGUIActionsPlayback::PlayEpgTag(
    const CFileItem& item,
    ContentUtils::PlayMode mode /* = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM */) const
{
  const std::shared_ptr<CPVREpgInfoTag> epgTag(CPVRItem(item).GetEpgInfoTag());
  if (!epgTag)
    return false;

  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingEpgTag(epgTag))
  {
    CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                    CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    return true;
  }

  // Obtain dynamic playback url and properties from the respective pvr client
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(epgTag->ClientID());
  if (!client)
    return false;

  CPVRStreamProperties props;
  client->GetEpgTagStreamProperties(epgTag, props);

  CFileItem* itemToPlay = nullptr;
  if (props.EPGPlaybackAsLive())
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(item);
    if (!groupMember)
      return false;

    itemToPlay = new CFileItem(groupMember);
  }
  else
  {
    itemToPlay = new CFileItem(epgTag);
  }

  CServiceBroker::GetPVRManager().PlaybackState()->StartPlayback(itemToPlay, mode);
  CheckAndSwitchToFullscreen(true);
  return true;
}

bool CPVRGUIActionsPlayback::SwitchToChannel(const CFileItem& item, bool bCheckResume) const
{
  if (item.m_bIsFolder)
    return false;

  std::shared_ptr<CPVRRecording> recording;
  const std::shared_ptr<const CPVRChannel> channel(CPVRItem(item).GetChannel());
  if (channel)
  {
    bool bSwitchToFullscreen =
        CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingChannel(channel);

    if (!bSwitchToFullscreen)
    {
      recording =
          CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());
      bSwitchToFullscreen =
          recording &&
          CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording);
    }

    if (bSwitchToFullscreen)
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      return true;
    }
  }

  ParentalCheckResult result =
      channel ? CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalLock(channel)
              : ParentalCheckResult::FAILED;
  if (result == ParentalCheckResult::SUCCESS)
  {
    // switch to channel or if recording present, ask whether to switch or play recording...
    if (!recording)
      recording =
          CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());

    if (recording)
    {
      bool bCancel(false);
      bool bPlayRecording = CGUIDialogYesNo::ShowAndGetInput(
          CVariant{19687}, // "Play recording"
          CVariant{""}, CVariant{12021}, // "Play from beginning"
          CVariant{recording->m_strTitle}, bCancel, CVariant{19000}, // "Switch to channel"
          CVariant{19687}, // "Play recording"
          0); // no autoclose
      if (bCancel)
        return false;

      if (bPlayRecording)
        return PlayRecording(CFileItem(recording), bCheckResume);
    }

    bool bFullscreen;
    switch (m_settings.GetIntValue(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES))
    {
      case 0: // never
        bFullscreen = false;
        break;
      case 1: // TV channels
        bFullscreen = !channel->IsRadio();
        break;
      case 2: // Radio channels
        bFullscreen = channel->IsRadio();
        break;
      case 3: // TV and radio channels
      default:
        bFullscreen = true;
        break;
    }
    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(item);
    if (!groupMember)
      return false;

    CServiceBroker::GetPVRManager().PlaybackState()->StartPlayback(new CFileItem(groupMember));
    CheckAndSwitchToFullscreen(bFullscreen);
    return true;
  }
  else if (result == ParentalCheckResult::FAILED)
  {
    const std::string channelName =
        channel ? channel->ChannelName() : g_localizeStrings.Get(19029); // Channel
    const std::string msg = StringUtils::Format(
        g_localizeStrings.Get(19035),
        channelName); // CHANNELNAME could not be played. Check the log for details.

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(19166),
                                          msg); // PVR information
  }

  return false;
}

bool CPVRGUIActionsPlayback::SwitchToChannel(PlaybackType type) const
{
  std::shared_ptr<CPVRChannelGroupMember> groupMember;
  bool bIsRadio(false);

  // check if the desired PlaybackType is already playing,
  // and if not, try to grab the last played channel of this type
  switch (type)
  {
    case PlaybackTypeRadio:
    {
      if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
        return true;

      const std::shared_ptr<CPVRChannelGroup> allGroup =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio();
      if (allGroup)
        groupMember = allGroup->GetLastPlayedChannelGroupMember();

      bIsRadio = true;
      break;
    }
    case PlaybackTypeTV:
    {
      if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV())
        return true;

      const std::shared_ptr<const CPVRChannelGroup> allGroup =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
      if (allGroup)
        groupMember = allGroup->GetLastPlayedChannelGroupMember();

      break;
    }
    default:
      if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
        return true;

      groupMember =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetLastPlayedChannelGroupMember();
      break;
  }

  // if we have a last played channel, start playback
  if (groupMember)
  {
    return SwitchToChannel(CFileItem(groupMember), true);
  }
  else
  {
    // if we don't, find the active channel group of the demanded type and play it's first channel
    const std::shared_ptr<const CPVRChannelGroup> channelGroup =
        CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bIsRadio);
    if (channelGroup)
    {
      // try to start playback of first channel in this group
      const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
          channelGroup->GetMembers();
      if (!groupMembers.empty())
      {
        return SwitchToChannel(CFileItem(*groupMembers.begin()), true);
      }
    }
  }

  CLog::LogF(LOGERROR,
             "Could not determine {} channel to playback. No last played channel found, and "
             "first channel of active group could also not be determined.",
             bIsRadio ? "Radio" : "TV");

  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Error,
      g_localizeStrings.Get(19166), // PVR information
      StringUtils::Format(
          g_localizeStrings.Get(19035),
          g_localizeStrings.Get(
              bIsRadio ? 19021
                       : 19020))); // Radio/TV could not be played. Check the log for details.
  return false;
}

bool CPVRGUIActionsPlayback::PlayChannelOnStartup() const
{
  int iAction = m_settings.GetIntValue(CSettings::SETTING_LOOKANDFEEL_STARTUPACTION);
  if (iAction != STARTUP_ACTION_PLAY_TV && iAction != STARTUP_ACTION_PLAY_RADIO)
    return false;

  bool playRadio = (iAction == STARTUP_ACTION_PLAY_RADIO);

  // get the last played channel or fallback to first channel of all channels group
  std::shared_ptr<CPVRChannelGroupMember> groupMember =
      CServiceBroker::GetPVRManager().PlaybackState()->GetLastPlayedChannelGroupMember(playRadio);

  if (!groupMember)
  {
    const std::shared_ptr<const CPVRChannelGroup> group =
        CServiceBroker::GetPVRManager().ChannelGroups()->Get(playRadio)->GetGroupAll();
    auto channels = group->GetMembers();
    if (channels.empty())
      return false;

    groupMember = channels.front();
    if (!groupMember)
      return false;
  }

  CLog::Log(LOGINFO, "PVR is starting playback of channel '{}'",
            groupMember->Channel()->ChannelName());
  return SwitchToChannel(CFileItem(groupMember), true);
}

bool CPVRGUIActionsPlayback::PlayMedia(const CFileItem& item) const
{
  std::unique_ptr<CFileItem> pvrItem = std::make_unique<CFileItem>(item);
  if (URIUtils::IsPVRChannel(item.GetPath()) && !item.HasPVRChannelInfoTag())
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelGroupMemberByPath(
            item.GetPath());
    if (groupMember)
      pvrItem = std::make_unique<CFileItem>(groupMember);
  }
  else if (URIUtils::IsPVRRecording(item.GetPath()) && !item.HasPVRRecordingInfoTag())
  {
    const std::shared_ptr<CPVRRecording> recording =
        CServiceBroker::GetPVRManager().Recordings()->GetByPath(item.GetPath());
    if (recording)
    {
      pvrItem = std::make_unique<CFileItem>(recording);
      pvrItem->SetStartOffset(item.GetStartOffset());
    }
  }
  bool bCheckResume = true;
  if (item.HasProperty("check_resume"))
    bCheckResume = item.GetProperty("check_resume").asBoolean();

  if (pvrItem && pvrItem->HasPVRChannelInfoTag())
  {
    return SwitchToChannel(*pvrItem, bCheckResume);
  }
  else if (pvrItem && pvrItem->HasPVRRecordingInfoTag())
  {
    return PlayRecording(*pvrItem, bCheckResume);
  }

  return false;
}

void CPVRGUIActionsPlayback::SeekForward()
{
  time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
  if (playbackStartTime > 0)
  {
    const std::shared_ptr<const CPVRChannel> playingChannel =
        CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (playingChannel)
    {
      time_t nextTime = 0;
      std::shared_ptr<CPVREpgInfoTag> next = playingChannel->GetEPGNext();
      if (next)
      {
        next->StartAsUTC().GetAsTime(nextTime);
      }
      else
      {
        // if there is no next event, jump to end of currently playing event
        next = playingChannel->GetEPGNow();
        if (next)
          next->EndAsUTC().GetAsTime(nextTime);
      }

      int64_t seekTime = 0;
      if (nextTime != 0)
      {
        seekTime = (nextTime - playbackStartTime) * 1000;
      }
      else
      {
        // no epg; jump to end of buffer
        seekTime = CServiceBroker::GetDataCacheCore().GetMaxTime();
      }
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
    }
  }
}

void CPVRGUIActionsPlayback::SeekBackward(unsigned int iThreshold)
{
  time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
  if (playbackStartTime > 0)
  {
    const std::shared_ptr<const CPVRChannel> playingChannel =
        CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (playingChannel)
    {
      time_t prevTime = 0;
      std::shared_ptr<CPVREpgInfoTag> prev = playingChannel->GetEPGNow();
      if (prev)
      {
        prev->StartAsUTC().GetAsTime(prevTime);

        // if playback time of current event is above threshold jump to start of current event
        int64_t playTime = CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000;
        if ((playbackStartTime + playTime - prevTime) <= iThreshold)
        {
          // jump to start of previous event
          prevTime = 0;
          prev = playingChannel->GetEPGPrevious();
          if (prev)
            prev->StartAsUTC().GetAsTime(prevTime);
        }
      }

      int64_t seekTime = 0;
      if (prevTime != 0)
      {
        seekTime = (prevTime - playbackStartTime) * 1000;
      }
      else
      {
        // no epg; jump to begin of buffer
        seekTime = CServiceBroker::GetDataCacheCore().GetMinTime();
      }
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
    }
  }
}
