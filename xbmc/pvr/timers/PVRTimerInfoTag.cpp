/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "settings/GUISettings.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "settings/AdvancedSettings.h"

#include "PVRTimerInfoTag.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/PVREpgInfoTag.h"

CPVRTimerInfoTag::CPVRTimerInfoTag(void)
{
  m_strTitle           = "";
  m_strDirectory       = "/";
  m_strSummary         = "";
  m_bIsActive          = false;
  m_iClientId          = CPVRManager::GetClients()->GetFirstID();
  m_iClientIndex       = -1;
  m_iClientChannelUid  = -1;
  m_bIsRecording       = false;
  m_StartTime          = NULL;
  m_StopTime           = NULL;
  m_bIsRepeating       = false;
  m_FirstDay           = NULL;
  m_iWeekdays          = 0;
  m_iPriority          = -1;
  m_iLifetime          = -1;
  m_epgInfo            = NULL;
  m_channel            = NULL;
  m_strFileNameAndPath = "";
}

CPVRTimerInfoTag::CPVRTimerInfoTag(const PVR_TIMER &timer, unsigned int iClientId)
{
  m_strTitle           = timer.strTitle;
  m_strDirectory       = timer.strDirectory;
  m_strSummary         = "";
  m_bIsActive          = timer.bIsActive;
  m_iClientId          = iClientId;
  m_iClientIndex       = timer.iClientIndex;
  m_iClientChannelUid  = timer.iClientChannelUid;
  m_bIsRecording       = timer.bIsRecording;
  m_StartTime          = (time_t) (timer.startTime ? timer.startTime + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0);
  m_StopTime           = (time_t) (timer.endTime ? timer.endTime + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0);
  m_bIsRepeating       = timer.bIsRepeating;
  m_FirstDay           = (time_t) (timer.firstDay ? timer.firstDay + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0);
  m_iWeekdays          = timer.iWeekdays;
  m_iPriority          = timer.iPriority;
  m_iLifetime          = timer.iLifetime;
  m_epgInfo            = NULL;
  m_channel            = NULL;
  m_strFileNameAndPath.Format("pvr://client%i/timers/%i", m_iClientId, m_iClientIndex);

  UpdateSummary();
}

bool CPVRTimerInfoTag::operator ==(const CPVRTimerInfoTag& right) const
{
  if (this == &right) return true;

  bool bChannelsMatch = true;
  if (m_channel && right.m_channel)
    bChannelsMatch = *m_channel == *right.m_channel;
  else if (!m_channel && right.m_channel)
    bChannelsMatch = false;
  else if (m_channel && !right.m_channel)
    bChannelsMatch = false;

  return (m_iClientIndex       == right.m_iClientIndex &&
          m_bIsActive          == right.m_bIsActive &&
          m_strSummary         == right.m_strSummary &&
          m_iClientChannelUid  == right.m_iClientChannelUid &&
          m_bIsRepeating       == right.m_bIsRepeating &&
          m_StartTime          == right.m_StartTime &&
          m_StopTime           == right.m_StopTime &&
          m_FirstDay           == right.m_FirstDay &&
          m_iWeekdays          == right.m_iWeekdays &&
          m_bIsRecording       == right.m_bIsRecording &&
          m_iPriority          == right.m_iPriority &&
          m_iLifetime          == right.m_iLifetime &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_strTitle           == right.m_strTitle &&
          m_iClientId          == right.m_iClientId &&
          bChannelsMatch);
}

/**
 * Compare not equal for two CPVRTimerInfoTag
 */
bool CPVRTimerInfoTag::operator !=(const CPVRTimerInfoTag& right) const
{
  if (this == &right) return false;

  return !(*this == right);
}

int CPVRTimerInfoTag::Compare(const CPVRTimerInfoTag &timer) const
{
  int iTimerDelta = StartTime() - timer.StartTime();

  /* if the start times are equal, compare the priority of the timers */
  return iTimerDelta == 0 ?
    timer.m_iPriority - m_iPriority :
    iTimerDelta;
}

time_t CPVRTimerInfoTag::StartTime(void) const
{
  time_t start;
  m_StartTime.GetAsTime(start);
  return start;
}

time_t CPVRTimerInfoTag::StopTime(void) const
{
  time_t stop;
  m_StopTime.GetAsTime(stop);
  return stop;
}

time_t CPVRTimerInfoTag::FirstDayTime(void) const
{
  time_t firstday;
  m_FirstDay.GetAsTime(firstday);
  return firstday;
}

void CPVRTimerInfoTag::UpdateSummary(void)
{
  m_strSummary.clear();

  if (!m_bIsRepeating)
  {
    m_strSummary.Format("%s %s %s %s %s",
        m_StartTime.GetAsLocalizedDate(),
        g_localizeStrings.Get(19159),
        m_StartTime.GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        m_StopTime.GetAsLocalizedTime("", false));
  }
  else if (m_FirstDay != NULL)
  {
    m_strSummary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s",
        m_iWeekdays & 0x01 ? g_localizeStrings.Get(19149) : "__",
        m_iWeekdays & 0x02 ? g_localizeStrings.Get(19150) : "__",
        m_iWeekdays & 0x04 ? g_localizeStrings.Get(19151) : "__",
        m_iWeekdays & 0x08 ? g_localizeStrings.Get(19152) : "__",
        m_iWeekdays & 0x10 ? g_localizeStrings.Get(19153) : "__",
        m_iWeekdays & 0x20 ? g_localizeStrings.Get(19154) : "__",
        m_iWeekdays & 0x40 ? g_localizeStrings.Get(19155) : "__",
        g_localizeStrings.Get(19156),
        m_FirstDay.GetAsLocalizedDate(false),
        g_localizeStrings.Get(19159),
        m_StartTime.GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        m_StopTime.GetAsLocalizedTime("", false));
  }
  else
  {
    m_strSummary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s",
        m_iWeekdays & 0x01 ? g_localizeStrings.Get(19149) : "__",
        m_iWeekdays & 0x02 ? g_localizeStrings.Get(19150) : "__",
        m_iWeekdays & 0x04 ? g_localizeStrings.Get(19151) : "__",
        m_iWeekdays & 0x08 ? g_localizeStrings.Get(19152) : "__",
        m_iWeekdays & 0x10 ? g_localizeStrings.Get(19153) : "__",
        m_iWeekdays & 0x20 ? g_localizeStrings.Get(19154) : "__",
        m_iWeekdays & 0x40 ? g_localizeStrings.Get(19155) : "__",
        g_localizeStrings.Get(19159),
        m_StartTime.GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        m_StopTime.GetAsLocalizedTime("", false));
  }
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
const CStdString &CPVRTimerInfoTag::GetStatus() const
{
  if (m_strFileNameAndPath == "pvr://timers/add.timer")
    return g_localizeStrings.Get(19026);
  else if (!m_bIsActive)
    return g_localizeStrings.Get(13106);
  else if (m_bIsActive && (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime()))
    return g_localizeStrings.Get(19162);
  else
    return g_localizeStrings.Get(305);
}

bool CPVRTimerInfoTag::AddToClient(void)
{
  UpdateEpgEvent();
  PVR_ERROR error;
  if (!CPVRManager::GetClients()->AddTimer(*this, &error))
  {
    DisplayError(error);
    return false;
  }
  else
  {
    if (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime())
      CPVRManager::Get()->TriggerRecordingsUpdate();
    return true;
  }
}

bool CPVRTimerInfoTag::DeleteFromClient(bool bForce /* = false */)
{
  bool bRemoved = false;
  PVR_ERROR error;

  bRemoved = CPVRManager::GetClients()->DeleteTimer(*this, bForce, &error);
  if (!bRemoved && error == PVR_ERROR_RECORDING_RUNNING)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(122,0,19122,0))
      bRemoved = CPVRManager::GetClients()->DeleteTimer(*this, true, &error);
    else
      return false;
  }

  if (!bRemoved)
  {
    DisplayError(error);
    return false;
  }

  CPVRManager::Get()->TriggerRecordingsUpdate();
  return true;
}

bool CPVRTimerInfoTag::RenameOnClient(const CStdString &strNewName)
{
  PVR_ERROR error;
  m_strTitle.Format("%s", strNewName);
  if (!CPVRManager::GetClients()->RenameTimer(*this, m_strTitle, &error))
  {
    if (error == PVR_ERROR_NOT_IMPLEMENTED)
      return UpdateOnClient();

    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRTimerInfoTag::UpdateEntry(const CPVRTimerInfoTag &tag)
{
  if (m_epgInfo)
  {
    m_epgInfo->SetTimer(NULL);
    m_epgInfo = NULL;
  }

  m_iClientId         = tag.m_iClientId;
  m_iClientIndex      = tag.m_iClientIndex;
  m_bIsActive         = tag.m_bIsActive;
  m_strTitle          = tag.m_strTitle;
  m_strDirectory      = tag.m_strDirectory;
  m_iClientChannelUid = tag.m_iClientChannelUid;
  m_StartTime         = tag.m_StartTime;
  m_StopTime          = tag.m_StopTime;
  m_FirstDay          = tag.m_FirstDay;
  m_iPriority         = tag.m_iPriority;
  m_iLifetime         = tag.m_iLifetime;
  m_bIsRecording      = tag.m_bIsRecording;
  m_bIsRepeating      = tag.m_bIsRepeating;
  m_iWeekdays         = tag.m_iWeekdays;
  m_iChannelNumber    = tag.m_iChannelNumber;
  m_bIsRadio          = tag.m_bIsRadio;

  /* try to find an epg event */
  UpdateEpgEvent();

  return true;
}

void CPVRTimerInfoTag::UpdateEpgEvent(bool bClear /* = false */)
{
  /* try to get the channel */
  CPVRChannel *channel = (CPVRChannel *) CPVRManager::GetChannelGroups()->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (!channel)
    return;

  /* try to get the EPG table */
  CPVREpg *epg = channel->GetEPG();
  if (!epg)
    return;

  /* try to set the timer on the epg tag that matches */
  m_epgInfo = (CPVREpgInfoTag *) epg->InfoTagBetween(m_StartTime, m_StopTime);
  if (!m_epgInfo)
    m_epgInfo = (CPVREpgInfoTag *) epg->InfoTagAround(m_StartTime);

  if (m_epgInfo)
    m_epgInfo->SetTimer(bClear ? NULL : this);
}

bool CPVRTimerInfoTag::UpdateOnClient()
{
  UpdateEpgEvent();
  PVR_ERROR error;
  if (!CPVRManager::GetClients()->UpdateTimer(*this, &error))
  {
    DisplayError(error);
    return false;
  }
  else
  {
    if (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime())
      CPVRManager::Get()->TriggerRecordingsUpdate();
    return true;
  }
}

void CPVRTimerInfoTag::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(19033,19111,19110,0); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_NOT_SYNC)
    CGUIDialogOK::ShowAndGetInput(19033,19112,19110,0); /* print info dialog "Timers not in sync!" */
  else if (err == PVR_ERROR_NOT_SAVED)
    CGUIDialogOK::ShowAndGetInput(19033,19109,19110,0); /* print info dialog "Couldn't delete timer!" */
  else if (err == PVR_ERROR_ALREADY_PRESENT)
    CGUIDialogOK::ShowAndGetInput(19033,19109,0,19067); /* print info dialog */
  else
    CGUIDialogOK::ShowAndGetInput(19033,19147,19110,0); /* print info dialog "Unknown error!" */

  return;
}

void CPVRTimerInfoTag::SetEpgInfoTag(CPVREpgInfoTag *tag)
{
  if (m_epgInfo != tag)
  {
    if (tag)
      CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to epg event %s", m_strTitle.c_str(), tag->Title().c_str());
    else
      CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to no epg event", m_strTitle.c_str());
    m_epgInfo = tag;
  }
}

int CPVRTimerInfoTag::ChannelNumber() const
{
  const CPVRChannel *channeltag = CPVRManager::GetChannelGroups()->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (channeltag)
    return channeltag->ChannelNumber();
  else
    return 0;
}

CStdString CPVRTimerInfoTag::ChannelName() const
{
  const CPVRChannel *channeltag = CPVRManager::GetChannelGroups()->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (channeltag)
    return channeltag->ChannelName();
  else
    return "";
}

bool CPVRTimerInfoTag::SetDuration(int iDuration)
{
  if (m_StartTime != NULL)
  {
    m_StopTime = m_StartTime + CDateTimeSpan(0, iDuration / 60, iDuration % 60, 0);
    return true;
  }

  return false;
}

CPVRTimerInfoTag *CPVRTimerInfoTag::CreateFromEpg(const CPVREpgInfoTag &tag)
{
  /* create a new timer */
  CPVRTimerInfoTag *newTag = new CPVRTimerInfoTag();
  if (!newTag)
  {
    CLog::Log(LOGERROR, "%s - couldn't create new timer", __FUNCTION__);
    return NULL;
  }

  /* check if a valid channel is set */
  const CPVRChannel *channel = tag.ChannelTag();
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "%s - no channel set", __FUNCTION__);
    return NULL;
  }

  /* check if the epg end date is in the future */
  if (tag.End() < CDateTime::GetCurrentDateTime())
  {
    CLog::Log(LOGERROR, "%s - end time is in the past", __FUNCTION__);
    return NULL;
  }

  int iPriority    = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  if (!iPriority)
    iPriority      = 50; /* default to 50 */

  int iLifetime    = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
  if (!iLifetime)
    iLifetime      = 30; /* default to 30 days */

  int iMarginStart = g_guiSettings.GetInt("pvrrecord.marginstart");
  if (!iMarginStart)
    iMarginStart   = 5;  /* default to 5 minutes */

  int iMarginStop  = g_guiSettings.GetInt("pvrrecord.marginstop");
  if (!iMarginStop)
    iMarginStop    = 10; /* default to 10 minutes */

  /* set the timer data */
  newTag->m_iClientIndex      = (tag.UniqueBroadcastID() > 0 ? tag.UniqueBroadcastID() : channel->ClientID());
  newTag->m_bIsActive         = true;
  newTag->m_strTitle          = tag.Title().IsEmpty() ? channel->ChannelName() : tag.Title();
  newTag->m_iChannelNumber    = channel->ChannelNumber();
  newTag->m_iClientChannelUid = channel->UniqueID();
  newTag->m_iClientId         = channel->ClientID();
  newTag->m_bIsRadio          = channel->IsRadio();
  newTag->m_StartTime         = tag.Start() - CDateTimeSpan(0, iMarginStart / 60, iMarginStart % 60, 0);
  newTag->m_StopTime          = tag.End() + CDateTimeSpan(0, iMarginStop / 60, iMarginStop % 60, 0);
  newTag->m_iPriority         = iPriority;
  newTag->m_iLifetime         = iLifetime;

  /* generate summary string */
  newTag->m_strSummary.Format("%s %s %s %s %s",
      newTag->m_StartTime.GetAsLocalizedDate(),
      g_localizeStrings.Get(19159),
      newTag->m_StartTime.GetAsLocalizedTime("", false),
      g_localizeStrings.Get(19160),
      newTag->m_StopTime.GetAsLocalizedTime("", false));

  /* unused only for reference */
  newTag->m_strFileNameAndPath = "pvr://timers/new";

  return newTag;
}
