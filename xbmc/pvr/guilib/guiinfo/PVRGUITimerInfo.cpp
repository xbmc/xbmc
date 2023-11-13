/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUITimerInfo.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;

CPVRGUITimerInfo::CPVRGUITimerInfo()
{
  ResetProperties();
}

void CPVRGUITimerInfo::ResetProperties()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_strActiveTimerTitle.clear();
  m_strActiveTimerChannelName.clear();
  m_strActiveTimerChannelIcon.clear();
  m_strActiveTimerTime.clear();
  m_strNextTimerInfo.clear();
  m_strNextRecordingTitle.clear();
  m_strNextRecordingChannelName.clear();
  m_strNextRecordingChannelIcon.clear();
  m_strNextRecordingTime.clear();
  m_iTimerAmount = 0;
  m_iRecordingTimerAmount = 0;
  m_iTimerInfoToggleStart = {};
  m_iTimerInfoToggleCurrent = 0;
}

bool CPVRGUITimerInfo::TimerInfoToggle()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_iTimerInfoToggleStart.time_since_epoch().count() == 0)
  {
    m_iTimerInfoToggleStart = std::chrono::steady_clock::now();
    m_iTimerInfoToggleCurrent = 0;
    return true;
  }

  auto now = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_iTimerInfoToggleStart);

  if (duration.count() >
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRInfoToggleInterval)
  {
    unsigned int iPrevious = m_iTimerInfoToggleCurrent;
    unsigned int iBoundary = m_iRecordingTimerAmount > 0 ? m_iRecordingTimerAmount : m_iTimerAmount;
    if (++m_iTimerInfoToggleCurrent > iBoundary - 1)
      m_iTimerInfoToggleCurrent = 0;

    if (m_iTimerInfoToggleCurrent != iPrevious)
    {
      m_iTimerInfoToggleStart = std::chrono::steady_clock::now();
      return true;
    }
  }

  return false;
}

void CPVRGUITimerInfo::UpdateTimersToggle()
{
  if (!TimerInfoToggle())
    return;

  std::string strActiveTimerTitle;
  std::string strActiveTimerChannelName;
  std::string strActiveTimerChannelIcon;
  std::string strActiveTimerTime;

  /* safe to fetch these unlocked, since they're updated from the same thread as this one */
  if (m_iRecordingTimerAmount > 0)
  {
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> activeTags = GetActiveRecordings();
    if (m_iTimerInfoToggleCurrent < activeTags.size())
    {
      const std::shared_ptr<const CPVRTimerInfoTag> tag = activeTags.at(m_iTimerInfoToggleCurrent);
      strActiveTimerTitle = tag->Title();
      strActiveTimerChannelName = tag->ChannelName();
      strActiveTimerChannelIcon = tag->ChannelIcon();
      strActiveTimerTime = tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false);
    }
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_strActiveTimerTitle = strActiveTimerTitle;
  m_strActiveTimerChannelName = strActiveTimerChannelName;
  m_strActiveTimerChannelIcon = strActiveTimerChannelIcon;
  m_strActiveTimerTime = strActiveTimerTime;
}

void CPVRGUITimerInfo::UpdateTimersCache()
{
  int iTimerAmount = AmountActiveTimers();
  int iRecordingTimerAmount = AmountActiveRecordings();

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_iTimerAmount = iTimerAmount;
    m_iRecordingTimerAmount = iRecordingTimerAmount;
    m_iTimerInfoToggleStart = {};
  }

  UpdateTimersToggle();
}

void CPVRGUITimerInfo::UpdateNextTimer()
{
  std::string strNextRecordingTitle;
  std::string strNextRecordingChannelName;
  std::string strNextRecordingChannelIcon;
  std::string strNextRecordingTime;
  std::string strNextTimerInfo;

  const std::shared_ptr<const CPVRTimerInfoTag> timer = GetNextActiveTimer();
  if (timer)
  {
    strNextRecordingTitle = timer->Title();
    strNextRecordingChannelName = timer->ChannelName();
    strNextRecordingChannelIcon = timer->ChannelIcon();
    strNextRecordingTime = timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false);

    strNextTimerInfo = StringUtils::Format("{} {} {} {}", g_localizeStrings.Get(19106),
                                           timer->StartAsLocalTime().GetAsLocalizedDate(true),
                                           g_localizeStrings.Get(19107),
                                           timer->StartAsLocalTime().GetAsLocalizedTime("", false));
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_strNextRecordingTitle = strNextRecordingTitle;
  m_strNextRecordingChannelName = strNextRecordingChannelName;
  m_strNextRecordingChannelIcon = strNextRecordingChannelIcon;
  m_strNextRecordingTime = strNextRecordingTime;
  m_strNextTimerInfo = strNextTimerInfo;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerTitle() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strActiveTimerTitle;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerChannelName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strActiveTimerChannelName;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerChannelIcon() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strActiveTimerChannelIcon;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerDateTime() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strActiveTimerTime;
}

const std::string& CPVRGUITimerInfo::GetNextTimerTitle() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strNextRecordingTitle;
}

const std::string& CPVRGUITimerInfo::GetNextTimerChannelName() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strNextRecordingChannelName;
}

const std::string& CPVRGUITimerInfo::GetNextTimerChannelIcon() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strNextRecordingChannelIcon;
}

const std::string& CPVRGUITimerInfo::GetNextTimerDateTime() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strNextRecordingTime;
}

const std::string& CPVRGUITimerInfo::GetNextTimer() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_strNextTimerInfo;
}

int CPVRGUIAnyTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTimers();
}

int CPVRGUIAnyTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRecordings();
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRGUIAnyTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
}

std::shared_ptr<CPVRTimerInfoTag> CPVRGUIAnyTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveTimer();
}

int CPVRGUITVTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTVTimers();
}

int CPVRGUITVTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveTVRecordings();
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRGUITVTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveTVRecordings();
}

std::shared_ptr<CPVRTimerInfoTag> CPVRGUITVTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveTVTimer();
}

int CPVRGUIRadioTimerInfo::AmountActiveTimers()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRadioTimers();
}

int CPVRGUIRadioTimerInfo::AmountActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->AmountActiveRadioRecordings();
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRGUIRadioTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRadioRecordings();
}

std::shared_ptr<CPVRTimerInfoTag> CPVRGUIRadioTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveRadioTimer();
}
