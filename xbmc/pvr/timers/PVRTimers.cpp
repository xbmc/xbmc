/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
#include "URL.h"

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

bool CPVRTimers::Load(void)
{
  // unload previous timers
  Unload();

  // (re)register observer
  g_EpgContainer.RegisterObserver(this);

  // update from clients
  return Update();
}

void CPVRTimers::Unload()
{
  // unregister observer
  g_EpgContainer.UnregisterObserver(this);

  // remove all tags
  CSingleLock lock(m_critSection);
  m_tags.clear();
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
  CPVRTimers newTimerList;
  g_PVRClients->GetTimers(&newTimerList);
  return UpdateEntries(newTimerList);
}

bool CPVRTimers::IsRecording(void) const
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      if ((*timerIt)->IsRecording())
        return true;

  return false;
}

bool CPVRTimers::UpdateEntries(const CPVRTimers &timers)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  vector<CStdString> timerNotifications;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = timers.m_tags.begin(); it != timers.m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      /* check if this timer is present in this container */
      CPVRTimerInfoTagPtr existingTimer = GetByClient((*timerIt)->m_iClientId, (*timerIt)->m_iClientIndex);
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->m_state != (*timerIt)->m_state);
        if (existingTimer->UpdateEntry(*(*timerIt)))
        {
          bChanged = true;
          UpdateEpgEvent(existingTimer);

          if (bStateChanged && g_PVRManager.IsStarted())
          {
            CStdString strMessage;
            existingTimer->GetNotificationText(strMessage);
            timerNotifications.push_back(strMessage);
          }

          CLog::Log(LOGDEBUG,"PVRTimers - %s - updated timer %d on client %d",
              __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
        }
      }
      else
      {
        /* new timer */
        CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag);
        newTimer->UpdateEntry(*(*timerIt));
        UpdateEpgEvent(newTimer);

        vector<CPVRTimerInfoTagPtr>* addEntry = NULL;
        map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::iterator itr = m_tags.find(newTimer->StartAsUTC());
        if (itr == m_tags.end())
        {
          addEntry = new vector<CPVRTimerInfoTagPtr>;
          m_tags.insert(make_pair(newTimer->StartAsUTC(), addEntry));
        }
        else
        {
          addEntry = itr->second;
        }

        addEntry->push_back(newTimer);
        UpdateEpgEvent(newTimer);
        bChanged = true;
        bAddedOrDeleted = true;

        if (g_PVRManager.IsStarted())
        {
          CStdString strMessage;
          newTimer->GetNotificationText(strMessage);
          timerNotifications.push_back(strMessage);
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - added timer %d on client %d",
            __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
      }
    }
  }

  /* to collect timer with changed starting time */
  vector<CPVRTimerInfoTagPtr> timersToMove;
  
  /* check for deleted timers */
  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::iterator it = m_tags.begin(); it != m_tags.end();)
  {
    for (int iTimerPtr = it->second->size() - 1; iTimerPtr >= 0; iTimerPtr--)
    {
      CPVRTimerInfoTagPtr timer = it->second->at(iTimerPtr);
      if (!timers.GetByClient(timer->m_iClientId, timer->m_iClientIndex))
      {
        /* timer was not found */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        if (g_PVRManager.IsStarted())
        {
          CStdString strMessage;
          strMessage.Format("%s: '%s'",
              (timer->EndAsUTC() <= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()) ?
                  g_localizeStrings.Get(19227) :
                  g_localizeStrings.Get(19228),
                  timer->m_strTitle.c_str());
          timerNotifications.push_back(strMessage);
        }

        it->second->erase(it->second->begin() + iTimerPtr);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else if (timer->StartAsUTC() != it->first)
      {
        /* timer start has changed */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - changed start time timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        timer->ClearEpgTag();

        /* remember timer */
        timersToMove.push_back(timer);
        
        /* remove timer for now, reinsert later */
        it->second->erase(it->second->begin() + iTimerPtr);

        bChanged = true;
        bAddedOrDeleted = true;
      }
    }
    if (it->second->size() == 0)
      m_tags.erase(it++);
    else
      ++it;
  }

  /* reinsert timers with changed timer start */
  for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = timersToMove.begin(); timerIt != timersToMove.end(); timerIt++)
  {
      vector<CPVRTimerInfoTagPtr>* addEntry = NULL;
      map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator itr = m_tags.find((*timerIt)->StartAsUTC());
      if (itr == m_tags.end())
      {
        addEntry = new vector<CPVRTimerInfoTagPtr>;
        m_tags.insert(make_pair((*timerIt)->StartAsUTC(), addEntry));
      }
      else
      {
        addEntry = itr->second;
      }

      addEntry->push_back(*timerIt);
      UpdateEpgEvent(*timerIt);
  }

  m_bIsUpdating = false;
  if (bChanged)
  {
    UpdateChannels();
    SetChanged();
    lock.Leave();

    NotifyObservers(bAddedOrDeleted ? ObservableMessageTimersReset : ObservableMessageTimers);

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

bool CPVRTimers::UpdateFromClient(const CPVRTimerInfoTag &timer)
{
  CSingleLock lock(m_critSection);
  CPVRTimerInfoTagPtr tag = GetByClient(timer.m_iClientId, timer.m_iClientIndex);
  if (!tag)
  {
    tag = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag());
    vector<CPVRTimerInfoTagPtr>* addEntry = NULL;
    map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::iterator itr = m_tags.find(timer.StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new vector<CPVRTimerInfoTagPtr>;
      m_tags.insert(make_pair(timer.StartAsUTC(), addEntry));
    }
    else
    {
      addEntry = itr->second;
    }
    addEntry->push_back(tag);
  }

  UpdateEpgEvent(tag);

  return tag->UpdateEntry(timer);
}

/********** getters **********/

CFileItemPtr CPVRTimers::GetNextActiveTimer(void) const
{
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsRecording())
      {
        CFileItemPtr fileItem(new CFileItem(*current));
        return fileItem;
      }
    }
  }

  CFileItemPtr fileItem;
  return fileItem;
}

vector<CFileItemPtr> CPVRTimers::GetActiveTimers(void) const
{
  vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive())
      {
        CFileItemPtr fileItem(new CFileItem(*current));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

int CPVRTimers::AmountActiveTimers(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      if ((*timerIt)->IsActive())
        ++iReturn;

  return iReturn;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRecordings(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsRecording())
      {
        CFileItemPtr fileItem(new CFileItem(*current));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

int CPVRTimers::AmountActiveRecordings(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      if ((*timerIt)->IsRecording())
        ++iReturn;

  return iReturn;
}

bool CPVRTimers::HasActiveTimers(void) const
{
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      if ((*timerIt)->IsActive())
        return true;

  return false;
}

bool CPVRTimers::GetDirectory(const CStdString& strPath, CFileItemList &items) const
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
    for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      {
        CPVRTimerInfoTagPtr current = *timerIt;
        item.reset(new CFileItem(*current));
        items.Add(item);
      }
    }

    return true;
  }
  return false;
}

/********** channel methods **********/

bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannel &channel, bool bDeleteRepeating /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::iterator timerIt = it->second->begin(); timerIt != it->second->end(); )
    {
      bool bDeleteActiveItem = !bCurrentlyActiveOnly ||
          (CDateTime::GetCurrentDateTime() > (*timerIt)->StartAsLocalTime() &&
           CDateTime::GetCurrentDateTime() < (*timerIt)->EndAsLocalTime());
      bool bDeleteRepeatingItem = bDeleteRepeating || !(*timerIt)->m_bIsRepeating;
      bool bChannelsMatch = (*timerIt)->ChannelNumber() == channel.ChannelNumber() &&
          (*timerIt)->m_bIsRadio == channel.IsRadio();

      if (bDeleteActiveItem && bDeleteRepeatingItem && bChannelsMatch)
      {
        bReturn = (*timerIt)->DeleteFromClient(true) || bReturn;
        timerIt = it->second->erase(timerIt);
      }
      else
      {
        ++timerIt;
      }
    }
  }

  return bReturn;
}

bool CPVRTimers::InstantTimer(const CPVRChannel &channel)
{
  if (!g_PVRManager.CheckParentalLock(channel))
    return false;

  CEpgInfoTag epgTag;
  bool bHasEpgNow = channel.GetEPGNow(epgTag);
  CPVRTimerInfoTag *newTimer = bHasEpgNow ? CPVRTimerInfoTag::CreateFromEpg(epgTag) : NULL;
  if (!newTimer)
  {
    newTimer = new CPVRTimerInfoTag;
    /* set the timer data */
    newTimer->m_iClientIndex      = -1;
    newTimer->m_strTitle          = channel.ChannelName();
    newTimer->m_strSummary        = g_localizeStrings.Get(19056);
    newTimer->m_iChannelNumber    = channel.ChannelNumber();
    newTimer->m_iClientChannelUid = channel.UniqueID();
    newTimer->m_iClientId         = channel.ClientID();
    newTimer->m_bIsRadio          = channel.IsRadio();

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

  bool bReturn = newTimer->AddToClient();
  if (!bReturn)
    CLog::Log(LOGERROR, "PVRTimers - %s - unable to add an instant timer on the client", __FUNCTION__);

  delete newTimer;

  return bReturn;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CPVRTimerInfoTag &item)
{
  if (!item.m_channel)
    return false;

  if (!g_PVRClients->SupportsTimers(item.m_iClientId))
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19215,0);
    return false;
  }

  if (!g_PVRManager.CheckParentalLock(*item.m_channel))
    return false;

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

  const CPVRTimerInfoTag *tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->DeleteFromClient(bForce);
}

bool CPVRTimers::RenameTimer(CFileItem &item, const CStdString &strNewName)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTag *tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->RenameOnClient(strNewName);
}

bool CPVRTimers::UpdateTimer(CFileItem &item)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTag *tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->UpdateOnClient();
}

CPVRTimerInfoTagPtr CPVRTimers::GetByClient(int iClientId, int iClientTimerId) const
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      if ((*timerIt)->m_iClientId == iClientId &&
          (*timerIt)->m_iClientIndex == iClientTimerId)
        return *timerIt;
    }
  }

  CPVRTimerInfoTagPtr empty;
  return empty;
}

bool CPVRTimers::IsRecordingOnChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
    {
      if ((*timerIt)->IsRecording() &&
          (*timerIt)->m_iClientChannelUid == channel.UniqueID() &&
          (*timerIt)->m_iClientId == channel.ClientID())
        return true;
    }
  }

  return false;
}

CFileItemPtr CPVRTimers::GetTimerForEpgTag(const CFileItem *item) const
{
  if (item && item->HasEPGInfoTag() && item->GetEPGInfoTag()->ChannelTag())
  {
    const CEpgInfoTag *epgTag = item->GetEPGInfoTag();
    const CPVRChannelPtr channel = epgTag->ChannelTag();
    CSingleLock lock(m_critSection);

    for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      for (vector<CPVRTimerInfoTagPtr>::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      {
        CPVRTimerInfoTagPtr timer = *timerIt;
        if (timer->m_iClientChannelUid == channel->UniqueID() &&
            timer->m_bIsRadio == channel->IsRadio() &&
            timer->StartAsUTC() <= epgTag->StartAsUTC() &&
            timer->EndAsUTC() >= epgTag->EndAsUTC())
        {
          CFileItemPtr fileItem(new CFileItem(*timer));
          return fileItem;
        }
      }
    }
  }

  CFileItemPtr fileItem;
  return fileItem;
}

void CPVRTimers::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageEpgContainer)
    g_PVRManager.TriggerTimersUpdate();
}

CDateTime CPVRTimers::GetNextEventTime(void) const
{
  const bool dailywakup = g_guiSettings.GetBool("pvrpowermanagement.dailywakeup");
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(0, 0, g_guiSettings.GetInt("pvrpowermanagement.prewakeup"), 0);
  const CDateTimeSpan idle(0, 0, g_guiSettings.GetInt("pvrpowermanagement.backendidletime"), 0);

  CDateTime wakeuptime;

  /* Check next active time */
  CFileItemPtr item = GetNextActiveTimer();
  if (item && item->HasPVRTimerInfoTag())
  {
    const CDateTime start = item->GetPVRTimerInfoTag()->StartAsUTC();
    wakeuptime = ((start - idle) > now) ?
        start - prewakeup:
        now + idle;
  }

  /* check daily wake up */
  if (dailywakup)
  {
    CDateTime dailywakeuptime;
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
    if (dailywakeuptime < wakeuptime)
      wakeuptime = dailywakeuptime;
  }

  const CDateTime retVal(wakeuptime);
  return retVal;
}

void CPVRTimers::UpdateEpgEvent(CPVRTimerInfoTagPtr timer)
{
  CSingleLock lock(timer->m_critSection);

  /* already got an epg event set */
  if (timer->m_epgTag)
    return;

  /* try to get the channel */
  CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(timer->m_iClientChannelUid, timer->m_iClientId);
  if (!channel)
    return;

  /* try to get the EPG table */
  CEpg *epg = channel->GetEPG();
  if (!epg)
    return;

  /* try to set the timer on the epg tag that matches with a 2 minute margin */
  CEpgInfoTagPtr epgTag = epg->GetTagBetween(timer->StartAsUTC() - CDateTimeSpan(0, 0, 2, 0), timer->EndAsUTC() + CDateTimeSpan(0, 0, 2, 0));
  if (!epgTag)
    epgTag = epg->GetTagAround(timer->StartAsUTC());

  if (epgTag)
  {
    timer->m_epgTag = epgTag;
    timer->m_genre = epgTag->Genre();
    timer->m_iGenreType = epgTag->GenreType();
    timer->m_iGenreSubType = epgTag->GenreSubType();
    epgTag->SetTimer(timer);
  }
}

void CPVRTimers::UpdateChannels(void)
{
  CSingleLock lock(m_critSection);
  for (map<CDateTime, vector<CPVRTimerInfoTagPtr>* >::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    for (vector<CPVRTimerInfoTagPtr>::iterator timerIt = it->second->begin(); timerIt != it->second->end(); timerIt++)
      (*timerIt)->UpdateChannel();
  }
}
