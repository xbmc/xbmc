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
#include "utils/log.h"

#include "PVRTimers.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/PVREpgContainer.h"
#include "pvr/addons/PVRClients.h"

#include "epg/Epg.h"

using namespace PVR;
using namespace EPG;

CPVRTimerInfoTag::CPVRTimerInfoTag(void)
{
  m_strTitle           = "";
  m_strDirectory       = "/";
  m_strSummary         = "";
  m_iClientId          = g_PVRClients->GetFirstID();
  m_iClientIndex       = -1;
  m_iClientChannelUid  = -1;
  m_iPriority          = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  m_iLifetime          = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
  m_bIsRepeating       = false;
  m_iWeekdays          = 0;
  m_strFileNameAndPath = "";
  m_iChannelNumber     = 0;
  m_bIsRadio           = false;
  m_epgInfo            = NULL;
  m_channel            = NULL;
  m_iMarginStart       = g_guiSettings.GetInt("pvrrecord.marginstart");
  m_iMarginEnd         = g_guiSettings.GetInt("pvrrecord.marginend");
  m_strGenre           = "";
  m_StartTime          = CDateTime::GetUTCDateTime();
  m_StopTime           = m_StartTime;
  m_state              = PVR_TIMER_STATE_SCHEDULED;
  m_FirstDay.SetValid(false);
}

CPVRTimerInfoTag::CPVRTimerInfoTag(const PVR_TIMER &timer, CPVRChannel *channel, unsigned int iClientId)
{
  m_strTitle           = timer.strTitle;
  m_strDirectory       = timer.strDirectory;
  m_strSummary         = "";
  m_iClientId          = iClientId;
  m_iClientIndex       = timer.iClientIndex;
  m_iClientChannelUid  = channel ? channel->UniqueID() : timer.iClientChannelUid;
  m_iChannelNumber     = channel ? g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->GetChannelNumber(*channel) : 0;
  m_StartTime          = timer.startTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_StopTime           = timer.endTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_bIsRepeating       = timer.bIsRepeating;
  m_FirstDay           = timer.firstDay + g_advancedSettings.m_iPVRTimeCorrection;
  m_iWeekdays          = timer.iWeekdays;
  m_iPriority          = timer.iPriority;
  m_iLifetime          = timer.iLifetime;
  m_iMarginStart       = timer.iMarginStart;
  m_iMarginEnd         = timer.iMarginEnd;
  m_strGenre           = CEpg::ConvertGenreIdToString(timer.iGenreType, timer.iGenreSubType);
  m_epgInfo            = NULL;
  m_channel            = channel;
  m_bIsRadio           = channel && channel->IsRadio();
  m_state              = timer.state;
  m_strFileNameAndPath.Format("pvr://client%i/timers/%i", m_iClientId, m_iClientIndex);

  if (timer.iEpgUid > 0)
  {
    m_epgInfo = (CPVREpgInfoTag *) channel->GetEPG()->GetTag(timer.iEpgUid, m_StartTime);
    if (m_epgInfo)
      m_strGenre = m_epgInfo->Genre();
  }

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
          m_strSummary         == right.m_strSummary &&
          m_iClientChannelUid  == right.m_iClientChannelUid &&
          m_bIsRepeating       == right.m_bIsRepeating &&
          m_StartTime          == right.m_StartTime &&
          m_StopTime           == right.m_StopTime &&
          m_FirstDay           == right.m_FirstDay &&
          m_iWeekdays          == right.m_iWeekdays &&
          m_iPriority          == right.m_iPriority &&
          m_iLifetime          == right.m_iLifetime &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_strTitle           == right.m_strTitle &&
          m_iClientId          == right.m_iClientId &&
          m_iMarginStart       == right.m_iMarginStart &&
          m_iMarginEnd         == right.m_iMarginEnd &&
          m_state              == right.m_state &&
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
  int iTimerDelta = 0;
  if (StartAsUTC() != timer.StartAsUTC())
  {
    CDateTimeSpan timerDelta = StartAsUTC() - timer.StartAsUTC();
    iTimerDelta = (timerDelta.GetSeconds() + timerDelta.GetMinutes() * 60 + timerDelta.GetHours() * 3600 + timerDelta.GetDays() * 86400);
  }

  /* if the start times are equal, compare the priority of the timers */
  return iTimerDelta == 0 ?
    timer.m_iPriority - m_iPriority :
    iTimerDelta;
}

void CPVRTimerInfoTag::UpdateSummary(void)
{
  m_strSummary.clear();

  if (!m_bIsRepeating)
  {
    m_strSummary.Format("%s %s %s %s %s",
        StartAsLocalTime().GetAsLocalizedDate(),
        g_localizeStrings.Get(19159),
        StartAsLocalTime().GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        EndAsLocalTime().GetAsLocalizedTime("", false));
  }
  else if (m_FirstDay.IsValid())
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
        FirstDayAsLocalTime().GetAsLocalizedDate(false),
        g_localizeStrings.Get(19159),
        StartAsLocalTime().GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        EndAsLocalTime().GetAsLocalizedTime("", false));
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
        StartAsLocalTime().GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        EndAsLocalTime().GetAsLocalizedTime("", false));
  }
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
const CStdString &CPVRTimerInfoTag::GetStatus() const
{
  if (m_strFileNameAndPath == "pvr://timers/add.timer")
    return g_localizeStrings.Get(19026);
  else if (m_state == PVR_TIMER_STATE_CANCELLED || m_state == PVR_TIMER_STATE_ABORTED)
    return g_localizeStrings.Get(13106);
  else if (m_state == PVR_TIMER_STATE_RECORDING)
    return g_localizeStrings.Get(19162);
  else
    return g_localizeStrings.Get(305);
}

bool CPVRTimerInfoTag::AddToClient(void)
{
  UpdateEpgEvent();
  PVR_ERROR error;
  if (!g_PVRClients->AddTimer(*this, &error))
  {
    DisplayError(error);
    return false;
  }
  else
    return true;
}

bool CPVRTimerInfoTag::DeleteFromClient(bool bForce /* = false */)
{
  bool bRemoved = false;
  PVR_ERROR error;

  bRemoved = g_PVRClients->DeleteTimer(*this, bForce, &error);
  if (!bRemoved && error == PVR_ERROR_RECORDING_RUNNING)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(122,0,19122,0))
      bRemoved = g_PVRClients->DeleteTimer(*this, true, &error);
    else
      return false;
  }

  if (!bRemoved)
  {
    DisplayError(error);
    return false;
  }

  if (m_epgInfo)
  {
    m_epgInfo->SetTimer(NULL);
    m_epgInfo = NULL;
  }

  return true;
}

bool CPVRTimerInfoTag::RenameOnClient(const CStdString &strNewName)
{
  PVR_ERROR error;
  m_strTitle.Format("%s", strNewName);
  if (!g_PVRClients->RenameTimer(*this, m_strTitle, &error))
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
  m_strTitle          = tag.m_strTitle;
  m_strDirectory      = tag.m_strDirectory;
  m_iClientChannelUid = tag.m_iClientChannelUid;
  m_StartTime         = tag.m_StartTime;
  m_StopTime          = tag.m_StopTime;
  m_FirstDay          = tag.m_FirstDay;
  m_iPriority         = tag.m_iPriority;
  m_iLifetime         = tag.m_iLifetime;
  m_state             = tag.m_state;
  m_bIsRepeating      = tag.m_bIsRepeating;
  m_iWeekdays         = tag.m_iWeekdays;
  m_iChannelNumber    = tag.m_iChannelNumber;
  m_bIsRadio          = tag.m_bIsRadio;
  m_iMarginStart      = tag.m_iMarginStart;
  m_iMarginEnd        = tag.m_iMarginEnd;
  m_epgInfo           = tag.m_epgInfo;
  m_strGenre          = tag.m_strGenre;

  /* try to find an epg event */
  UpdateEpgEvent();
  if (m_epgInfo != NULL)
  {
    m_strGenre = m_epgInfo->Genre();
    m_epgInfo->SetTimer(this);
  }

  UpdateSummary();

  return true;
}

void CPVRTimerInfoTag::UpdateEpgEvent(bool bClear /* = false */)
{
  if (bClear)
  {
    if (m_epgInfo)
    {
      m_epgInfo->SetTimer(NULL);
      m_epgInfo = NULL;
    }
  }
  else
  {
    /* already got an epg event set */
    if (m_epgInfo)
      return;

    /* try to get the channel */
    CPVRChannel *channel = (CPVRChannel *) g_PVRChannelGroups->GetByUniqueID(m_iClientChannelUid, m_iClientId);
    if (!channel)
      return;

    /* try to get the EPG table */
    CPVREpg *epg = channel->GetEPG();
    if (!epg)
      return;

    /* try to set the timer on the epg tag that matches with a 2 minute margin */
    m_epgInfo = (CPVREpgInfoTag *) epg->GetTagBetween(StartAsLocalTime() - CDateTimeSpan(0, 0, 2, 0), EndAsLocalTime() + CDateTimeSpan(0, 0, 2, 0));
    if (!m_epgInfo)
      m_epgInfo = (CPVREpgInfoTag *) epg->GetTagAround(StartAsLocalTime());

    if (m_epgInfo)
      m_epgInfo->SetTimer(this);
  }
}

bool CPVRTimerInfoTag::UpdateOnClient()
{
  UpdateEpgEvent();
  PVR_ERROR error;
  if (!g_PVRClients->UpdateTimer(*this, &error))
  {
    DisplayError(error);
    return false;
  }
  else
    return true;
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
  const CPVRChannel *channeltag = g_PVRChannelGroups->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (channeltag)
    return channeltag->ChannelNumber();
  else
    return 0;
}

CStdString CPVRTimerInfoTag::ChannelName() const
{
  const CPVRChannel *channeltag = g_PVRChannelGroups->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (channeltag)
    return channeltag->ChannelName();
  else
    return "";
}

CStdString CPVRTimerInfoTag::ChannelIcon() const
{
  const CPVRChannel *channeltag = g_PVRChannelGroups->GetByUniqueID(m_iClientChannelUid, m_iClientId);
  if (channeltag)
    return channeltag->IconPath();
  else
    return "";
}

bool CPVRTimerInfoTag::SetDuration(int iDuration)
{
  if (m_StartTime.IsValid())
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
  CPVRChannel *channel = (CPVRChannel *) tag.ChannelTag();
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "%s - no channel set", __FUNCTION__);
    return NULL;
  }

  /* check if the epg end date is in the future */
  if (tag.EndAsLocalTime() < CDateTime::GetCurrentDateTime())
  {
    CLog::Log(LOGERROR, "%s - end time is in the past", __FUNCTION__);
    return NULL;
  }

  /* set the timer data */
  CDateTime newStart = tag.StartAsUTC();
  CDateTime newEnd = tag.EndAsUTC();
  newTag->m_iClientIndex      = -1;
  newTag->m_strSummary        = newTag->m_epgInfo->Plot();
  newTag->m_iChannelNumber    = channel->ChannelNumber();
  newTag->m_iClientChannelUid = channel->UniqueID();
  newTag->m_iClientId         = channel->ClientID();
  newTag->m_bIsRadio          = channel->IsRadio();
  newTag->SetStartFromUTC(newStart);
  newTag->SetEndFromUTC(newEnd);

  if (tag.Plot().IsEmpty())
  {
    newTag->m_strSummary.Format("%s %s %s %s %s",
        newTag->StartAsLocalTime().GetAsLocalizedDate(),
        g_localizeStrings.Get(19159),
        newTag->StartAsLocalTime().GetAsLocalizedTime("", false),
        g_localizeStrings.Get(19160),
        newTag->EndAsLocalTime().GetAsLocalizedTime("", false));
  }
  else
  {
    newTag->m_strSummary = tag.Plot();
  }

  /* we might have a copy of the tag here, so get the real one from the pvrmanager */
  const CPVREpg *epgTable = channel->GetEPG();
  newTag->m_epgInfo = epgTable ? (CPVREpgInfoTag *) epgTable->GetTag(tag.UniqueBroadcastID(), tag.StartAsUTC()) : NULL;

  /* unused only for reference */
  newTag->m_strFileNameAndPath = "pvr://timers/new";

  return newTag;
}

const CDateTime &CPVRTimerInfoTag::StartAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_StartTime);

  return tmp;
}

const CDateTime &CPVRTimerInfoTag::EndAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_StopTime);

  return tmp;
}

const CDateTime &CPVRTimerInfoTag::FirstDayAsLocalTime(void) const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_FirstDay);

  return tmp;
}
