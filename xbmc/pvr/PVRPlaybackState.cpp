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
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRStreamProperties.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Timer.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <mutex>

using namespace PVR;

class CPVRPlaybackState::CLastWatchedUpdateTimer : public CTimer, private ITimerCallback
{
public:
  CLastWatchedUpdateTimer(CPVRPlaybackState& state,
                          const std::shared_ptr<CPVRChannelGroupMember>& channel,
                          const CDateTime& time)
    : CTimer(this), m_state(state), m_channel(channel), m_time(time)
  {
  }

  // ITimerCallback implementation
  void OnTimeout() override { m_state.UpdateLastWatched(m_channel, m_time); }

private:
  CLastWatchedUpdateTimer() = delete;

  CPVRPlaybackState& m_state;
  const std::shared_ptr<CPVRChannelGroupMember> m_channel;
  const CDateTime m_time;
};

CPVRPlaybackState::CPVRPlaybackState() = default;

CPVRPlaybackState::~CPVRPlaybackState() = default;

void CPVRPlaybackState::ReInit()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  Clear();

  if (m_playingClientId != -1)
  {
    if (m_playingChannelUniqueId != -1)
    {
      const std::shared_ptr<const CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetByIdFromAll(m_playingGroupId);
      if (group)
        m_playingChannel = group->GetByUniqueID({m_playingClientId, m_playingChannelUniqueId});
      else
        CLog::LogFC(LOGERROR, LOGPVR, "Failed to obtain group with id '{}'", m_playingGroupId);
    }
    else if (!m_strPlayingRecordingUniqueId.empty())
    {
      m_playingRecording = CServiceBroker::GetPVRManager().Recordings()->GetById(
          m_playingClientId, m_strPlayingRecordingUniqueId);
    }
    else if (m_playingEpgTagChannelUniqueId != -1 && m_playingEpgTagUniqueId != 0)
    {
      const std::shared_ptr<const CPVREpg> epg =
          CServiceBroker::GetPVRManager().EpgContainer().GetByChannelUid(
              m_playingClientId, m_playingEpgTagChannelUniqueId);
      if (epg)
        m_playingEpgTag = epg->GetTagByBroadcastId(m_playingEpgTagUniqueId);
    }
  }

  const std::shared_ptr<const CPVRChannelGroupsContainer> groups =
      CServiceBroker::GetPVRManager().ChannelGroups();
  const CPVRChannelGroups* groupsTV = groups->GetTV();
  const CPVRChannelGroups* groupsRadio = groups->GetRadio();

  m_activeGroupTV = groupsTV->GetLastOpenedGroup();
  m_activeGroupRadio = groupsRadio->GetLastOpenedGroup();
  if (!m_activeGroupTV)
    m_activeGroupTV = groupsTV->GetGroupAll();
  if (!m_activeGroupRadio)
    m_activeGroupRadio = groupsRadio->GetGroupAll();

  GroupMemberPair lastPlayed = groupsTV->GetLastAndPreviousToLastPlayedChannelGroupMember();
  m_lastPlayedChannelTV = lastPlayed.first;
  m_previousToLastPlayedChannelTV = lastPlayed.second;

  lastPlayed = groupsRadio->GetLastAndPreviousToLastPlayedChannelGroupMember();
  m_lastPlayedChannelRadio = lastPlayed.first;
  m_previousToLastPlayedChannelRadio = lastPlayed.second;
}

void CPVRPlaybackState::ClearData()
{
  m_playingGroupId = -1;
  m_playingChannelUniqueId = -1;
  m_strPlayingRecordingUniqueId.clear();
  m_playingEpgTagChannelUniqueId = -1;
  m_playingEpgTagUniqueId = 0;
  m_playingClientId = -1;
  m_strPlayingClientName.clear();
}

void CPVRPlaybackState::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  m_lastPlayedChannelTV.reset();
  m_lastPlayedChannelRadio.reset();
  m_previousToLastPlayedChannelTV.reset();
  m_previousToLastPlayedChannelRadio.reset();
  m_lastWatchedUpdateTimer.reset();
  m_activeGroupTV.reset();
  m_activeGroupRadio.reset();
}

void CPVRPlaybackState::OnPlaybackStarted(const CFileItem& item)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  ClearData();

  if (item.HasPVRChannelGroupMemberInfoTag())
  {
    const std::shared_ptr<CPVRChannelGroupMember> channel = item.GetPVRChannelGroupMemberInfoTag();

    m_playingChannel = channel;
    m_playingGroupId = m_playingChannel->GroupID();
    m_playingClientId = m_playingChannel->Channel()->ClientID();
    m_playingChannelUniqueId = m_playingChannel->Channel()->UniqueID();

    SetActiveChannelGroup(channel);

    if (channel->Channel()->IsRadio())
    {
      if (m_lastPlayedChannelRadio != channel)
      {
        m_previousToLastPlayedChannelRadio = m_lastPlayedChannelRadio;
        m_lastPlayedChannelRadio = channel;
      }
    }
    else
    {
      if (m_lastPlayedChannelTV != channel)
      {
        m_previousToLastPlayedChannelTV = m_lastPlayedChannelTV;
        m_lastPlayedChannelTV = channel;
      }
    }

    int iLastWatchedDelay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED) *
                            1000;
    if (iLastWatchedDelay > 0)
    {
      // Insert new / replace existing last watched update timer
      if (m_lastWatchedUpdateTimer)
        m_lastWatchedUpdateTimer->Stop(true);

      m_lastWatchedUpdateTimer =
          std::make_unique<CLastWatchedUpdateTimer>(*this, channel, CDateTime::GetUTCDateTime());
      m_lastWatchedUpdateTimer->Start(std::chrono::milliseconds(iLastWatchedDelay));
    }
    else
    {
      // Store last watched timestamp immediately
      UpdateLastWatched(channel, CDateTime::GetUTCDateTime());
    }
  }
  else if (item.HasPVRRecordingInfoTag())
  {
    m_playingRecording = item.GetPVRRecordingInfoTag();
    m_playingClientId = m_playingRecording->ClientID();
    m_strPlayingRecordingUniqueId = m_playingRecording->ClientRecordingID();
  }
  else if (item.HasEPGInfoTag())
  {
    m_playingEpgTag = item.GetEPGInfoTag();
    m_playingClientId = m_playingEpgTag->ClientID();
    m_playingEpgTagChannelUniqueId = m_playingEpgTag->UniqueChannelID();
    m_playingEpgTagUniqueId = m_playingEpgTag->UniqueBroadcastID();
  }
  else if (item.HasPVRChannelInfoTag())
  {
    CLog::LogFC(LOGERROR, LOGPVR, "Channel item without channel group member!");
  }

  if (m_playingClientId != -1)
  {
    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(m_playingClientId);
    if (client)
      m_strPlayingClientName = client->GetFullClientName();
  }
}

bool CPVRPlaybackState::OnPlaybackStopped(const CFileItem& item)
{
  // Playback ended due to user interaction

  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool bChanged = false;

  if (item.HasPVRChannelGroupMemberInfoTag() &&
      item.GetPVRChannelGroupMemberInfoTag()->Channel()->ClientID() == m_playingClientId &&
      item.GetPVRChannelGroupMemberInfoTag()->Channel()->UniqueID() == m_playingChannelUniqueId)
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
    ClearData();
  }
  else if (item.HasPVRRecordingInfoTag() &&
           item.GetPVRRecordingInfoTag()->ClientID() == m_playingClientId &&
           item.GetPVRRecordingInfoTag()->ClientRecordingID() == m_strPlayingRecordingUniqueId)
  {
    bChanged = true;
    m_playingRecording.reset();
    ClearData();
  }
  else if (item.HasEPGInfoTag() && item.GetEPGInfoTag()->ClientID() == m_playingClientId &&
           item.GetEPGInfoTag()->UniqueChannelID() == m_playingEpgTagChannelUniqueId &&
           item.GetEPGInfoTag()->UniqueBroadcastID() == m_playingEpgTagUniqueId)
  {
    bChanged = true;
    m_playingEpgTag.reset();
    ClearData();
  }
  else if (item.HasPVRChannelInfoTag())
  {
    CLog::LogFC(LOGERROR, LOGPVR, "Channel item without channel group member!");
  }

  return bChanged;
}

std::unique_ptr<CFileItem> CPVRPlaybackState::GetNextAutoplayItem(const CFileItem& item)
{
  if (!item.GetProperty("epg_playlist_item").asBoolean(false))
    return {};

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (item.HasEPGInfoTag() && item.GetEPGInfoTag()->ClientID() == m_playingClientId &&
      item.GetEPGInfoTag()->UniqueChannelID() == m_playingEpgTagChannelUniqueId &&
      item.GetEPGInfoTag()->UniqueBroadcastID() == m_playingEpgTagUniqueId)
  {
    auto& pvrMgr{CServiceBroker::GetPVRManager()};
    const std::shared_ptr<const CPVREpg> epg{
        pvrMgr.EpgContainer().GetByChannelUid(m_playingClientId, m_playingEpgTagChannelUniqueId)};
    const std::shared_ptr<const CPVRClient> client{pvrMgr.GetClient(m_playingClientId)};
    if (epg && client)
    {
      const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags{epg->GetTags()};
      bool nextIsMatch{false};
      for (const auto& tag : tags)
      {
        if (nextIsMatch)
        {
          // Next to play is successor of given item in channel's timeline.
          return std::make_unique<CFileItem>(tag);
        }

        if (tag != tags.back() && tag->StartAsUTC() == m_playingEpgTag->StartAsUTC() &&
            tag->EndAsUTC() == m_playingEpgTag->EndAsUTC())
          nextIsMatch = true;
      }

      if (!nextIsMatch)
      {
        // No more non-live epg items in channel's timeline. Next to play is the live channel.
        const std::shared_ptr<CPVRChannelGroup> group{
            pvrMgr.ChannelGroups()->Get(m_playingEpgTag->IsRadio())->GetGroupAll()};
        if (group)
        {
          const std::shared_ptr<CPVRChannelGroupMember> groupMember{
              group->GetByUniqueID({m_playingClientId, m_playingEpgTagChannelUniqueId})};
          if (groupMember)
            return std::make_unique<CFileItem>(groupMember);
        }
      }
    }
  }
  return {};
}

bool CPVRPlaybackState::OnPlaybackEnded(const CFileItem& item)
{
  // Playback ended, but not due to user interaction

  std::unique_ptr<CFileItem> nextToPlay{GetNextAutoplayItem(item)};
  if (nextToPlay)
    StartPlayback(nextToPlay.release());

  return OnPlaybackStopped(item);
}

void CPVRPlaybackState::StartPlayback(
    CFileItem* item,
    ContentUtils::PlayMode mode /* = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM */) const
{
  // Obtain dynamic playback url and properties from the respective pvr client
  const std::shared_ptr<const CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
  if (client)
  {
    CPVRStreamProperties props;

    if (item->IsPVRChannel())
    {
      client->GetChannelStreamProperties(item->GetPVRChannelInfoTag(), props);
    }
    else if (item->IsPVRRecording())
    {
      client->GetRecordingStreamProperties(item->GetPVRRecordingInfoTag(), props);
    }
    else if (item->IsEPG())
    {
      client->GetEpgTagStreamProperties(item->GetEPGInfoTag(), props);

      if (mode == ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM)
      {
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME))
          item->SetProperty("epg_playlist_item", true);
      }
      else if (mode == ContentUtils::PlayMode::PLAY_FROM_HERE)
      {
        item->SetProperty("epg_playlist_item", true);
      }
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
}

bool CPVRPlaybackState::IsPlaying() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel != nullptr || m_playingRecording != nullptr || m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingTV() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel && !m_playingChannel->Channel()->IsRadio();
}

bool CPVRPlaybackState::IsPlayingRadio() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel && m_playingChannel->Channel()->IsRadio();
}

bool CPVRPlaybackState::IsPlayingEncryptedChannel() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel && m_playingChannel->Channel()->IsEncrypted();
}

bool CPVRPlaybackState::IsPlayingRecording() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingRecording != nullptr;
}

bool CPVRPlaybackState::IsPlayingEpgTag() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingChannel(int iClientID, int iUniqueChannelID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingClientId == iClientID && m_playingChannelUniqueId == iUniqueChannelID;
}

bool CPVRPlaybackState::IsPlayingChannel(const std::shared_ptr<const CPVRChannel>& channel) const
{
  if (channel)
  {
    const std::shared_ptr<const CPVRChannel> current = GetPlayingChannel();
    if (current && current->ClientID() == channel->ClientID() &&
        current->UniqueID() == channel->UniqueID())
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingRecording(
    const std::shared_ptr<const CPVRRecording>& recording) const
{
  if (recording)
  {
    const std::shared_ptr<const CPVRRecording> current = GetPlayingRecording();
    if (current && current->ClientID() == recording->ClientID() &&
        current->ClientRecordingID() == recording->ClientRecordingID())
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingEpgTag(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (epgTag)
  {
    const std::shared_ptr<const CPVREpgInfoTag> current = GetPlayingEpgTag();
    if (current && current->ClientID() == epgTag->ClientID() &&
        current->UniqueChannelID() == epgTag->UniqueChannelID() &&
        current->UniqueBroadcastID() == epgTag->UniqueBroadcastID())
      return true;
  }

  return false;
}

std::shared_ptr<CPVRChannel> CPVRPlaybackState::GetPlayingChannel() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel ? m_playingChannel->Channel() : std::shared_ptr<CPVRChannel>();
}

std::shared_ptr<CPVRChannelGroupMember> CPVRPlaybackState::GetPlayingChannelGroupMember() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannel;
}

std::shared_ptr<CPVRRecording> CPVRPlaybackState::GetPlayingRecording() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingRecording;
}

std::shared_ptr<CPVREpgInfoTag> CPVRPlaybackState::GetPlayingEpgTag() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingEpgTag;
}

int CPVRPlaybackState::GetPlayingChannelUniqueID() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingChannelUniqueId;
}

std::string CPVRPlaybackState::GetPlayingClientName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strPlayingClientName;
}

int CPVRPlaybackState::GetPlayingClientID() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_playingClientId;
}

bool CPVRPlaybackState::IsRecording() const
{
  return CServiceBroker::GetPVRManager().Timers()->IsRecording();
}

bool CPVRPlaybackState::IsRecordingOnPlayingChannel() const
{
  const std::shared_ptr<const CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel &&
         CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*currentChannel);
}

bool CPVRPlaybackState::IsPlayingActiveRecording() const
{
  const std::shared_ptr<const CPVRRecording> currentRecording = GetPlayingRecording();
  return currentRecording && currentRecording->IsInProgress();
}

bool CPVRPlaybackState::CanRecordOnPlayingChannel() const
{
  const std::shared_ptr<const CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel && currentChannel->CanRecord();
}

namespace
{
std::shared_ptr<CPVRChannelGroup> GetFirstNonDeletedAndNonHiddenChannelGroup(
    const std::shared_ptr<CPVRChannelGroupMember>& groupMember)
{
  CPVRChannelGroups* groups{
      CServiceBroker::GetPVRManager().ChannelGroups()->Get(groupMember->IsRadio())};
  if (groups)
  {
    const std::vector<std::shared_ptr<CPVRChannelGroup>> members{
        groups->GetMembers(true /* exclude hidden */)};

    for (const auto& member : members)
    {
      if (member->IsDeleted())
        continue;

      if (member->GetByUniqueID(groupMember->Channel()->StorageId()))
        return member;
    }
  }

  CLog::LogFC(LOGERROR, LOGPVR,
              "Failed to obtain non-deleted and non-hidden group for channel '{}‘",
              groupMember->Channel()->ChannelName());
  return {};
}
} // unnamed namespace

void CPVRPlaybackState::SetActiveChannelGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (group)
  {
    if (group->IsHidden() || group->IsDeleted())
    {
      CLog::LogFC(LOGERROR, LOGPVR,
                  "Rejecting to make hidden or deleted group '{}‘ the active group.",
                  group->GroupName());
      return;
    }

    if (group->IsRadio())
      m_activeGroupRadio = group;
    else
      m_activeGroupTV = group;

    auto duration = std::chrono::system_clock::now().time_since_epoch();
    uint64_t tsMillis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    group->SetLastOpened(tsMillis);
  }
}

void CPVRPlaybackState::SetActiveChannelGroup(
    const std::shared_ptr<CPVRChannelGroupMember>& channel)
{
  const bool bRadio = channel->Channel()->IsRadio();
  std::shared_ptr<CPVRChannelGroup> group{
      CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio)->GetById(channel->GroupID())};

  if (group && (group->IsHidden() || group->IsDeleted()))
    group = GetFirstNonDeletedAndNonHiddenChannelGroup(channel);

  SetActiveChannelGroup(group);
}

std::shared_ptr<CPVRChannelGroup> CPVRPlaybackState::GetActiveChannelGroup(bool bRadio) const
{
  if (bRadio)
    return m_activeGroupRadio;
  else
    return m_activeGroupTV;
}

CDateTime CPVRPlaybackState::GetPlaybackTime(int iClientID, int iUniqueChannelID) const
{
  const std::shared_ptr<const CPVREpgInfoTag> epgTag = GetPlayingEpgTag();
  if (epgTag && iClientID == epgTag->ClientID() && iUniqueChannelID == epgTag->UniqueChannelID())
  {
    // playing an epg tag on requested channel
    return epgTag->StartAsUTC() +
           CDateTimeSpan(0, 0, 0, CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
  }

  // not playing / playing live / playing timeshifted
  return GetChannelPlaybackTime(iClientID, iUniqueChannelID);
}

CDateTime CPVRPlaybackState::GetChannelPlaybackTime(int iClientID, int iUniqueChannelID) const
{
  if (IsPlayingChannel(iClientID, iUniqueChannelID))
  {
    // start time valid?
    time_t startTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (startTime > 0)
      return CDateTime(startTime + CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
  }

  // not playing / playing live
  return CDateTime::GetUTCDateTime();
}

void CPVRPlaybackState::UpdateLastWatched(const std::shared_ptr<CPVRChannelGroupMember>& channel,
                                          const CDateTime& time)
{
  time_t iTime;
  time.GetAsTime(iTime);

  channel->Channel()->SetLastWatched(iTime, channel->GroupID());

  // update last watched timestamp for group
  const bool bRadio = channel->Channel()->IsRadio();
  const std::shared_ptr<CPVRChannelGroup> group =
      CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio)->GetById(channel->GroupID());
  if (group)
    group->SetLastWatched(iTime);
}

std::shared_ptr<CPVRChannelGroupMember> CPVRPlaybackState::GetLastPlayedChannelGroupMember(
    bool bRadio) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return bRadio ? m_lastPlayedChannelRadio : m_lastPlayedChannelTV;
}

std::shared_ptr<CPVRChannelGroupMember> CPVRPlaybackState::
    GetPreviousToLastPlayedChannelGroupMember(bool bRadio) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return bRadio ? m_previousToLastPlayedChannelRadio : m_previousToLastPlayedChannelTV;
}
