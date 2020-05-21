/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRPlaybackState.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "cores/DataCacheCore.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Timer.h"

using namespace PVR;

class CPVRPlaybackState::CLastWatchedUpdateTimer : public CTimer, private ITimerCallback
{
public:
  CLastWatchedUpdateTimer(CPVRPlaybackState& state,
                          const std::shared_ptr<CPVRChannel>& channel,
                          const CDateTime& time)
    : CTimer(this)
    , m_state(state)
    , m_channel(channel)
    , m_time(time)
  {
  }

  // ITimerCallback implementation
  void OnTimeout() override
  {
    m_state.UpdateLastWatched(m_channel, m_time);
  }

private:
  CLastWatchedUpdateTimer() = delete;

  CPVRPlaybackState& m_state;
  const std::shared_ptr<CPVRChannel> m_channel;
  const CDateTime m_time;
};


CPVRPlaybackState::CPVRPlaybackState() = default;

CPVRPlaybackState::~CPVRPlaybackState() = default;

void CPVRPlaybackState::OnPlaybackStarted(const std::shared_ptr<CFileItem> item)
{
  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  m_playingClientId = -1;
  m_playingChannelUniqueId = -1;
  m_strPlayingClientName.clear();

  if (item->HasPVRChannelInfoTag())
  {
    const std::shared_ptr<CPVRChannel> channel = item->GetPVRChannelInfoTag();

    m_playingChannel = channel;
    m_playingClientId = m_playingChannel->ClientID();
    m_playingChannelUniqueId = m_playingChannel->UniqueID();

    SetPlayingGroup(channel);

    int iLastWatchedDelay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED) * 1000;
    if (iLastWatchedDelay > 0)
    {
      // Insert new / replace existing last watched update timer
      if (m_lastWatchedUpdateTimer)
        m_lastWatchedUpdateTimer->Stop(true);

      m_lastWatchedUpdateTimer.reset(new CLastWatchedUpdateTimer(*this, channel, CDateTime::GetUTCDateTime()));
      m_lastWatchedUpdateTimer->Start(iLastWatchedDelay);
    }
    else
    {
      // Store last watched timestamp immediately
      UpdateLastWatched(channel, CDateTime::GetUTCDateTime());
    }
  }
  else if (item->HasPVRRecordingInfoTag())
  {
    m_playingRecording = item->GetPVRRecordingInfoTag();
    m_playingClientId = m_playingRecording->m_iClientId;
  }
  else if (item->HasEPGInfoTag())
  {
    m_playingEpgTag = item->GetEPGInfoTag();
    m_playingClientId = m_playingEpgTag->ClientID();
  }

  if (m_playingClientId != -1)
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_playingClientId);
    if (client)
      m_strPlayingClientName = client->GetFriendlyName();
  }
}

bool CPVRPlaybackState::OnPlaybackStopped(const std::shared_ptr<CFileItem> item)
{
  // Playback ended due to user interaction

  bool bChanged = false;

  if (item->HasPVRChannelInfoTag() && item->GetPVRChannelInfoTag() == m_playingChannel)
  {
    bool bUpdateLastWatched = true;

    if (m_lastWatchedUpdateTimer)
    {
      if (m_lastWatchedUpdateTimer->IsRunning())
      {
        // If last watched timer is still running, cancel it. Channel was not watched long enough to store the value.
        m_lastWatchedUpdateTimer->Stop(true);
        bUpdateLastWatched = false;
      }
      m_lastWatchedUpdateTimer.reset();
    }

    if (bUpdateLastWatched)
    {
      // If last watched timer is not running (any more), channel was watched long enough to store the value.
      UpdateLastWatched(m_playingChannel, CDateTime::GetUTCDateTime());
    }

    bChanged = true;
    m_playingChannel.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingClientName.clear();
  }
  else if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag() == m_playingRecording)
  {
    bChanged = true;
    m_playingRecording.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingClientName.clear();
  }
  else if (item->HasEPGInfoTag() && m_playingEpgTag && *item->GetEPGInfoTag() == *m_playingEpgTag)
  {
    bChanged = true;
    m_playingEpgTag.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingClientName.clear();
  }

  return bChanged;
}

void CPVRPlaybackState::OnPlaybackEnded(const std::shared_ptr<CFileItem> item)
{
  // Playback ended, but not due to user interaction
  OnPlaybackStopped(item);
}

bool CPVRPlaybackState::IsPlaying() const
{
  return m_playingChannel != nullptr || m_playingRecording != nullptr || m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingTV() const
{
  return m_playingChannel && !m_playingChannel->IsRadio();
}

bool CPVRPlaybackState::IsPlayingRadio() const
{
  return m_playingChannel && m_playingChannel->IsRadio();
}

bool CPVRPlaybackState::IsPlayingEncryptedChannel() const
{
  return m_playingChannel && m_playingChannel->IsEncrypted();
}

bool CPVRPlaybackState::IsPlayingRecording() const
{
  return m_playingRecording != nullptr;
}

bool CPVRPlaybackState::IsPlayingEpgTag() const
{
  return m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingChannel(int iClientID, int iUniqueChannelID) const
{
  return m_playingChannel && m_playingClientId == iClientID && m_playingChannelUniqueId == iUniqueChannelID;
}

bool CPVRPlaybackState::IsPlayingChannel(const std::shared_ptr<CPVRChannel>& channel) const
{
  if (channel)
  {
    const std::shared_ptr<CPVRChannel> current = GetPlayingChannel();
    if (current && *current == *channel)
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingRecording(const std::shared_ptr<CPVRRecording>& recording) const
{
  if (recording)
  {
    const std::shared_ptr<CPVRRecording> current = GetPlayingRecording();
    if (current && *current == *recording)
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (epgTag)
  {
    const std::shared_ptr<CPVREpgInfoTag> current = GetPlayingEpgTag();
    if (current && *current == *epgTag)
      return true;
  }

  return false;
}

std::shared_ptr<CPVRChannel> CPVRPlaybackState::GetPlayingChannel() const
{
  return m_playingChannel;
}

std::shared_ptr<CPVRRecording> CPVRPlaybackState::GetPlayingRecording() const
{
  return m_playingRecording;
}

std::shared_ptr<CPVREpgInfoTag> CPVRPlaybackState::GetPlayingEpgTag() const
{
  return m_playingEpgTag;
}

int CPVRPlaybackState::GetPlayingChannelUniqueID() const
{
  return m_playingChannelUniqueId;
}

std::string CPVRPlaybackState::GetPlayingClientName() const
{
  return m_strPlayingClientName;
}

int CPVRPlaybackState::GetPlayingClientID() const
{
  return m_playingClientId;
}

bool CPVRPlaybackState::IsRecording() const
{
  return CServiceBroker::GetPVRManager().Timers()->IsRecording();
}

bool CPVRPlaybackState::IsRecordingOnPlayingChannel() const
{
  const std::shared_ptr<CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel && CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*currentChannel);
}

bool CPVRPlaybackState::IsPlayingActiveRecording() const
{
  const std::shared_ptr<CPVRRecording> currentRecording = GetPlayingRecording();
  return currentRecording && currentRecording->IsInProgress();
}

bool CPVRPlaybackState::CanRecordOnPlayingChannel() const
{
  const std::shared_ptr<CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel && currentChannel->CanRecord();
}

void CPVRPlaybackState::SetPlayingGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (group)
    CServiceBroker::GetPVRManager().ChannelGroups()->Get(group->IsRadio())->SetSelectedGroup(group);
}

void CPVRPlaybackState::SetPlayingGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  const std::shared_ptr<CPVRChannelGroup> group = CServiceBroker::GetPVRManager().ChannelGroups()->GetSelectedGroup(channel->IsRadio());
  if (!group || !group->IsGroupMember(channel))
  {
    // The channel we'll switch to is not part of the current selected group.
    // Set the first group as the selected group where the channel is a member.
    CPVRChannelGroups* channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(channel->IsRadio());
    const std::vector<std::shared_ptr<CPVRChannelGroup>> groups = channelGroups->GetGroupsByChannel(channel, true);
    if (!groups.empty())
      channelGroups->SetSelectedGroup(groups.front());
  }
}

std::shared_ptr<CPVRChannelGroup> CPVRPlaybackState::GetPlayingGroup(bool bRadio) const
{
  return CServiceBroker::GetPVRManager().ChannelGroups()->GetSelectedGroup(bRadio);
}

CDateTime CPVRPlaybackState::GetPlaybackTime() const
{
  // start time valid?
  time_t startTime = CServiceBroker::GetDataCacheCore().GetStartTime();
  if (startTime > 0)
    return CDateTime(startTime + CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
  else
    return CDateTime::GetUTCDateTime();
}

void CPVRPlaybackState::UpdateLastWatched(const std::shared_ptr<CPVRChannel>& channel, const CDateTime& time)
{
  time_t iTime;
  time.GetAsTime(iTime);

  channel->SetLastWatched(iTime);

  // update last watched timestamp for group
  const std::shared_ptr<CPVRChannelGroup> group = GetPlayingGroup(channel->IsRadio());
  group->SetLastWatched(iTime);

  CServiceBroker::GetPVRManager().ChannelGroups()->SetLastPlayedGroup(group);
}
