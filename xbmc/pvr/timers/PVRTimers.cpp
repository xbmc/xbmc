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

#include "Application.h"
#include "FileItem.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

#include "PVRTimers.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "pvr/addons/PVRClients.h"

using namespace std;
using namespace PVR;
using namespace EPG;

CPVRTimers::CPVRTimers(void)
{
  m_bIsUpdating = false;
}

CPVRTimers::~CPVRTimers(void)
{
  Unload();
}

int CPVRTimers::Load()
{
  Unload();
  g_EpgContainer.RegisterObserver(this);
  Update();

  return GetNumTimers();
}

void CPVRTimers::Unload()
{
  CSingleLock lock(m_critSection);
  CEpgContainer *epg = &g_EpgContainer;
  if (epg)
    epg->UnregisterObserver(this);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    vector<CPVRTimerInfoTag*> *timers = it->second;
    for (unsigned int iTagPtr = 0; iTagPtr < timers->size(); iTagPtr++)
      delete timers->at(iTagPtr);
    delete it->second;
  }
  m_tags.clear();
}

int CPVRTimers::LoadFromClients(void)
{
  return g_PVRClients->GetTimers(this);
}

bool CPVRTimers::Update(void)
{
  {
    CSingleLock lock(m_critSection);
    if (m_bIsUpdating)
      return false;
    m_bIsUpdating = true;
  }

  CLog::Log(LOGDEBUG, "CPVRTimers - %s - updating timers", __FUNCTION__);
  CPVRTimers PVRTimers_tmp;
  PVRTimers_tmp.LoadFromClients();

  return UpdateEntries(&PVRTimers_tmp);
}

bool CPVRTimers::IsRecording(void)
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    vector<CPVRTimerInfoTag*> *timers = it->second;
    for (unsigned int iTagPtr = 0; iTagPtr < timers->size(); iTagPtr++)
      if (timers->at(iTagPtr)->IsRecording())
        return true;
  }

  return false;
}

bool CPVRTimers::UpdateEntries(CPVRTimers *timers)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  vector<CStdString> timerNotifications;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = timers->m_tags.begin(); it != timers->m_tags.end(); it++)
  {
    vector<CPVRTimerInfoTag*> *entry = it->second;
    for (unsigned int iTagPtr = 0; iTagPtr < entry->size(); iTagPtr++)
    {
      const CPVRTimerInfoTag *timer = entry->at(iTagPtr);

      /* check if this timer is present in this container */
      CPVRTimerInfoTag *existingTimer = (CPVRTimerInfoTag *) GetByClient(timer->m_iClientId, timer->m_iClientIndex);
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->m_state != timer->m_state);
        if (existingTimer->UpdateEntry(*timer))
        {
          bChanged = true;

          if (bStateChanged && g_PVRManager.IsStarted())
          {
            CStdString strMessage;
            existingTimer->GetNotificationText(strMessage);
            timerNotifications.push_back(strMessage);
          }

          CLog::Log(LOGDEBUG,"PVRTimers - %s - updated timer %d on client %d",
              __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);
        }
      }
      else
      {
        /* new timer */
        CPVRTimerInfoTag *newTimer = new CPVRTimerInfoTag;
        newTimer->UpdateEntry(*timer);

        vector<CPVRTimerInfoTag *>* addEntry = NULL;
        map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator itr = m_tags.find(newTimer->StartAsUTC());
        if (itr == m_tags.end())
        {
          addEntry = new vector<CPVRTimerInfoTag *>;
          m_tags.insert(make_pair(newTimer->StartAsUTC(), addEntry));
        }
        else
        {
          addEntry = itr->second;
        }

        addEntry->push_back(newTimer);
        bChanged = true;
        bAddedOrDeleted = true;

        if (g_PVRManager.IsStarted())
        {
          CStdString strMessage;
          newTimer->GetNotificationText(strMessage);
          timerNotifications.push_back(strMessage);
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - added timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);
      }
    }
  }

  /* check for deleted timers */
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it != m_tags.end() ? it++ : it)
  {
    vector<CPVRTimerInfoTag*> *entry = it->second;
    for (int iTagPtr = entry->size() - 1; iTagPtr >= 0; iTagPtr--)
    {
      CPVRTimerInfoTag *timer = entry->at(iTagPtr);
      if (!timer)
        continue;

      if (timers->GetByClient(timer->m_iClientId, timer->m_iClientIndex) == NULL)
      {
        /* timer was not found */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        if (g_PVRManager.IsStarted())
        {
          CStdString strMessage;
          strMessage.Format("%s: '%s'", (timer->EndAsUTC() <= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()) ? g_localizeStrings.Get(19227) : g_localizeStrings.Get(19228), timer->m_strTitle.c_str());
          timerNotifications.push_back(strMessage);
        }

        delete entry->at(iTagPtr);
        entry->erase(entry->begin() + iTagPtr);

        if (entry->size() == 0)
          m_tags.erase(it++);
        bChanged = true;
        bAddedOrDeleted = true;
      }
    }
  }

  m_bIsUpdating = false;
  if (bChanged)
  {
    SetChanged();
    lock.Leave();

    NotifyObservers(bAddedOrDeleted ? "timers-reset" : "timers", false);

    if (g_guiSettings.GetBool("pvrrecord.timernotifications"))
    {
      /* queue notifications */
      for (unsigned int iNotificationPtr = 0; iNotificationPtr < timerNotifications.size(); iNotificationPtr++)
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
            g_localizeStrings.Get(19166),
            timerNotifications.at(iNotificationPtr));
      }
    }
  }

  return bChanged;
}

bool CPVRTimers::UpdateEntry(const CPVRTimerInfoTag &timer)
{
  CPVRTimerInfoTag *tag = NULL;
  CSingleLock lock(m_critSection);

  if ((tag = GetByClient(timer.m_iClientId, timer.m_iClientIndex)) == NULL)
  {
    tag = new CPVRTimerInfoTag();
    vector<CPVRTimerInfoTag *>* addEntry = NULL;
    map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator itr = m_tags.find(timer.StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new vector<CPVRTimerInfoTag *>;
      m_tags.insert(make_pair(timer.StartAsUTC(), addEntry));
    }
    else
    {
      addEntry = itr->second;
    }
    addEntry->push_back(tag);
  }

  return tag->UpdateEntry(timer);
}

/********** getters **********/

int CPVRTimers::GetTimers(CFileItemList* results)
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    vector<CPVRTimerInfoTag*> *timers = it->second;
    for (unsigned int iTagPtr = 0; iTagPtr < timers->size(); iTagPtr++)
    {
      CFileItemPtr timer(new CFileItem(*timers->at(iTagPtr)));
      results->Add(timer);
      ++iReturn;
    }
  }

  return iReturn;
}

bool CPVRTimers::GetNextActiveTimer(CPVRTimerInfoTag *tag) const
{
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *current = it->second->at(iTimerPtr);
      if (current->IsActive() && !current->IsRecording())
      {
        *tag = *current;
        return true;
      }
    }
  }

  return false;
}

int CPVRTimers::GetActiveTimers(vector<CPVRTimerInfoTag *> *tags) const
{
  int iInitialSize = tags->size();
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      if (it->second->at(iTimerPtr)->IsActive())
        tags->push_back(it->second->at(iTimerPtr));
    }
  }

  return tags->size() - iInitialSize;
}

int CPVRTimers::GetNumActiveTimers(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      if (it->second->at(iTimerPtr)->IsActive())
        ++iReturn;
    }
  }

  return iReturn;
}

int CPVRTimers::GetNumActiveRecordings(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      if (it->second->at(iTimerPtr)->IsRecording())
        ++iReturn;
    }
  }

  return iReturn;
}

CPVRTimerInfoTag *CPVRTimers::GetTimer(const CDateTime &start, int iTimer /* = -1 */) const
{
  CSingleLock lock(m_critSection);
  map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.find(start);
  if (it != m_tags.end() && it->second->size() > 0)
  {
    if (iTimer != -1)
    {
      for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
      {
        if (it->second->at(iTimerPtr)->m_iClientIndex == iTimer)
          return it->second->at(iTimerPtr);
      }
    }
    else
    {
      return it->second->at(0);
    }
  }
  return NULL;
}

int CPVRTimers::GetNumTimers() const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    iReturn += it->second->size();
  return iReturn;
}

bool CPVRTimers::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  URIUtils::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName == "timers")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/add.timer", false));
    item->SetLabel(g_localizeStrings.Get(19026));
    item->SetLabelPreformated(true);
    items.Add(item);

    CSingleLock lock(m_critSection);
    for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
      {
        item.reset(new CFileItem(*it->second->at(iTimerPtr)));
        items.Add(item);
      }
    }

    return true;
  }
  return false;
}

/********** channel methods **********/

bool CPVRTimers::ChannelHasTimers(const CPVRChannel &channel)
{
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timer = it->second->at(iTimerPtr);

      if (timer->ChannelNumber() == channel.ChannelNumber() && timer->m_bIsRadio == channel.IsRadio())
        return true;
    }
  }

  return false;
}


bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannel &channel, bool bDeleteRepeating /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); it++)
  {
    for (int iTimerPtr = it->second->size() - 1; iTimerPtr >= 0; iTimerPtr--)
    {
      CPVRTimerInfoTag *timer = it->second->at(iTimerPtr);

      if (bCurrentlyActiveOnly &&
          (CDateTime::GetCurrentDateTime() < timer->StartAsLocalTime() ||
           CDateTime::GetCurrentDateTime() > timer->EndAsLocalTime()))
        continue;

      if (!bDeleteRepeating && timer->m_bIsRepeating)
        continue;

      if (timer->ChannelNumber() == channel.ChannelNumber() && timer->m_bIsRadio == channel.IsRadio())
      {
        bReturn = timer->DeleteFromClient(true) || bReturn;
        it->second->erase(it->second->begin() + iTimerPtr);
      }
    }
  }

  return bReturn;
}

CPVRTimerInfoTag *CPVRTimers::InstantTimer(CPVRChannel *channel, bool bStartTimer /* = true */)
{
  if (!channel)
    return NULL;

  CEpgInfoTag epgTag;
  bool bHasEpgNow = channel->GetEPGNow(epgTag);
  CPVRTimerInfoTag *newTimer = bHasEpgNow ? CPVRTimerInfoTag::CreateFromEpg(epgTag) : NULL;
  if (!newTimer)
  {
    newTimer = new CPVRTimerInfoTag;
    /* set the timer data */
    newTimer->m_iClientIndex      = -1;
    newTimer->m_strTitle          = channel->ChannelName();
    newTimer->m_strSummary        = g_localizeStrings.Get(19056);
    newTimer->m_iChannelNumber    = channel->ChannelNumber();
    newTimer->m_iClientChannelUid = channel->UniqueID();
    newTimer->m_iClientId         = channel->ClientID();
    newTimer->m_bIsRadio          = channel->IsRadio();

    /* generate summary string */
    newTimer->m_strSummary.Format("%s %s %s %s %s",
        newTimer->StartAsLocalTime().GetAsLocalizedDate(),
        g_localizeStrings.Get(19159),
        newTimer->StartAsLocalTime().GetAsLocalizedTime(StringUtils::EmptyString, false),
        g_localizeStrings.Get(19160),
        newTimer->EndAsLocalTime().GetAsLocalizedTime(StringUtils::EmptyString, false));
  }

  CDateTime startTime(0);
  newTimer->SetStartFromUTC(startTime);
  newTimer->m_iMarginStart = 0; /* set the start margin to 0 for instant timers */

  int iDuration = g_guiSettings.GetInt("pvrrecord.instantrecordtime");
  CDateTime endTime = CDateTime::GetUTCDateTime() + CDateTimeSpan(0, 0, iDuration ? iDuration : 120, 0);
  newTimer->SetEndFromUTC(endTime);

  /* unused only for reference */
  newTimer->m_strFileNameAndPath = "pvr://timers/new";

  if (bStartTimer && !newTimer->AddToClient())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - unable to add an instant timer on the client", __FUNCTION__);
    delete newTimer;
    newTimer = NULL;
  }

  return newTimer;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTag *tag = (CPVRTimerInfoTag *)item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return AddTimer(*tag);
}

bool CPVRTimers::AddTimer(CPVRTimerInfoTag &item)
{
  if (!g_PVRClients->GetAddonCapabilities(item.m_iClientId).bSupportsTimers)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19215,0);
    return false;
  }

  return item.AddToClient();
}

bool CPVRTimers::DeleteTimer(const CFileItem &item, bool bForce /* = false */)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTag *tag = (CPVRTimerInfoTag *)item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return DeleteTimer(*tag, bForce);
}

bool CPVRTimers::DeleteTimer(CPVRTimerInfoTag &item, bool bForce /* = false */)
{
  return item.DeleteFromClient(bForce);
}

bool CPVRTimers::RenameTimer(CFileItem &item, const CStdString &strNewName)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return RenameTimer(*tag, strNewName);
}

bool CPVRTimers::RenameTimer(CPVRTimerInfoTag &item, const CStdString &strNewName)
{
  return item.RenameOnClient(strNewName);
}

bool CPVRTimers::UpdateTimer(const CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  const CPVRTimerInfoTag* tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return UpdateTimer((CPVRTimerInfoTag &) *tag);
}

bool CPVRTimers::UpdateTimer(CPVRTimerInfoTag &item)
{
  return item.UpdateOnClient();
}

CPVRTimerInfoTag *CPVRTimers::GetByClient(int iClientId, int iClientTimerId)
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timer = it->second->at(iTimerPtr);
      if (timer->m_iClientId == iClientId && timer->m_iClientIndex == iClientTimerId)
        return timer;
    }
  }

  return NULL;
}

bool CPVRTimers::IsRecordingOnChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timer = it->second->at(iTimerPtr);

      if (timer->IsRecording() && timer->m_iClientChannelUid == channel.UniqueID() && timer->m_iClientId == channel.ClientID())
        return true;
    }
  }

  return false;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CEpgInfoTag *Epg)
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTag *>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < it->second->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timer = it->second->at(iTimerPtr);

      if (!Epg || !Epg->GetTable() || !Epg->GetTable()->Channel())
        continue;

      const CPVRChannel *channel = Epg->GetTable()->Channel();
      if (timer->ChannelNumber() != channel->ChannelNumber()
          || timer->m_bIsRadio != channel->IsRadio())
        continue;

      if (timer->StartAsUTC() > Epg->StartAsUTC() || timer->EndAsUTC() < Epg->EndAsUTC())
        continue;

      return timer;
    }
  }
  return NULL;
}

CPVRTimerInfoTag *CPVRTimers::GetMatch(const CFileItem *item)
{
  CPVRTimerInfoTag *returnTag = NULL;

  if (item && item->HasEPGInfoTag())
    returnTag = GetMatch(item->GetEPGInfoTag());

  return returnTag;
}

void CPVRTimers::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("epg"))
    g_PVRManager.TriggerTimersUpdate();
}

CDateTime CPVRTimers::GetNextEventTime(void) const
{
  const CStdString wakeupcmd = g_guiSettings.GetString("pvrpowermanagement.setwakeupcmd", false);
  const bool dailywakup = g_guiSettings.GetBool("pvrpowermanagement.dailywakeup");
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(0, 0, g_guiSettings.GetInt("pvrpowermanagement.prewakeup"), 0);
  const CDateTimeSpan idle(0, 0, g_guiSettings.GetInt("pvrpowermanagement.backendidletime"), 0);

  CDateTime timerwakeuptime;
  CDateTime dailywakeuptime;

  /* Check next active time */
  CPVRTimerInfoTag timer;
  if (GetNextActiveTimer(&timer))
  {
    const CDateTime start = timer.StartAsUTC();

    if ((start - idle) > now) {
      timerwakeuptime = start - prewakeup;
    } else {
      timerwakeuptime = now + idle;
    }
  }

  /* check daily wake up */
  if (dailywakup)
  {
    dailywakeuptime.SetFromDBTime(g_guiSettings.GetString("pvrpowermanagement.dailywakeuptime", false));
    dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

    dailywakeuptime.SetDateTime(
      now.GetYear(), now.GetMonth(), now.GetDay(),
      dailywakeuptime.GetHour(), dailywakeuptime.GetMinute(), dailywakeuptime.GetSecond()
    );

    if ((dailywakeuptime - idle) < now)
    {
      const CDateTimeSpan oneDay(1,0,0,0);
      dailywakeuptime += oneDay;
    }
  }

  const CDateTime retVal((dailywakeuptime < timerwakeuptime) ? dailywakeuptime : timerwakeuptime);
  return retVal;
}
