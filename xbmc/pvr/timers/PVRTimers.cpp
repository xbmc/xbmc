/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "FileItem.h"
#include "settings/Settings.h"
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

#include <assert.h>
#include <cstdlib>

using namespace PVR;
using namespace EPG;

CPVRTimers::CPVRTimers(void)
{
  m_bIsUpdating = false;
  m_iLastId     = 0;
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

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsRecording())
        return true;

  return false;
}

bool CPVRTimers::UpdateEntries(const CPVRTimers &timers)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  std::vector<std::string> timerNotifications;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (MapTags::const_iterator it = timers.m_tags.begin(); it != timers.m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      /* check if this timer is present in this container */
      CPVRTimerInfoTagPtr existingTimer = GetByClient((*timerIt)->m_iClientId, (*timerIt)->m_iClientIndex);
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->m_state != (*timerIt)->m_state);
        if (existingTimer->UpdateEntry(*timerIt))
        {
          bChanged = true;
          UpdateEpgEvent(existingTimer);

          if (bStateChanged && g_PVRManager.IsStarted())
          {
            std::string strMessage;
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
        newTimer->UpdateEntry(*timerIt);
        UpdateEpgEvent(newTimer);

        VecTimerInfoTag* addEntry = NULL;
        MapTags::iterator itr = m_tags.find(newTimer->StartAsUTC());
        if (itr == m_tags.end())
        {
          addEntry = new VecTimerInfoTag;
          m_tags.insert(std::make_pair(newTimer->StartAsUTC(), addEntry));
        }
        else
        {
          addEntry = itr->second;
        }

        newTimer->m_iTimerId = ++m_iLastId;
        addEntry->push_back(newTimer);
        UpdateEpgEvent(newTimer);
        bChanged = true;
        bAddedOrDeleted = true;

        if (g_PVRManager.IsStarted())
        {
          std::string strMessage;
          newTimer->GetNotificationText(strMessage);
          timerNotifications.push_back(strMessage);
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - added timer %d on client %d",
            __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
      }
    }
  }

  /* to collect timer with changed starting time */
  VecTimerInfoTag timersToMove;

  /* check for deleted timers */
  for (MapTags::iterator it = m_tags.begin(); it != m_tags.end();)
  {
    for (std::vector<CPVRTimerInfoTagPtr>::iterator it2 = it->second->begin(); it2 != it->second->end();)
    {
      CPVRTimerInfoTagPtr timer(*it2);
      if (!timers.GetByClient(timer->m_iClientId, timer->m_iClientIndex))
      {
        /* timer was not found */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        if (g_PVRManager.IsStarted())
          timerNotifications.push_back(timer->GetDeletedNotificationText());

        /** clear the EPG tag explicitly here, because it no longer happens automatically with shared pointers */
        timer->ClearEpgTag();
        it2 = it->second->erase(it2);

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
        it2 = it->second->erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else
      {
        ++it2;
      }
    }
    if (it->second->empty())
      it = m_tags.erase(it);
    else
      ++it;
  }

  /* reinsert timers with changed timer start */
  for (VecTimerInfoTag::const_iterator timerIt = timersToMove.begin(); timerIt != timersToMove.end(); ++timerIt)
  {
    VecTimerInfoTag* addEntry = NULL;
    MapTags::const_iterator itr = m_tags.find((*timerIt)->StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new VecTimerInfoTag;
      m_tags.insert(std::make_pair((*timerIt)->StartAsUTC(), addEntry));
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

    if (CSettings::Get().GetBool("pvrrecord.timernotifications"))
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

bool CPVRTimers::UpdateFromClient(const CPVRTimerInfoTagPtr &timer)
{
  CSingleLock lock(m_critSection);
  CPVRTimerInfoTagPtr tag = GetByClient(timer->m_iClientId, timer->m_iClientIndex);
  if (!tag)
  {
    tag = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag());
    VecTimerInfoTag* addEntry = NULL;
    MapTags::iterator itr = m_tags.find(timer->StartAsUTC());
    if (itr == m_tags.end())
    {
      addEntry = new VecTimerInfoTag;
      m_tags.insert(std::make_pair(timer->StartAsUTC(), addEntry));
    }
    else
    {
      addEntry = itr->second;
    }
    tag->m_iTimerId = ++m_iLastId;
    addEntry->push_back(tag);
  }

  UpdateEpgEvent(tag);

  return tag->UpdateEntry(timer);
}

/********** getters **********/

CFileItemPtr CPVRTimers::GetNextActiveTimer(void) const
{
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsRecording() && !current->IsRepeating())
      {
        CFileItemPtr fileItem(new CFileItem(current));
        return fileItem;
      }
    }
  }

  CFileItemPtr fileItem;
  return fileItem;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveTimers(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsRepeating())
      {
        CFileItemPtr fileItem(new CFileItem(current));
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

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsActive() && !(*timerIt)->IsRepeating())
        ++iReturn;

  return iReturn;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRecordings(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsRecording())
      {
        CFileItemPtr fileItem(new CFileItem(current));
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

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsRecording())
        ++iReturn;

  return iReturn;
}

bool CPVRTimers::HasActiveTimers(void) const
{
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      if ((*timerIt)->IsActive() && !(*timerIt)->IsRepeating())
        return true;

  return false;
}

bool CPVRTimers::GetRootDirectory(const CPVRTimersPath &path, CFileItemList &items) const
{
  CFileItemPtr item(new CFileItem(CPVRTimersPath::PATH_ADDTIMER, false));
  item->SetLabel(g_localizeStrings.Get(19026)); // "Add timer..."
  item->SetLabelPreformated(true);
  item->SetSpecialSort(SortSpecialOnTop);
  items.Add(item);

  bool bRadio   = path.IsRadio();
  bool bGrouped = path.IsGrouped();

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : *tagsEntry.second)
    {
      if ((bRadio == timer->m_bIsRadio) &&
          (!bGrouped || (timer->m_iParentClientIndex == 0)))
      {
        item.reset(new CFileItem(timer));
        std::string strItemPath(
          CPVRTimersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex).GetPath());
        item->SetPath(strItemPath);
        items.Add(item);
      }
    }
  }
  return true;
}

bool CPVRTimers::GetSubDirectory(const CPVRTimersPath &path, CFileItemList &items) const
{
  bool         bRadio    = path.IsRadio();
  unsigned int iParentId = path.GetParentId();
  int          iClientId = path.GetClientId();

  CFileItemPtr item;

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : *tagsEntry.second)
    {
      if ((timer->m_bIsRadio == bRadio) &&
          (timer->m_iParentClientIndex > 0) &&
          (timer->m_iClientId == iClientId) &&
          (timer->m_iParentClientIndex == iParentId))
      {
        item.reset(new CFileItem(timer));
        items.Add(item);
      }
    }
  }
  return true;
}

bool CPVRTimers::GetDirectory(const std::string& strPath, CFileItemList &items) const
{
  CPVRTimersPath path(strPath);
  if (path.IsValid())
  {
    if (path.IsTimersRoot())
    {
      /* Root folder containing both timer schedules and timers. */
      return GetRootDirectory(path, items);
    }
    else if (path.IsTimerSchedule())
    {
      /* Sub folder containing the timers scheduled by the given timer schedule. */
      return GetSubDirectory(path, items);
    }
  }

  CLog::Log(LOGERROR,"CPVRTimers - %s - invalid URL %s", __FUNCTION__, strPath.c_str());
  return false;
}

/********** channel methods **********/

bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannelPtr &channel, bool bDeleteRepeating /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  {
    CSingleLock lock(m_critSection);

    for (MapTags::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); ++it)
    {
      for (VecTimerInfoTag::iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      {
        bool bDeleteActiveItem = !bCurrentlyActiveOnly || (*timerIt)->IsRecording();
        bool bDeleteRepeatingItem = bDeleteRepeating || !(*timerIt)->IsRepeating();
        bool bChannelsMatch = (*timerIt)->ChannelTag() == channel;

        if (bDeleteActiveItem && bDeleteRepeatingItem && bChannelsMatch)
        {
          CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d", __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
          bReturn = (*timerIt)->DeleteFromClient(true, false) || bReturn;
          SetChanged();
        }
      }
    }
  }

  NotifyObservers(ObservableMessageTimersReset);

  return bReturn;
}

bool CPVRTimers::InstantTimer(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  if (!g_PVRManager.CheckParentalLock(channel))
    return false;

  CEpgInfoTagPtr epgTag(channel->GetEPGNow());
  CPVRTimerInfoTagPtr newTimer;
  if (epgTag)
    newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag);

  if (!newTimer)
  {
    newTimer.reset(new CPVRTimerInfoTag);
    /* set the timer data */
    newTimer->m_iClientIndex      = -1;
    newTimer->m_strTitle          = channel->ChannelName();
    newTimer->m_strSummary        = g_localizeStrings.Get(19056);
    newTimer->m_iChannelNumber    = channel->ChannelNumber();
    newTimer->m_iClientChannelUid = channel->UniqueID();
    newTimer->m_iClientId         = channel->ClientID();
    newTimer->m_bIsRadio          = channel->IsRadio();

    /* generate summary string */
    newTimer->m_strSummary = StringUtils::Format("%s %s %s %s %s",
                                                 newTimer->StartAsLocalTime().GetAsLocalizedDate().c_str(),
                                                 g_localizeStrings.Get(19159).c_str(),
                                                 newTimer->StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
                                                 g_localizeStrings.Get(19160).c_str(),
                                                 newTimer->EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }

  CDateTime startTime(0);
  newTimer->SetStartFromUTC(startTime);
  newTimer->m_iMarginStart = 0; /* set the start margin to 0 for instant timers */

  int iDuration = CSettings::Get().GetInt("pvrrecord.instantrecordtime");
  CDateTime endTime = CDateTime::GetUTCDateTime() + CDateTimeSpan(0, 0, iDuration ? iDuration : 120, 0);
  newTimer->SetEndFromUTC(endTime);

  /* unused only for reference */
  newTimer->m_strFileNameAndPath = CPVRTimersPath::PATH_NEW;

  bool bReturn = newTimer->AddToClient();
  if (!bReturn)
    CLog::Log(LOGERROR, "PVRTimers - %s - unable to add an instant timer on the client", __FUNCTION__);

  return bReturn;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CPVRTimerInfoTagPtr &item)
{
  if (!item->m_channel && item->GetTimerType() && !item->GetTimerType()->IsRepeatingEpgBased())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no channel given", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(19033, 19109); // Couldn't save timer
    return false;
  }

  if (!g_PVRClients->SupportsTimers(item->m_iClientId))
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19215);
    return false;
  }

  if (!g_PVRManager.CheckParentalLock(item->m_channel))
    return false;

  return item->AddToClient();
}

bool CPVRTimers::DeleteTimer(const CFileItem &item, bool bForce /* = false */, bool bDeleteSchedule /* = false */)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  const CPVRTimerInfoTagPtr tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->DeleteFromClient(bForce, bDeleteSchedule);
}

bool CPVRTimers::RenameTimer(CFileItem &item, const std::string &strNewName)
{
  /* Check if a CPVRTimerInfoTag is inside file item */
  if (!item.IsPVRTimer())
  {
    CLog::Log(LOGERROR, "PVRTimers - %s - no TimerInfoTag given", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTagPtr tag = item.GetPVRTimerInfoTag();
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

  CPVRTimerInfoTagPtr tag = item.GetPVRTimerInfoTag();
  if (!tag)
    return false;

  return tag->UpdateOnClient();
}

CPVRTimerInfoTagPtr CPVRTimers::GetByClient(int iClientId, unsigned int iClientTimerId) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
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

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
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
    const CEpgInfoTagPtr epgTag(item->GetEPGInfoTag());
    const CPVRChannelPtr channel(epgTag->ChannelTag());
    CSingleLock lock(m_critSection);

    for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    {
      for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      {
        CPVRTimerInfoTagPtr timer = *timerIt;

        if (timer->GetEpgInfoTag() == epgTag || 
            (timer->m_iClientChannelUid == channel->UniqueID() &&
            timer->m_bIsRadio == channel->IsRadio() &&
            timer->StartAsUTC() <= epgTag->StartAsUTC() &&
            timer->EndAsUTC() >= epgTag->EndAsUTC()))
        {
          CFileItemPtr fileItem(new CFileItem(timer));
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
  const bool dailywakup = CSettings::Get().GetBool("pvrpowermanagement.dailywakeup");
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(0, 0, CSettings::Get().GetInt("pvrpowermanagement.prewakeup"), 0);
  const CDateTimeSpan idle(0, 0, CSettings::Get().GetInt("pvrpowermanagement.backendidletime"), 0);

  CDateTime wakeuptime;

  /* Check next active time */
  CFileItemPtr item = GetNextActiveTimer();
  if (item && item->HasPVRTimerInfoTag())
  {
    const CDateTimeSpan prestart(0, 0, item->GetPVRTimerInfoTag()->MarginStart(), 0);
    const CDateTime start = item->GetPVRTimerInfoTag()->StartAsUTC();
    wakeuptime = ((start - prestart - prewakeup - idle) > now) ?
        start - prestart - prewakeup :
        now + idle;
  }

  /* check daily wake up */
  if (dailywakup)
  {
    CDateTime dailywakeuptime;
    dailywakeuptime.SetFromDBTime(CSettings::Get().GetString("pvrpowermanagement.dailywakeuptime"));
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
    if (!wakeuptime.IsValid() || dailywakeuptime < wakeuptime)
      wakeuptime = dailywakeuptime;
  }

  const CDateTime retVal(wakeuptime);
  return retVal;
}

void CPVRTimers::UpdateEpgEvent(CPVRTimerInfoTagPtr timer)
{
  CSingleLock lock(timer->m_critSection);

  /* repeating timers have no epg event */
  if (timer->IsRepeating())
    return;

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
  for (MapTags::iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
      (*timerIt)->UpdateChannel();
  }
}

void CPVRTimers::GetAll(CFileItemList& items) const
{
  CFileItemPtr item;
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); timerIt != it->second->end(); ++timerIt)
    {
      item.reset(new CFileItem(*timerIt));
      items.Add(item);
    }
  }
}

CPVRTimerInfoTagPtr CPVRTimers::GetById(unsigned int iTimerId) const
{
  CPVRTimerInfoTagPtr item;
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); !item && it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second->begin(); !item && timerIt != it->second->end(); ++timerIt)
    {
      if ((*timerIt)->m_iTimerId == iTimerId)
        item = *timerIt;
    }
  }
  return item;
}

//= CPVRTimersPath ============================================================

const std::string CPVRTimersPath::PATH_ADDTIMER = "pvr://timers/addtimer/";
const std::string CPVRTimersPath::PATH_NEW      = "pvr://timers/new/";

CPVRTimersPath::CPVRTimersPath(const std::string &strPath)
{
  Init(strPath);
}

CPVRTimersPath::CPVRTimersPath(const std::string &strPath, int iClientId, unsigned int iParentId)
{
  if (Init(strPath))
  {
    /* set/replace client and parent id. */
    m_path = StringUtils::Format("pvr://timers/%s/%s/%d/%d",
                                 m_bRadio   ? "radio"   : "tv",
                                 m_bGrouped ? "grouped" : "all",
                                 iClientId,
                                 iParentId);
    m_iClientId = iClientId;
    m_iParentId = iParentId;
    m_bRoot = false;
  }
}

CPVRTimersPath::CPVRTimersPath(bool bRadio, bool bGrouped) :
  m_path(StringUtils::Format(
    "pvr://timers/%s/%s", bRadio ? "radio" : "tv", bGrouped ? "grouped" : "all")),
  m_bValid(true),
  m_bRoot(true),
  m_bRadio(bRadio),
  m_bGrouped(bGrouped),
  m_iClientId(-1),
  m_iParentId(0)
{
}

bool CPVRTimersPath::Init(const std::string &strPath)
{
  std::string strVarPath(strPath);
  URIUtils::RemoveSlashAtEnd(strVarPath);

  m_path = strVarPath;
  const std::vector<std::string> segments = URIUtils::SplitPath(m_path);

  m_bValid   = (((segments.size() == 4) || (segments.size() == 6)) &&
                (segments.at(1) == "timers") &&
                ((segments.at(2) == "radio") || (segments.at(2) == "tv"))&&
                ((segments.at(3) == "grouped") || (segments.at(3) == "all")));
  m_bRoot    = (m_bValid && (segments.size() == 4));
  m_bRadio   = (m_bValid && (segments.at(2) == "radio"));
  m_bGrouped = (m_bValid && (segments.at(3) == "grouped"));

  if (!m_bValid || m_bRoot)
  {
    m_iClientId = -1;
    m_iParentId = 0;
  }
  else
  {
    char *end;
    m_iClientId = std::strtol (segments.at(4).c_str(), &end, 10);
    m_iParentId = std::strtoul(segments.at(5).c_str(), &end, 10);
  }

  return m_bValid;
}
