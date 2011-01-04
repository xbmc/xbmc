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

#include "GUISettings.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"

#include "PVRTimerInfoTag.h"
#include "PVRChannel.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

/**
 * Create a blank unmodified timer tag
 */
CPVRTimerInfoTag::CPVRTimerInfoTag()
{
  Reset();
}

/**
 * Creates a instant timer on current date and channel if "bool Init"
 * is set one hour later as now otherwise a blank CPVRTimerInfoTag is
 * given.
 * \param bool Init             = Initialize as instant timer if set
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
CPVRTimerInfoTag::CPVRTimerInfoTag(bool Init)
{
  Reset();

  /* Check if instant flag is set otherwise return */
  if (!Init)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: Can't initialize tag, Init flag not set!");
    return;
  }

  /* Get setup variables */
  int rectime     = g_guiSettings.GetInt("pvrrecord.instantrecordtime");
  int defprio     = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  int deflifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");

  if (!rectime)
    rectime     = 180; /* Default 180 minutes */

  if (!defprio)
    defprio     = 50;  /* Default */

  if (!deflifetime)
    deflifetime = 99;  /* Default 99 days */

  const CPVRChannel *channel = NULL;

  CFileItem *curPlayingChannel = g_PVRManager.GetCurrentPlayingItem();
  if (curPlayingChannel)
    channel = curPlayingChannel->GetPVRChannelInfoTag();
  else
    channel = PVRChannelsTV.GetByNumber(1);

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: constructor can't get valid channel");
    return;
  }

  /* Set default timer */
  m_clientIndex   = -1;
  m_Active        = true;
  m_strTitle      = g_localizeStrings.Get(19056);
  m_channelNum    = channel->ChannelNumber();
  m_clientNum     = channel->ClientChannelNumber();
  m_clientID      = channel->ClientID();
  m_Radio         = channel->IsRadio();

  /* Calculate start/stop times */
  CDateTime time  = CDateTime::GetCurrentDateTime();
  m_StartTime     = time;
  m_StopTime      = time + CDateTimeSpan(0, rectime / 60, rectime % 60, 0);   /* Add recording time */

  /* Set priority and lifetime */
  m_Priority      = defprio;
  m_Lifetime      = deflifetime;

  /* Generate summary string */
  m_Summary.Format("%s %s %s %s %s", m_StartTime.GetAsLocalizedDate()
                   , g_localizeStrings.Get(19159)
                   , m_StartTime.GetAsLocalizedTime("",false)
                   , g_localizeStrings.Get(19160)
                   , m_StopTime.GetAsLocalizedTime("",false));

  m_strFileNameAndPath = "pvr://timers/new"; /* Unused only for reference */
  return;
}

/**
 * Create Timer based upon an TVEPGInfoTag
 * \param const CFileItem& item = reference to CPVREpgInfoTag class
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
CPVRTimerInfoTag::CPVRTimerInfoTag(const CFileItem& item)
{
  Reset();

  const CPVREpgInfoTag* tag = item.GetEPGInfoTag();
  if (tag == NULL)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: Can't initialize tag, no EPGInfoTag given!");
    return;
  }

  const CPVRChannel *channel = CPVRChannels::GetByChannelIDFromAll(tag->ChannelTag()->ChannelID());
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: constructor is called with not present channel");
    return;
  }

  /* Check epg end date is in the future */
  if (tag->End() < CDateTime::GetCurrentDateTime())
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: Can't initialize tag, EPGInfoTag is in the past!");
    return;
  }

  /* Get setup variables */
  int defprio     = g_guiSettings.GetInt("pvrrecord.defaultpriority");
  int deflifetime = g_guiSettings.GetInt("pvrrecord.defaultlifetime");
  int marginstart = g_guiSettings.GetInt("pvrrecord.marginstart");
  int marginstop  = g_guiSettings.GetInt("pvrrecord.marginstop");

  if (!defprio)
    defprio     = 50;  /* Default */

  if (!deflifetime)
    deflifetime = 99;  /* Default 99 days */

  if (!deflifetime)
    marginstart = 2;   /* Default start 2 minutes earlier */

  if (!deflifetime)
    marginstop  = 10;  /* Default stop 10 minutes later */

  /* Set timer based on EPG entry */
  m_clientIndex   = tag->UniqueBroadcastID();
  m_Active        = true;
  m_strTitle      = tag->Title();
  m_channelNum    = channel->ChannelNumber();
  m_clientNum     = channel->ClientChannelNumber();
  m_clientID      = channel->ClientID();
  m_Radio         = channel->IsRadio();

  if (m_strTitle.IsEmpty())
    m_strTitle  = channel->ChannelName();

  /* Calculate start/stop times */
  m_StartTime     = tag->Start() - CDateTimeSpan(0, marginstart / 60, marginstart % 60, 0);
  m_StopTime      = tag->End()  + CDateTimeSpan(0, marginstop / 60, marginstop % 60, 0);

  /* Set priority and lifetime */
  m_Priority      = defprio;
  m_Lifetime      = deflifetime;

  /* Generate summary string */
  m_Summary.Format("%s %s %s %s %s", m_StartTime.GetAsLocalizedDate()
                   , g_localizeStrings.Get(19159)
                   , m_StartTime.GetAsLocalizedTime("",false)
                   , g_localizeStrings.Get(19160)
                   , m_StopTime.GetAsLocalizedTime("",false));

  m_strFileNameAndPath = "pvr://timers/new"; /* Unused only for reference */
  return;
}

/**
 * Compare equal for two CPVRTimerInfoTag
 */
bool CPVRTimerInfoTag::operator ==(const CPVRTimerInfoTag& right) const
{
  if (this == &right) return true;

  return (m_clientIndex           == right.m_clientIndex &&
          m_Active                == right.m_Active &&
          m_Summary               == right.m_Summary &&
          m_channelNum            == right.m_channelNum &&
          m_clientNum             == right.m_clientNum &&
          m_Radio                 == right.m_Radio &&
          m_Repeat                == right.m_Repeat &&
          m_StartTime             == right.m_StartTime &&
          m_StopTime              == right.m_StopTime &&
          m_FirstDay              == right.m_FirstDay &&
          m_Weekdays              == right.m_Weekdays &&
          m_recStatus             == right.m_recStatus &&
          m_Priority              == right.m_Priority &&
          m_Lifetime              == right.m_Lifetime &&
          m_strFileNameAndPath    == right.m_strFileNameAndPath &&
          m_strTitle              == right.m_strTitle &&
          m_clientID              == right.m_clientID);
}

/**
 * Compare not equal for two CPVRTimerInfoTag
 */
bool CPVRTimerInfoTag::operator !=(const CPVRTimerInfoTag& right) const
{
  if (this == &right) return false;

  return (m_clientIndex           != right.m_clientIndex &&
          m_Active                != right.m_Active &&
          m_Summary               != right.m_Summary &&
          m_channelNum            != right.m_channelNum &&
          m_clientNum             != right.m_clientNum &&
          m_Radio                 != right.m_Radio &&
          m_Repeat                != right.m_Repeat &&
          m_StartTime             != right.m_StartTime &&
          m_StopTime              != right.m_StopTime &&
          m_FirstDay              != right.m_FirstDay &&
          m_Weekdays              != right.m_Weekdays &&
          m_recStatus             != right.m_recStatus &&
          m_Priority              != right.m_Priority &&
          m_Lifetime              != right.m_Lifetime &&
          m_strFileNameAndPath    != right.m_strFileNameAndPath &&
          m_strTitle              != right.m_strTitle &&
          m_clientID              != right.m_clientID);
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

int CPVRTimerInfoTag::Compare(const CPVRTimerInfoTag &timer) const
{
  time_t timer1 = StartTime();
  time_t timer2 = timer.StartTime();
  int r = timer1 - timer2;
  if (r == 0)
    r = timer.m_Priority - m_Priority;
  return r;
}

/**
 * Initialize blank CPVRTimerInfoTag
 */
void CPVRTimerInfoTag::Reset()
{
  m_strTitle      = "";
  m_strDir        = "/";
  m_Summary       = "";

  m_Active        = false;
  m_channelNum    = -1;
  m_clientID      = g_PVRManager.GetFirstClientID();
  m_clientIndex   = -1;
  m_clientNum     = -1;
  m_Radio         = false;
  m_recStatus     = false;

  m_StartTime     = NULL;
  m_StopTime      = NULL;

  m_Repeat        = false;
  m_FirstDay      = NULL;
  m_Weekdays      = 0;

  m_Priority      = -1;
  m_Lifetime      = -1;

  m_EpgInfo       = NULL;

  m_strFileNameAndPath = "";
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
const CStdString CPVRTimerInfoTag::GetStatus() const
{
  if (m_strFileNameAndPath == "pvr://timers/add.timer")
    return g_localizeStrings.Get(19026);
  else if (!m_Active)
    return g_localizeStrings.Get(13106);
  else if (m_Active && (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime()))
    return g_localizeStrings.Get(19162);
  else
    return g_localizeStrings.Get(305);
}

bool CPVRTimerInfoTag::Add() const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->AddTimer(*this);
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

bool CPVRTimerInfoTag::Delete(bool force) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->DeleteTimer(*this, force);

    if (err == PVR_ERROR_RECORDING_RUNNING)
    {
      if (CGUIDialogYesNo::ShowAndGetInput(122,0,19122,0))
        err = clients->find(m_clientID)->second->DeleteTimer(*this, true);
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

bool CPVRTimerInfoTag::Rename(CStdString &newname) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->RenameTimer(*this, newname);

    if (err == PVR_ERROR_NOT_IMPLEMENTED)
      err = clients->find(m_clientID)->second->UpdateTimer(*this);

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

bool CPVRTimerInfoTag::Update() const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->UpdateTimer(*this);
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

void CPVRTimerInfoTag::SetEpg(const CPVREpgInfoTag *tag)
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
  CPVRChannel *channeltag = CPVRChannels::GetByClientFromAll(m_clientNum, m_clientID);
  if (channeltag)
    return channeltag->ChannelNumber();
  else
    return 0;
}

CStdString CPVRTimerInfoTag::ChannelName() const
{
  CPVRChannel *channeltag = CPVRChannels::GetByClientFromAll(m_clientNum, m_clientID);
  if (channeltag)
    return channeltag->ChannelName();
  else
    return "";
}
