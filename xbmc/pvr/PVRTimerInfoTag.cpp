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

#include "PVRTimerInfoTag.h"
#include "PVRChannelGroupsContainer.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

CPVRTimerInfoTag::CPVRTimerInfoTag()
{
  m_strTitle           = "";
  m_strDir             = "/";
  m_strSummary         = "";
  m_bValidSummary      = false;
  m_bIsActive          = false;
  m_iChannelNumber     = -1;
  m_iClientID          = g_PVRManager.GetFirstClientID();
  m_iClientIndex       = -1;
  m_iClientNumber      = -1;
  m_bIsRadio           = false;
  m_bIsRecording       = false;
  m_StartTime          = NULL;
  m_StopTime           = NULL;
  m_bIsRepeating       = false;
  m_FirstDay           = NULL;
  m_iWeekdays          = 0;
  m_iPriority          = -1;
  m_iLifetime          = -1;
  m_EpgInfo            = NULL;
  m_strFileNameAndPath = "";
}

bool CPVRTimerInfoTag::operator ==(const CPVRTimerInfoTag& right) const
{
  if (this == &right) return true;

  return (m_iClientIndex       == right.m_iClientIndex &&
          m_bIsActive          == right.m_bIsActive &&
          m_strSummary         == right.m_strSummary &&
          m_iChannelNumber     == right.m_iChannelNumber &&
          m_iClientNumber      == right.m_iClientNumber &&
          m_bIsRadio           == right.m_bIsRadio &&
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
          m_iClientID          == right.m_iClientID);
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

void CPVRTimerInfoTag::SetStart(CDateTime Start)
{
  m_StartTime = Start;
  m_bValidSummary = false;
}

void CPVRTimerInfoTag::SetStop(CDateTime Start)
{
  m_StopTime = Start;
  m_bValidSummary = false;
}

void CPVRTimerInfoTag::SetWeekdays(int Weekdays)
{
  m_iWeekdays = Weekdays;
  m_bValidSummary = false;
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

  m_bValidSummary = true;
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
const CStdString CPVRTimerInfoTag::GetStatus() const
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

bool CPVRTimerInfoTag::AddToClient() const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_iClientID)->second->AddTimer(*this);
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    if (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime())
      g_PVRManager.TriggerRecordingsUpdate(false);

    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

bool CPVRTimerInfoTag::DeleteFromClient(bool force) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_iClientID)->second->DeleteTimer(*this, force);

    if (err == PVR_ERROR_RECORDING_RUNNING)
    {
      if (CGUIDialogYesNo::ShowAndGetInput(122,0,19122,0))
        err = clients->find(m_iClientID)->second->DeleteTimer(*this, true);
    }

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

bool CPVRTimerInfoTag::RenameOnClient(const CStdString &newname) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_iClientID)->second->RenameTimer(*this, newname);

    if (err == PVR_ERROR_NOT_IMPLEMENTED)
      err = clients->find(m_iClientID)->second->UpdateTimer(*this);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }

  return false;
}

bool CPVRTimerInfoTag::UpdateOnClient() const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_iClientID)->second->UpdateTimer(*this);
    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    if (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime())
      g_PVRManager.TriggerRecordingsUpdate(false);

    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
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

void CPVRTimerInfoTag::SetEpgInfoTag(const CPVREpgInfoTag *tag)
{
  if (m_EpgInfo != tag)
  {
    if (tag)
      CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to epg event %s", Title().c_str(), tag->Title().c_str());
    else
      CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to no epg event", Title().c_str());
    m_EpgInfo = tag;
  }
}

int CPVRTimerInfoTag::ChannelNumber() const
{
  CPVRChannel *channeltag = CPVRChannelGroup::GetByClientFromAll(m_iClientNumber, m_iClientID);
  if (channeltag)
    return channeltag->ChannelNumber();
  else
    return 0;
}

CStdString CPVRTimerInfoTag::ChannelName() const
{
  CPVRChannel *channeltag = CPVRChannelGroup::GetByClientFromAll(m_iClientNumber, m_iClientID);
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

CPVRTimerInfoTag *CPVRTimerInfoTag::InstantTimer()
{
  /* create a new timer */
  CPVRTimerInfoTag *newTag = new CPVRTimerInfoTag();
  if (!newTag)
  {
    CLog::Log(LOGERROR, "%s - couldn't create new timer", __FUNCTION__);
    return NULL;
  }

  CFileItem *curPlayingChannel = g_PVRManager.GetCurrentPlayingItem();
  CPVRChannel *channel = (curPlayingChannel) ? curPlayingChannel->GetPVRChannelInfoTag(): NULL;
  if (!channel)
  {
    CLog::Log(LOGDEBUG, "%s - couldn't find current playing channel", __FUNCTION__);
    channel = g_PVRChannelGroups.GetGroupAllTV()->GetByChannelNumber(1);

    if (!channel)
    {
      CLog::Log(LOGERROR, "%s - cannot find any channels",
          __FUNCTION__);
    }
  }

  int iDuration = g_guiSettings.GetInt("pvrrecord.instantrecordtime");
  if (!iDuration)
    iDuration   = 180; /* default to 180 minutes */

  int iPriority = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  if (!iPriority)
    iPriority   = 50;  /* default to 50 */

  int iLifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
  if (!iLifetime)
    iLifetime   = 30;  /* default to 30 days */

  /* set the timer data */
  newTag->m_iClientIndex   = -1;
  newTag->m_bIsActive      = true;
  newTag->m_strTitle       = g_localizeStrings.Get(19056);
  newTag->m_iChannelNumber = channel->ChannelNumber();
  newTag->m_iClientNumber  = channel->ClientChannelNumber();
  newTag->m_iClientID      = channel->ClientID();
  newTag->m_bIsRadio       = channel->IsRadio();
  newTag->m_StartTime      = CDateTime::GetCurrentDateTime();
  newTag->SetDuration(iDuration);
  newTag->m_iPriority      = iPriority;
  newTag->m_iLifetime      = iLifetime;

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
  newTag->m_iClientIndex   = (tag.UniqueBroadcastID() > 0 ? tag.UniqueBroadcastID() : channel->ClientID());
  newTag->m_bIsActive      = true;
  newTag->m_strTitle       = tag.Title().IsEmpty() ? channel->ChannelName() : tag.Title();
  newTag->m_iChannelNumber = channel->ChannelNumber();
  newTag->m_iClientNumber  = channel->ClientChannelNumber();
  newTag->m_iClientID      = channel->ClientID();
  newTag->m_bIsRadio       = channel->IsRadio();
  newTag->m_StartTime      = tag.Start() - CDateTimeSpan(0, iMarginStart / 60, iMarginStart % 60, 0);
  newTag->m_StopTime       = tag.End() + CDateTimeSpan(0, iMarginStop / 60, iMarginStop % 60, 0);
  newTag->m_iPriority      = iPriority;
  newTag->m_iLifetime      = iLifetime;

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
