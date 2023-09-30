/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsPlayback.h"

#include "FileItem.h"
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
#include "video/guilib/VideoSelectActionProcessor.h"

#include <memory>
#include <string>
#include <vector>

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

  const VIDEO::GUILIB::SelectAction action =
      VIDEO::GUILIB::CVideoSelectActionProcessorBase::ChoosePlayOrResume(item);
  if (action == VIDEO::GUILIB::SELECT_ACTION_RESUME)
  {
    const_cast<CFileItem*>(&item)->SetStartOffset(STARTOFFSET_RESUME);
  }
  else if (action == VIDEO::GUILIB::SELECT_ACTION_PLAY)
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
  if (VIDEO_UTILS::GetItemResumeInformation(item).isResumable)
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

void CPVRGUIActionsPlayback::StartPlayback(CFileItem* item,
                                           bool bFullscreen,
                                           const CPVRStreamProperties* epgProps) const
{
  // Obtain dynamic playback url and properties from the respective pvr client
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
  if (client)
  {
    CPVRStreamProperties props;

    if (item->IsPVRChannel())
    {
      // If this was an EPG Tag to be played as live then PlayEpgTag() will create a channel
      // fileitem instead and pass the epg tags props so we use those and skip the client call
      if (epgProps)
        props = *epgProps;
      else
        client->GetChannelStreamProperties(item->GetPVRChannelInfoTag(), props);
    }
    else if (item->IsPVRRecording())
    {
      client->GetRecordingStreamProperties(item->GetPVRRecordingInfoTag(), props);
    }
    else if (item->IsEPG())
    {
      if (epgProps) // we already have props from PlayEpgTag()
        props = *epgProps;
      else
        client->GetEpgTagStreamProperties(item->GetEPGInfoTag(), props);
    }

    if (props.size())
    {
      const std::string url = props.GetStreamURL();
      if (!url.empty())
        item->SetDynPath(url);

      const std::string mime = props.GetStreamMimeType();
      if (!mime.empty())
      {
        item->SetMimeType(mime);
        item->SetContentLookup(false);
      }

      for (const auto& prop : props)
        item->SetProperty(prop.first, prop.second);
    }
  }

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
  CheckAndSwitchToFullscreen(bFullscreen);
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
    if (!item.m_bIsFolder && VIDEO_UTILS::IsAutoPlayNextItem(item))
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
      VIDEO_UTILS::GetItemsForPlayList(parentItem, *queuedItems);

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
      CheckAndSwitchToFullscreen(true);
    }
    else
    {
      CFileItem* itemToPlay = new CFileItem(recording);
      itemToPlay->SetStartOffset(item.GetStartOffset());
      StartPlayback(itemToPlay, true);
    }
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
    VIDEO_UTILS::GetItemsForPlayList(itemToQueue, *queuedItems);

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, -1,
                                               static_cast<void*>(queuedItems.release()));
    CheckAndSwitchToFullscreen(true);
  }
  return true;
}

bool CPVRGUIActionsPlayback::PlayEpgTag(const CFileItem& item) const
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
  const std::shared_ptr<CPVRClient> client =
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

  StartPlayback(itemToPlay, true, &props);
  return true;
}

bool CPVRGUIActionsPlayback::SwitchToChannel(const CFileItem& item, bool bCheckResume) const
{
  if (item.m_bIsFolder)
    return false;

  std::shared_ptr<CPVRRecording> recording;
  const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
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

    StartPlayback(new CFileItem(groupMember), bFullscreen);
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

      const std::shared_ptr<CPVRChannelGroup> allGroup =
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
    const std::shared_ptr<CPVRChannelGroup> channelGroup =
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
    const std::shared_ptr<CPVRChannelGroup> group =
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
    const std::shared_ptr<CPVRChannel> playingChannel =
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
    const std::shared_ptr<CPVRChannel> playingChannel =
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
