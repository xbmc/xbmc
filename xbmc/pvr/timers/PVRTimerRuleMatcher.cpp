/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimerRuleMatcher.h"

#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h" // PVR_CHANNEL_INVALID_UID
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "utils/RegExp.h"

#include <memory>

using namespace PVR;

CPVRTimerRuleMatcher::CPVRTimerRuleMatcher(const std::shared_ptr<CPVRTimerInfoTag>& timerRule,
                                           const CDateTime& start)
  : m_timerRule(timerRule), m_start(CPVRTimerInfoTag::ConvertUTCToLocalTime(start))
{
}

CPVRTimerRuleMatcher::~CPVRTimerRuleMatcher() = default;

std::shared_ptr<const CPVRChannel> CPVRTimerRuleMatcher::GetChannel() const
{
  if (m_timerRule->GetTimerType()->SupportsChannels())
    return m_timerRule->Channel();

  return {};
}

CDateTime CPVRTimerRuleMatcher::GetNextTimerStart() const
{
  if (!m_timerRule->GetTimerType()->SupportsStartTime())
    return CDateTime(); // invalid datetime

  const CDateTime startDateLocal = m_timerRule->GetTimerType()->SupportsFirstDay()
                                       ? m_timerRule->FirstDayAsLocalTime()
                                       : m_start;
  const CDateTime startTimeLocal = m_timerRule->StartAsLocalTime();
  CDateTime nextStart(startDateLocal.GetYear(), startDateLocal.GetMonth(), startDateLocal.GetDay(),
                      startTimeLocal.GetHour(), startTimeLocal.GetMinute(), 0);

  const CDateTimeSpan oneDay(1, 0, 0, 0);
  while (nextStart < m_start)
  {
    nextStart += oneDay;
  }

  if (m_timerRule->GetTimerType()->SupportsWeekdays() &&
      m_timerRule->WeekDays() != PVR_WEEKDAY_ALLDAYS)
  {
    bool bMatch = false;
    while (!bMatch)
    {
      int startWeekday = nextStart.GetDayOfWeek();
      if (startWeekday == 0)
        startWeekday = 7;

      bMatch = ((1 << (startWeekday - 1)) & m_timerRule->WeekDays());
      if (!bMatch)
        nextStart += oneDay;
    }
  }

  return nextStart.GetAsUTCDateTime();
}

bool CPVRTimerRuleMatcher::Matches(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  return epgTag && CPVRTimerInfoTag::ConvertUTCToLocalTime(epgTag->EndAsUTC()) > m_start &&
         MatchSeriesLink(epgTag) && MatchChannel(epgTag) && MatchStart(epgTag) &&
         MatchEnd(epgTag) && MatchDayOfWeek(epgTag) && MatchSearchText(epgTag);
}

bool CPVRTimerRuleMatcher::MatchSeriesLink(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->RequiresEpgSeriesLinkOnCreate())
    return epgTag->SeriesLink() == m_timerRule->SeriesLink();
  else
    return true;
}

bool CPVRTimerRuleMatcher::MatchChannel(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->SupportsAnyChannel() &&
      m_timerRule->ClientChannelUID() == PVR_CHANNEL_INVALID_UID)
    return true; // matches any channel

  if (m_timerRule->GetTimerType()->SupportsChannels())
    return m_timerRule->ClientChannelUID() != PVR_CHANNEL_INVALID_UID &&
           epgTag->ClientID() == m_timerRule->ClientID() &&
           epgTag->UniqueChannelID() == m_timerRule->ClientChannelUID();
  else
    return true;
}

bool CPVRTimerRuleMatcher::MatchStart(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->SupportsFirstDay())
  {
    // only year, month and day do matter here...
    const CDateTime startEpgLocal = CPVRTimerInfoTag::ConvertUTCToLocalTime(epgTag->StartAsUTC());
    const CDateTime startEpg(startEpgLocal.GetYear(), startEpgLocal.GetMonth(),
                             startEpgLocal.GetDay(), 0, 0, 0);
    const CDateTime firstDayLocal = m_timerRule->FirstDayAsLocalTime();
    const CDateTime startTimer(firstDayLocal.GetYear(), firstDayLocal.GetMonth(),
                               firstDayLocal.GetDay(), 0, 0, 0);
    if (startEpg < startTimer)
      return false;
  }

  if (m_timerRule->GetTimerType()->SupportsStartAnyTime() && m_timerRule->IsStartAnyTime())
    return true; // matches any start time

  if (m_timerRule->GetTimerType()->SupportsStartTime())
  {
    // only hours and minutes do matter here...
    const CDateTime startEpgLocal = CPVRTimerInfoTag::ConvertUTCToLocalTime(epgTag->StartAsUTC());
    const CDateTime startEpg(2000, 1, 1, startEpgLocal.GetHour(), startEpgLocal.GetMinute(), 0);
    const CDateTime startTimerLocal = m_timerRule->StartAsLocalTime();
    const CDateTime startTimer(2000, 1, 1, startTimerLocal.GetHour(), startTimerLocal.GetMinute(),
                               0);
    return startEpg >= startTimer;
  }
  else
    return true;
}

bool CPVRTimerRuleMatcher::MatchEnd(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->SupportsEndAnyTime() && m_timerRule->IsEndAnyTime())
    return true; // matches any end time

  if (m_timerRule->GetTimerType()->SupportsEndTime())
  {
    // only hours and minutes do matter here...
    const CDateTime endEpgLocal = CPVRTimerInfoTag::ConvertUTCToLocalTime(epgTag->EndAsUTC());
    const CDateTime endEpg(2000, 1, 1, endEpgLocal.GetHour(), endEpgLocal.GetMinute(), 0);
    const CDateTime endTimerLocal = m_timerRule->EndAsLocalTime();
    const CDateTime endTimer(2000, 1, 1, endTimerLocal.GetHour(), endTimerLocal.GetMinute(), 0);
    return endEpg <= endTimer;
  }
  else
    return true;
}

bool CPVRTimerRuleMatcher::MatchDayOfWeek(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->SupportsWeekdays())
  {
    if (m_timerRule->WeekDays() != PVR_WEEKDAY_ALLDAYS)
    {
      const CDateTime startEpgLocal = CPVRTimerInfoTag::ConvertUTCToLocalTime(epgTag->StartAsUTC());
      int startWeekday = startEpgLocal.GetDayOfWeek();
      if (startWeekday == 0)
        startWeekday = 7;

      return ((1 << (startWeekday - 1)) & m_timerRule->WeekDays());
    }
  }
  return true;
}

bool CPVRTimerRuleMatcher::MatchSearchText(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (m_timerRule->GetTimerType()->SupportsEpgFulltextMatch() && m_timerRule->IsFullTextEpgSearch())
  {
    if (!m_textSearch)
    {
      m_textSearch = std::make_unique<CRegExp>(true /* case insensitive */);
      m_textSearch->RegComp(m_timerRule->EpgSearchString());
    }
    return m_textSearch->RegFind(epgTag->Title()) >= 0 ||
           m_textSearch->RegFind(epgTag->EpisodeName()) >= 0 ||
           m_textSearch->RegFind(epgTag->PlotOutline()) >= 0 ||
           m_textSearch->RegFind(epgTag->Plot()) >= 0;
  }
  else if (m_timerRule->GetTimerType()->SupportsEpgTitleMatch())
  {
    if (!m_textSearch)
    {
      m_textSearch = std::make_unique<CRegExp>(true /* case insensitive */);
      m_textSearch->RegComp(m_timerRule->EpgSearchString());
    }
    return m_textSearch->RegFind(epgTag->Title()) >= 0;
  }
  else
    return true;
}
