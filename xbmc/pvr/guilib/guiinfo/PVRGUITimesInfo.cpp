/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUITimesInfo.h"

#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <cmath>
#include <ctime>
#include <memory>
#include <mutex>

using namespace PVR;

CPVRGUITimesInfo::CPVRGUITimesInfo()
{
  Reset();
}

void CPVRGUITimesInfo::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_iStartTime = 0;
  m_iDuration = 0;
  m_iTimeshiftStartTime = 0;
  m_iTimeshiftEndTime = 0;
  m_iTimeshiftPlayTime = 0;
  m_iTimeshiftOffset = 0;

  m_iTimeshiftProgressStartTime = 0;
  m_iTimeshiftProgressEndTime = 0;
  m_iTimeshiftProgressDuration = 0;

  m_playingEpgTag.reset();
  m_playingChannel.reset();
}

void CPVRGUITimesInfo::UpdatePlayingTag()
{
  const std::shared_ptr<const CPVRChannel> currentChannel =
      CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
  std::shared_ptr<CPVREpgInfoTag> currentTag = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingEpgTag();

  if (currentChannel || currentTag)
  {
    if (currentChannel && !currentTag)
      currentTag = currentChannel->GetEPGNow();

    const std::shared_ptr<const CPVRChannelGroupsContainer> groups =
        CServiceBroker::GetPVRManager().ChannelGroups();

    std::unique_lock<CCriticalSection> lock(m_critSection);

    const std::shared_ptr<const CPVRChannel> playingChannel =
        m_playingEpgTag ? groups->GetChannelForEpgTag(m_playingEpgTag) : nullptr;

    if (!m_playingEpgTag || !currentTag || !playingChannel || !currentChannel ||
        m_playingEpgTag->StartAsUTC() != currentTag->StartAsUTC() ||
        m_playingEpgTag->EndAsUTC() != currentTag->EndAsUTC() || *playingChannel != *currentChannel)
    {
      if (currentTag)
      {
        m_playingEpgTag = currentTag;
        m_iDuration = m_playingEpgTag->GetDuration();
      }
      else if (m_iTimeshiftEndTime > m_iTimeshiftStartTime)
      {
        m_playingEpgTag.reset();
        m_iDuration = m_iTimeshiftEndTime - m_iTimeshiftStartTime;
      }
      else
      {
        m_playingEpgTag.reset();
        m_iDuration = 0;
      }
    }
  }
  else
  {
    const std::shared_ptr<const CPVRRecording> recording =
        CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingRecording();
    if (recording)
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      m_playingEpgTag.reset();
      m_iDuration = recording->GetDuration();
    }
  }
}

void CPVRGUITimesInfo::UpdateTimeshiftData()
{
  if (!CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV() && !CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
  {
    // If nothing is playing (anymore), there is no need to update data.
    Reset();
    return;
  }

  time_t now = std::time(nullptr);
  time_t iStartTime;
  int64_t iPlayTime, iMinTime, iMaxTime;
  CServiceBroker::GetDataCacheCore().GetPlayTimes(iStartTime, iPlayTime, iMinTime, iMaxTime);
  bool bPlaying = CServiceBroker::GetDataCacheCore().GetSpeed() == 1.0f;
  const std::shared_ptr<const CPVRChannel> playingChannel =
      CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (playingChannel != m_playingChannel)
  {
    // playing channel changed. we need to reset offset and playtime.
    m_iTimeshiftOffset = 0;
    m_iTimeshiftPlayTime = 0;
    m_playingChannel = playingChannel;
  }

  if (!iStartTime)
  {
    if (m_iStartTime == 0)
      iStartTime = now;
    else
      iStartTime = m_iStartTime;

    iMinTime = iPlayTime;
    iMaxTime = iPlayTime;
  }

  m_iStartTime = iStartTime;
  m_iTimeshiftStartTime = iStartTime + iMinTime / 1000;
  m_iTimeshiftEndTime = iStartTime + iMaxTime / 1000;

  if (m_iTimeshiftEndTime > m_iTimeshiftStartTime)
  {
    // timeshifting supported
    m_iTimeshiftPlayTime = iStartTime + iPlayTime / 1000;
    if (iMaxTime > iPlayTime)
      m_iTimeshiftOffset = (iMaxTime - iPlayTime) / 1000;
    else
      m_iTimeshiftOffset = 0;
  }
  else
  {
    // timeshifting not supported
    if (bPlaying)
      m_iTimeshiftPlayTime = now - m_iTimeshiftOffset;

    m_iTimeshiftOffset = now - m_iTimeshiftPlayTime;
  }

  UpdateTimeshiftProgressData();
}

void CPVRGUITimesInfo::UpdateTimeshiftProgressData()
{
  // Note: General idea of the ts progress is always to be able to visualise both the complete
  //       ts buffer and the complete playing epg event (if any) side by side with the same time
  //       scale. TS progress start and end times will be calculated accordingly.
  //       + Start is usually ts buffer start, except if start time of playing epg event is
  //         before ts buffer start, then progress start is epg event start.
  //       + End is usually ts buffer end, except if end time of playing epg event is
  //         after ts buffer end, then progress end is epg event end.
  //       In simple timeshift mode (settings value), progress start is always the start time of
  //       playing epg event and progress end is always the end time of playing epg event.

  std::unique_lock<CCriticalSection> lock(m_critSection);

  //////////////////////////////////////////////////////////////////////////////////////
  // start time
  //////////////////////////////////////////////////////////////////////////////////////
  bool bUpdatedStartTime = false;
  if (m_playingEpgTag)
  {
    time_t start = 0;
    m_playingEpgTag->StartAsUTC().GetAsTime(start);
    if (start < m_iTimeshiftStartTime ||
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRTimeshiftSimpleOSD)
    {
      // playing event started before start of ts buffer or simple ts osd to be used
      m_iTimeshiftProgressStartTime = start;
      bUpdatedStartTime = true;
    }
  }

  if (!bUpdatedStartTime)
  {
    // default to ts buffer start
    m_iTimeshiftProgressStartTime = m_iTimeshiftStartTime;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  // end time
  //////////////////////////////////////////////////////////////////////////////////////
  bool bUpdatedEndTime = false;
  if (m_playingEpgTag)
  {
    time_t end = 0;
    m_playingEpgTag->EndAsUTC().GetAsTime(end);
    if (end > m_iTimeshiftEndTime ||
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRTimeshiftSimpleOSD)
    {
      // playing event will end after end of ts buffer or simple ts osd to be used
      m_iTimeshiftProgressEndTime = end;
      bUpdatedEndTime = true;
    }
  }

  if (!bUpdatedEndTime)
  {
    // default to ts buffer end
    m_iTimeshiftProgressEndTime = m_iTimeshiftEndTime;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  // duration
  //////////////////////////////////////////////////////////////////////////////////////
  m_iTimeshiftProgressDuration = m_iTimeshiftProgressEndTime - m_iTimeshiftProgressStartTime;
}

void CPVRGUITimesInfo::Update()
{
  UpdatePlayingTag();
  UpdateTimeshiftData();
}

std::string CPVRGUITimesInfo::TimeToTimeString(time_t datetime, TIME_FORMAT format, bool withSeconds)
{
  CDateTime time;
  time.SetFromUTCDateTime(datetime);
  return time.GetAsLocalizedTime(format, withSeconds);
}

std::string CPVRGUITimesInfo::GetTimeshiftStartTime(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return TimeToTimeString(m_iTimeshiftStartTime, format, false);
}

std::string CPVRGUITimesInfo::GetTimeshiftEndTime(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return TimeToTimeString(m_iTimeshiftEndTime, format, false);
}

std::string CPVRGUITimesInfo::GetTimeshiftPlayTime(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return TimeToTimeString(m_iTimeshiftPlayTime, format, true);
}

std::string CPVRGUITimesInfo::GetTimeshiftOffset(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return StringUtils::SecondsToTimeString(m_iTimeshiftOffset, format);
}

std::string CPVRGUITimesInfo::GetTimeshiftProgressDuration(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return StringUtils::SecondsToTimeString(m_iTimeshiftProgressDuration, format);
}

std::string CPVRGUITimesInfo::GetTimeshiftProgressStartTime(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return TimeToTimeString(m_iTimeshiftProgressStartTime, format, false);
}

std::string CPVRGUITimesInfo::GetTimeshiftProgressEndTime(TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return TimeToTimeString(m_iTimeshiftProgressEndTime, format, false);
}

std::string CPVRGUITimesInfo::GetEpgEventDuration(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag, TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return StringUtils::SecondsToTimeString(GetEpgEventDuration(epgTag), format);
}

std::string CPVRGUITimesInfo::GetEpgEventElapsedTime(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag, TIME_FORMAT format) const
{
  int iElapsed = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (epgTag && m_playingEpgTag && *epgTag != *m_playingEpgTag)
    iElapsed = epgTag->Progress();
  else
    iElapsed = GetElapsedTime();

  return StringUtils::SecondsToTimeString(iElapsed, format);
}

std::string CPVRGUITimesInfo::GetEpgEventRemainingTime(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag, TIME_FORMAT format) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return StringUtils::SecondsToTimeString(GetRemainingTime(epgTag), format);
}

std::string CPVRGUITimesInfo::GetEpgEventFinishTime(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag, TIME_FORMAT format) const
{
  CDateTime finish = CDateTime::GetCurrentDateTime();
  finish += CDateTimeSpan(0, 0, 0, GetRemainingTime(epgTag));
  return finish.GetAsLocalizedTime(format);
}

std::string CPVRGUITimesInfo::GetEpgEventSeekTime(int iSeekSize, TIME_FORMAT format) const
{
  return StringUtils::SecondsToTimeString(GetElapsedTime() + iSeekSize, format);
}

int CPVRGUITimesInfo::GetElapsedTime() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_playingEpgTag || m_iTimeshiftStartTime)
  {
    CDateTime current(m_iTimeshiftPlayTime);
    CDateTime start = m_playingEpgTag ? m_playingEpgTag->StartAsUTC() : CDateTime(m_iTimeshiftStartTime);
    CDateTimeSpan time = current > start ? current - start : CDateTimeSpan(0, 0, 0, 0);
    return time.GetSecondsTotal();
  }
  else
  {
    return 0;
  }
}

int CPVRGUITimesInfo::GetRemainingTime(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (epgTag && m_playingEpgTag && *epgTag != *m_playingEpgTag)
    return epgTag->GetDuration() - epgTag->Progress();
  else
    return m_iDuration - GetElapsedTime();
}

int CPVRGUITimesInfo::GetTimeshiftProgress() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::lrintf(static_cast<float>(m_iTimeshiftPlayTime - m_iTimeshiftStartTime) / (m_iTimeshiftEndTime - m_iTimeshiftStartTime) * 100);
}

int CPVRGUITimesInfo::GetTimeshiftProgressDuration() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iTimeshiftProgressDuration;
}

int CPVRGUITimesInfo::GetTimeshiftProgressPlayPosition() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::lrintf(static_cast<float>(m_iTimeshiftPlayTime - m_iTimeshiftProgressStartTime) / m_iTimeshiftProgressDuration * 100);
}

int CPVRGUITimesInfo::GetTimeshiftProgressEpgStart() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_playingEpgTag)
  {
    time_t epgStart = 0;
    m_playingEpgTag->StartAsUTC().GetAsTime(epgStart);
    return std::lrintf(static_cast<float>(epgStart - m_iTimeshiftProgressStartTime) / m_iTimeshiftProgressDuration * 100);
  }
  return 0;
}

int CPVRGUITimesInfo::GetTimeshiftProgressEpgEnd() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_playingEpgTag)
  {
    time_t epgEnd = 0;
    m_playingEpgTag->EndAsUTC().GetAsTime(epgEnd);
    return std::lrintf(static_cast<float>(epgEnd - m_iTimeshiftProgressStartTime) / m_iTimeshiftProgressDuration * 100);
  }
  return 0;
}

int CPVRGUITimesInfo::GetTimeshiftProgressBufferStart() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::lrintf(static_cast<float>(m_iTimeshiftStartTime - m_iTimeshiftProgressStartTime) / m_iTimeshiftProgressDuration * 100);
}

int CPVRGUITimesInfo::GetTimeshiftProgressBufferEnd() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::lrintf(static_cast<float>(m_iTimeshiftEndTime - m_iTimeshiftProgressStartTime) / m_iTimeshiftProgressDuration * 100);
}

int CPVRGUITimesInfo::GetEpgEventDuration(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (epgTag && m_playingEpgTag && *epgTag != *m_playingEpgTag)
    return epgTag->GetDuration();
  else
    return m_iDuration;
}

int CPVRGUITimesInfo::GetEpgEventProgress(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (epgTag && m_playingEpgTag && *epgTag != *m_playingEpgTag)
    return std::lrintf(epgTag->ProgressPercentage());
  else
    return std::lrintf(static_cast<float>(GetElapsedTime()) / m_iDuration * 100);
}

bool CPVRGUITimesInfo::IsTimeshifting() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return (m_iTimeshiftOffset > static_cast<unsigned int>(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeshiftThreshold));
}
