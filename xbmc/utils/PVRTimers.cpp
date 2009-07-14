/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * DESCRIPTION:
 *
 * cPVRTimerInfoTag is part of the PVRManager to support sheduled recordings.
 *
 * The timer information tag holds data about current programmed timers for
 * the PVRManager. It is possible to create timers directly based upon
 * a EPG entry by giving the EPG information tag or as instant timer
 * on currently tuned channel, or give a blank tag to modify later.
 *
 * With exception of the blank one, the tag can easily and unmodified added
 * by the PVRManager function "bool AddTimer(const CFileItem &item)" to
 * the backend server.
 *
 * The filename inside the tag is for reference only and gives the index
 * number of the tag reported by the PVR backend and can not be played!
 *
 *
 * USED SETUP VARIABLES:
 *
 * ------------- Name -------------|---Type--|-default-|--Description-----
 * pvrmanager.instantrecordtime    = Integer = 180     = Length of a instant timer in minutes
 * pvrmanager.defaultpriority      = Integer = 50      = Default Priority
 * pvrmanager.defaultlifetime      = Integer = 99      = Liftime of the timer in days
 * pvrmanager.marginstart          = Integer = 2       = Minutes to start record earlier
 * pvrmanager.marginstop           = Integer = 10      = Minutes to stop record later
 *
 */

#include "stdafx.h"
#include "PVRTimers.h"
#include "TVEPGInfoTag.h"
#include "GUISettings.h"
#include "PVRManager.h"
#include "FileItem.h"
#include "Util.h"


/**
 * Create a blank unmodified timer tag
 */
cPVRTimerInfoTag::cPVRTimerInfoTag()
{
  Reset();
}

/**
 * Creates a instant timer on current date and channel if "bool Init"
 * is set one hour later as now otherwise a blank cPVRTimerInfoTag is
 * given.
 * \param bool Init             = Initialize as instant timer if set
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
cPVRTimerInfoTag::cPVRTimerInfoTag(bool Init)
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

  const cPVRChannelInfoTag *channel = NULL;

  CFileItem *curPlayingChannel = CPVRManager::GetInstance()->GetCurrentChannelItem();
  if (curPlayingChannel)
    channel = curPlayingChannel->GetTVChannelInfoTag();
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
  m_strTitle      = g_localizeStrings.Get(18072);
  m_channelNum    = channel->Number();
  m_clientNum     = channel->ClientNumber();
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
                   , g_localizeStrings.Get(18078)
                   , m_StartTime.GetAsLocalizedTime("",false)
                   , g_localizeStrings.Get(18079)
                   , m_StopTime.GetAsLocalizedTime("",false));

  m_strFileNameAndPath = "pvr://timers/new"; /* Unused only for reference */
  return;
}

/**
 * Create Timer based upon an TVEPGInfoTag
 * \param const CFileItem& item = reference to CTVEPGInfoTag class
 *
 * Note:
 * Check active flag "m_Active" is set, after creating the tag. If it
 * is false something goes wrong during initialization!
 * See Log for errors.
 */
cPVRTimerInfoTag::cPVRTimerInfoTag(const CFileItem& item)
{
  Reset();

  const CTVEPGInfoTag* tag = item.GetTVEPGInfoTag();
  if (tag == NULL)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: Can't initialize tag, no EPGInfoTag given!");
    return;
  }

  const cPVRChannelInfoTag *channel = cPVRChannels::GetByChannelIDFromAll(tag->m_idChannel);
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "cPVRTimerInfoTag: constructor is called with not present channel");
    return;
  }

  /* Check epg end date is in the future */
  if (tag->m_endTime < CDateTime::GetCurrentDateTime())
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
  m_clientIndex   = -1;
  m_Active        = true;
  m_strTitle      = tag->m_strTitle;
  m_channelNum    = channel->m_iChannelNum;
  m_clientNum     = channel->m_iClientNum;
  m_clientID      = channel->m_clientID;
  m_Radio         = channel->m_radio;

  if (m_strTitle.IsEmpty())
    m_strTitle  = channel->m_strChannel;

  /* Calculate start/stop times */
  m_StartTime     = tag->m_startTime - CDateTimeSpan(0, marginstart / 60, marginstart % 60, 0);
  m_StopTime      = tag->m_endTime  + CDateTimeSpan(0, marginstop / 60, marginstop % 60, 0);

  /* Set priority and lifetime */
  m_Priority      = defprio;
  m_Lifetime      = deflifetime;

  /* Generate summary string */
  m_Summary.Format("%s %s %s %s %s", m_StartTime.GetAsLocalizedDate()
                   , g_localizeStrings.Get(18078)
                   , m_StartTime.GetAsLocalizedTime("",false)
                   , g_localizeStrings.Get(18079)
                   , m_StopTime.GetAsLocalizedTime("",false));

  m_strFileNameAndPath = "pvr://timers/new"; /* Unused only for reference */
  return;
}

/**
 * Compare equal for two cPVRTimerInfoTag
 */
bool cPVRTimerInfoTag::operator ==(const cPVRTimerInfoTag& right) const
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
 * Compare not equal for two cPVRTimerInfoTag
 */
bool cPVRTimerInfoTag::operator !=(const cPVRTimerInfoTag& right) const
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

time_t cPVRTimerInfoTag::StartTime(void) const
{ 
  time_t start;
  m_StartTime.GetAsTime(start);
  return start; 
}

time_t cPVRTimerInfoTag::StopTime(void) const
{ 
  time_t stop;
  m_StopTime.GetAsTime(stop);
  return stop; 
}
  
int cPVRTimerInfoTag::Compare(const cPVRTimerInfoTag &timer) const
{
  time_t timer1 = StartTime();
  time_t timer2 = timer.StartTime();
  int r = timer1 - timer2;
  if (r == 0)
    r = timer.m_Priority - m_Priority;
  return r;
}

/**
 * Initialize blank cPVRTimerInfoTag
 */
void cPVRTimerInfoTag::Reset()
{
  m_strTitle      = "";
  m_Summary       = "";

  m_Active        = false;
  m_channelNum    = -1;
  m_clientID      = CPVRManager::GetInstance()->GetFirstClientID();
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

  m_strFileNameAndPath = "";
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
const CStdString cPVRTimerInfoTag::GetStatus() const
{
  if (m_strFileNameAndPath == "pvr://timers/add.timer")
    return g_localizeStrings.Get(18057);
  else if (!m_Active)
    return g_localizeStrings.Get(13106);
  else if (m_StartTime < CDateTime::GetCurrentDateTime() && m_StopTime > CDateTime::GetCurrentDateTime())
    return g_localizeStrings.Get(18069);
  else
    return g_localizeStrings.Get(305);
}


// --- cPVRTimers ---------------------------------------------------------------

cPVRTimers PVRTimers;

cPVRTimers::cPVRTimers(void)
{

}

bool cPVRTimers::Load()
{
  CPVRManager *manager  = CPVRManager::GetInstance();
  CLIENTMAP   *clients  = manager->Clients();
  
  Clear();

  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    IPVRClient* client = (*itr).second;
    if (client->GetNumTimers() > 0)
    {
      client->GetAllTimers(this);
    }
    itr++;
  }
    
  return true;
}

bool cPVRTimers::Update()
{
  Load();
}

int cPVRTimers::GetNumTimers()
{
  return size();
}

cPVRTimerInfoTag *cPVRTimers::GetTimer(cPVRTimerInfoTag *Timer)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).m_channelNum == Timer->m_channelNum &&
        (at(i).m_Weekdays && at(i).m_Weekdays == Timer->m_Weekdays || !at(i).m_Weekdays && at(i).m_FirstDay == Timer->m_FirstDay) &&
        at(i).m_StartTime == Timer->m_StartTime &&
        at(i).m_StopTime == Timer->m_StopTime)
      return &at(i);
  }
  return NULL;
}

cPVRTimerInfoTag *cPVRTimers::GetMatch(CDateTime t)
{

  return NULL;
}

cPVRTimerInfoTag *cPVRTimers::GetMatch(time_t t)
{

  return NULL;
}

cPVRTimerInfoTag *cPVRTimers::GetMatch(const CTVEPGInfoTag *Epg, int *Match)
{

  return NULL;
}

cPVRTimerInfoTag *cPVRTimers::GetNextActiveTimer(void)
{
  cPVRTimerInfoTag *t0 = NULL;
  for (unsigned int i = 0; i < size(); i++)
  {
    if ((at(i).m_Active) && (!t0 || at(i).m_StopTime > CDateTime::GetCurrentDateTime() && at(i).Compare(*t0) < 0))
    {
      t0 = &at(i);
    }
  }
  return t0;
}

void cPVRTimers::Clear()
{
  /* Clear all current present Timers inside list */
  erase(begin(), end());
  return;
}
