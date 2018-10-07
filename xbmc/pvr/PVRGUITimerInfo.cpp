/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUITimerInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;

CPVRGUITimerInfo::CPVRGUITimerInfo()
{
  ResetProperties();
}

void CPVRGUITimerInfo::ResetProperties()
{
  CSingleLock lock(m_critSection);
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
  m_iTimerInfoToggleStart = 0;
  m_iTimerInfoToggleCurrent = 0;
}

bool CPVRGUITimerInfo::TimerInfoToggle()
{
  CSingleLock lock(m_critSection);
  if (m_iTimerInfoToggleStart == 0)
  {
    m_iTimerInfoToggleStart = XbmcThreads::SystemClockMillis();
    m_iTimerInfoToggleCurrent = 0;
    return true;
  }

  if ((int) (XbmcThreads::SystemClockMillis() - m_iTimerInfoToggleStart) > CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRInfoToggleInterval)
  {
    unsigned int iPrevious = m_iTimerInfoToggleCurrent;
    unsigned int iBoundary = m_iRecordingTimerAmount > 0 ? m_iRecordingTimerAmount : m_iTimerAmount;
    if (++m_iTimerInfoToggleCurrent > iBoundary - 1)
      m_iTimerInfoToggleCurrent = 0;

    return m_iTimerInfoToggleCurrent != iPrevious;
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
    std::vector<CFileItemPtr> activeTags = GetActiveRecordings();
    if (m_iTimerInfoToggleCurrent < activeTags.size() && activeTags.at(m_iTimerInfoToggleCurrent)->HasPVRTimerInfoTag())
    {
      CPVRTimerInfoTagPtr tag = activeTags.at(m_iTimerInfoToggleCurrent)->GetPVRTimerInfoTag();
      strActiveTimerTitle = StringUtils::Format("%s", tag->Title().c_str());
      strActiveTimerChannelName = StringUtils::Format("%s", tag->ChannelName().c_str());
      strActiveTimerChannelIcon = StringUtils::Format("%s", tag->ChannelIcon().c_str());
      strActiveTimerTime = StringUtils::Format("%s", tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());
    }
  }

  CSingleLock lock(m_critSection);
  m_strActiveTimerTitle = strActiveTimerTitle;
  m_strActiveTimerChannelName = strActiveTimerChannelName;
  m_strActiveTimerChannelIcon = strActiveTimerChannelIcon;
  m_strActiveTimerTime = strActiveTimerTime;
}

void CPVRGUITimerInfo::UpdateTimersCache(void)
{
  int iTimerAmount = AmountActiveTimers();
  int iRecordingTimerAmount = AmountActiveRecordings();

  {
    CSingleLock lock(m_critSection);
    m_iTimerAmount = iTimerAmount;
    m_iRecordingTimerAmount = iRecordingTimerAmount;
    m_iTimerInfoToggleStart = 0;
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

  CFileItemPtr tag = GetNextActiveTimer();
  if (tag && tag->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr timer = tag->GetPVRTimerInfoTag();
    strNextRecordingTitle = StringUtils::Format("%s", timer->Title().c_str());
    strNextRecordingChannelName = StringUtils::Format("%s", timer->ChannelName().c_str());
    strNextRecordingChannelIcon = StringUtils::Format("%s", timer->ChannelIcon().c_str());
    strNextRecordingTime = StringUtils::Format("%s", timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());

    strNextTimerInfo = StringUtils::Format("%s %s %s %s",
        g_localizeStrings.Get(19106).c_str(),
        timer->StartAsLocalTime().GetAsLocalizedDate(true).c_str(),
        g_localizeStrings.Get(19107).c_str(),
        timer->StartAsLocalTime().GetAsLocalizedTime("HH:mm", false).c_str());
  }

  CSingleLock lock(m_critSection);
  m_strNextRecordingTitle = strNextRecordingTitle;
  m_strNextRecordingChannelName = strNextRecordingChannelName;
  m_strNextRecordingChannelIcon = strNextRecordingChannelIcon;
  m_strNextRecordingTime = strNextRecordingTime;
  m_strNextTimerInfo = strNextTimerInfo;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerTitle() const
{
  CSingleLock lock(m_critSection);
  return m_strActiveTimerTitle;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerChannelName() const
{
  CSingleLock lock(m_critSection);
  return m_strActiveTimerChannelName;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerChannelIcon() const
{
  CSingleLock lock(m_critSection);
  return m_strActiveTimerChannelIcon;
}

const std::string& CPVRGUITimerInfo::GetActiveTimerDateTime() const
{
  CSingleLock lock(m_critSection);
  return m_strActiveTimerTime;
}

const std::string& CPVRGUITimerInfo::GetNextTimerTitle() const
{
  CSingleLock lock(m_critSection);
  return m_strNextRecordingTitle;
}

const std::string& CPVRGUITimerInfo::GetNextTimerChannelName() const
{
  CSingleLock lock(m_critSection);
  return m_strNextRecordingChannelName;
}

const std::string& CPVRGUITimerInfo::GetNextTimerChannelIcon() const
{
  CSingleLock lock(m_critSection);
  return m_strNextRecordingChannelIcon;
}

const std::string& CPVRGUITimerInfo::GetNextTimerDateTime() const
{
  CSingleLock lock(m_critSection);
  return m_strNextRecordingTime;
}

const std::string& CPVRGUITimerInfo::GetNextTimer() const
{
  CSingleLock lock(m_critSection);
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

std::vector<CFileItemPtr> CPVRGUIAnyTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
}

CFileItemPtr CPVRGUIAnyTimerInfo::GetNextActiveTimer()
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

std::vector<CFileItemPtr> CPVRGUITVTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveTVRecordings();
}

CFileItemPtr CPVRGUITVTimerInfo::GetNextActiveTimer()
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

std::vector<CFileItemPtr> CPVRGUIRadioTimerInfo::GetActiveRecordings()
{
  return CServiceBroker::GetPVRManager().Timers()->GetActiveRadioRecordings();
}

CFileItemPtr CPVRGUIRadioTimerInfo::GetNextActiveTimer()
{
  return CServiceBroker::GetPVRManager().Timers()->GetNextActiveRadioTimer();
}
