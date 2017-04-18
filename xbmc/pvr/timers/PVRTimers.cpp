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

#include "PVRTimers.h"

#include <cassert>
#include <cstdlib>
#include <utility>

#include "ServiceBroker.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "pvr/PVRJobs.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"

using namespace PVR;

bool CPVRTimersContainer::UpdateFromClient(const CPVRTimerInfoTagPtr &timer)
{
  CSingleLock lock(m_critSection);
  CPVRTimerInfoTagPtr tag = GetByClient(timer->m_iClientId, timer->m_iClientIndex);
  if (!tag)
  {
    tag.reset(new CPVRTimerInfoTag());
    tag->m_iTimerId = ++m_iLastId;
    InsertTimer(tag);
  }

  return tag->UpdateEntry(timer);
}

CPVRTimerInfoTagPtr CPVRTimersContainer::GetByClient(int iClientId, unsigned int iClientTimerId) const
{
  CSingleLock lock(m_critSection);
  for (const auto startDates : m_tags)
  {
    for (const auto timer : startDates.second)
    {
      if (timer->m_iClientId == iClientId && timer->m_iClientIndex == iClientTimerId)
      {
        return timer;
      }
    }
  }

  return CPVRTimerInfoTagPtr();
}

void CPVRTimersContainer::InsertTimer(const CPVRTimerInfoTagPtr &newTimer)
{
  auto it = m_tags.find(newTimer->m_bStartAnyTime ? CDateTime() : newTimer->StartAsUTC());
  if (it == m_tags.end())
  {
    VecTimerInfoTag addEntry({newTimer});
    m_tags.insert(std::make_pair(newTimer->m_bStartAnyTime ? CDateTime() : newTimer->StartAsUTC(), addEntry));
  }
  else
  {
    it->second.emplace_back(newTimer);;
  }
}

CPVRTimers::CPVRTimers(void)
: m_bIsUpdating(false),
  m_settings({
    CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP,
    CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP,
    CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME,
    CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
    CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS,
    CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS
  })
{
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
  CServiceBroker::GetPVRManager().EpgContainer().RegisterObserver(this);

  // update from clients
  return Update();
}

void CPVRTimers::Unload()
{
  // unregister observer
  CServiceBroker::GetPVRManager().EpgContainer().UnregisterObserver(this);

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
  CPVRTimersContainer newTimerList;
  std::vector<int> failedClients;
  CServiceBroker::GetPVRManager().Clients()->GetTimers(&newTimerList, failedClients);
  return UpdateEntries(newTimerList, failedClients);
}

bool CPVRTimers::IsRecording(void) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
      if ((*timerIt)->IsRecording())
        return true;

  return false;
}

bool CPVRTimers::SetEpgTagTimer(const CPVRTimerInfoTagPtr &timer)
{
  if (timer->IsTimerRule() || timer->m_bStartAnyTime || timer->m_bEndAnyTime)
    return false;

  std::vector<CPVREpgInfoTagPtr> tags(CServiceBroker::GetPVRManager().EpgContainer().GetEpgTagsForTimer(timer));

  if (tags.empty())
    return false;

  // assign first matching epg tag to the timer.
  timer->SetEpgTag(tags.front());

  // assign timer to every matching epg tag.
  for (const auto &tag : tags)
    tag->SetTimer(timer);

  return true;
}

bool CPVRTimers::ClearEpgTagTimer(const CPVRTimerInfoTagPtr &timer)
{
  if (timer->IsTimerRule() || timer->m_bStartAnyTime || timer->m_bEndAnyTime)
    return false;

  std::vector<CPVREpgInfoTagPtr> tags(CServiceBroker::GetPVRManager().EpgContainer().GetEpgTagsForTimer(timer));

  if (tags.empty())
    return false;

  for (const auto &tag : tags)
    tag->ClearTimer();

  return true;
}

bool CPVRTimers::UpdateEntries(const CPVRTimersContainer &timers, const std::vector<int> &failedClients)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  std::vector< std::pair< int, std::string> > timerNotifications;

  CSingleLock lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (MapTags::const_iterator it = timers.GetTags().begin(); it != timers.GetTags().end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
    {
      /* check if this timer is present in this container */
      CPVRTimerInfoTagPtr existingTimer = GetByClient((*timerIt)->m_iClientId, (*timerIt)->m_iClientIndex);
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->m_state != (*timerIt)->m_state);
        ClearEpgTagTimer(existingTimer);
        if (existingTimer->UpdateEntry(*timerIt))
        {
          SetEpgTagTimer(existingTimer);

          bChanged = true;
          existingTimer->ResetChildState();

          if (bStateChanged)
          {
            std::string strMessage;
            existingTimer->GetNotificationText(strMessage);
            timerNotifications.push_back(std::make_pair((*timerIt)->m_iClientId, strMessage));
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
        newTimer->m_iTimerId = ++m_iLastId;
        SetEpgTagTimer(newTimer);
        InsertTimer(newTimer);

        bChanged = true;
        bAddedOrDeleted = true;

         std::string strMessage;
         newTimer->GetNotificationText(strMessage);
         timerNotifications.push_back(std::make_pair(newTimer->m_iClientId, strMessage));

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
    for (std::vector<CPVRTimerInfoTagPtr>::iterator it2 = it->second.begin(); it2 != it->second.end();)
    {
      CPVRTimerInfoTagPtr timer(*it2);
      if (!timers.GetByClient(timer->m_iClientId, timer->m_iClientIndex))
      {
        /* timer was not found */
        bool bIgnoreTimer(false);
        for (const auto &failedClient : failedClients)
        {
          if (failedClient == timer->m_iClientId)
          {
            bIgnoreTimer = true;
            break;
          }
        }

        if (bIgnoreTimer)
        {
          ++it2;
          continue;
        }

        CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        timerNotifications.push_back(std::make_pair(timer->m_iClientId, timer->GetDeletedNotificationText()));

        ClearEpgTagTimer(timer);

        it2 = it->second.erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else if ((timer->m_bStartAnyTime && it->first != CDateTime()) ||
               (!timer->m_bStartAnyTime && timer->StartAsUTC() != it->first))
      {
        /* timer start has changed */
        CLog::Log(LOGDEBUG,"PVRTimers - %s - changed start time timer %d on client %d",
            __FUNCTION__, timer->m_iClientIndex, timer->m_iClientId);

        ClearEpgTagTimer(timer);

        /* remember timer */
        timersToMove.push_back(timer);

        /* remove timer for now, reinsert later */
        it2 = it->second.erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else
      {
        ++it2;
      }
    }
    if (it->second.empty())
      it = m_tags.erase(it);
    else
      ++it;
  }

  /* reinsert timers with changed timer start */
  for (VecTimerInfoTag::const_iterator timerIt = timersToMove.begin(); timerIt != timersToMove.end(); ++timerIt)
  {
    SetEpgTagTimer(*timerIt);
    InsertTimer(*timerIt);
  }

  /* update child information for all parent timers */
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (timersEntry->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
      {
        const CPVRTimerInfoTagPtr parentTimer(GetByClient(timersEntry->m_iClientId, timersEntry->GetTimerRuleId()));
        if (parentTimer)
          parentTimer->UpdateChildState(timersEntry);
      }
    }
  }

  m_bIsUpdating = false;
  if (bChanged)
  {
    UpdateChannels();
    lock.Leave();

    CServiceBroker::GetPVRManager().SetChanged();
    CServiceBroker::GetPVRManager().NotifyObservers(bAddedOrDeleted ? ObservableMessageTimersReset : ObservableMessageTimers);

    if (!timerNotifications.empty() && CServiceBroker::GetPVRManager().IsStarted())
    {
      CPVREventlogJob *job = new CPVREventlogJob;

      /* queue notifications / fill eventlog */
      for (const auto &entry : timerNotifications)
      {
        std::string strName;
        CServiceBroker::GetPVRManager().Clients()->GetClientAddonName(entry.first, strName);

        std::string strIcon;
        CServiceBroker::GetPVRManager().Clients()->GetClientAddonIcon(entry.first, strIcon);

        job->AddEvent(m_settings.GetBoolValue(CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS),
                      false, // info, no error
                      strName,
                      entry.second,
                      strIcon);
      }

      CJobManager::GetInstance().AddJob(job, nullptr);
    }
  }

  return bChanged;
}

bool CPVRTimers::KindMatchesTag(const TimerKind &eKind, const CPVRTimerInfoTagPtr &tag) const
{
  return (eKind == TimerKindAny) ||
         (eKind == TimerKindTV && !tag->m_bIsRadio) ||
         (eKind == TimerKindRadio && tag->m_bIsRadio);
}

CFileItemPtr CPVRTimers::GetNextActiveTimer(const TimerKind &eKind) const
{
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (KindMatchesTag(eKind, timersEntry) &&
          timersEntry->IsActive() &&
          !timersEntry->IsRecording() &&
          !timersEntry->IsTimerRule() &&
          !timersEntry->IsBroken())
        return CFileItemPtr(new CFileItem(timersEntry));
    }
  }

  return CFileItemPtr();
}

CFileItemPtr CPVRTimers::GetNextActiveTimer(void) const
{
  return GetNextActiveTimer(TimerKindAny);
}

CFileItemPtr CPVRTimers::GetNextActiveTVTimer(void) const
{
  return GetNextActiveTimer(TimerKindTV);
}

CFileItemPtr CPVRTimers::GetNextActiveRadioTimer(void) const
{
  return GetNextActiveTimer(TimerKindRadio);
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveTimers(void) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsTimerRule())
      {
        CFileItemPtr fileItem(new CFileItem(current));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

int CPVRTimers::AmountActiveTimers(const TimerKind &eKind) const
{
  int iReturn = 0;
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (KindMatchesTag(eKind, timersEntry) &&
          timersEntry->IsActive() &&
          !timersEntry->IsTimerRule())
        ++iReturn;
    }
  }

  return iReturn;
}

int CPVRTimers::AmountActiveTimers(void) const
{
  return AmountActiveTimers(TimerKindAny);
}

int CPVRTimers::AmountActiveTVTimers(void) const
{
  return AmountActiveTimers(TimerKindTV);
}

int CPVRTimers::AmountActiveRadioTimers(void) const
{
  return AmountActiveTimers(TimerKindRadio);
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRecordings(const TimerKind &eKind) const
{
  std::vector<CFileItemPtr> tags;
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (KindMatchesTag(eKind, timersEntry) &&
          timersEntry->IsRecording() &&
          !timersEntry->IsTimerRule())
      {
        CFileItemPtr fileItem(new CFileItem(timersEntry));
        tags.push_back(fileItem);
      }
    }
  }

  return tags;
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRecordings(void) const
{
  return GetActiveRecordings(TimerKindAny);
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveTVRecordings(void) const
{
  return GetActiveRecordings(TimerKindTV);
}

std::vector<CFileItemPtr> CPVRTimers::GetActiveRadioRecordings(void) const
{
  return GetActiveRecordings(TimerKindRadio);
}

int CPVRTimers::AmountActiveRecordings(const TimerKind &eKind) const
{
  int iReturn = 0;
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (KindMatchesTag(eKind, timersEntry) &&
          timersEntry->IsRecording() &&
          !timersEntry->IsTimerRule())
        ++iReturn;
    }
  }

  return iReturn;
}

int CPVRTimers::AmountActiveRecordings(void) const
{
  return AmountActiveRecordings(TimerKindAny);
}

int CPVRTimers::AmountActiveTVRecordings(void) const
{
  return AmountActiveRecordings(TimerKindTV);
}

int CPVRTimers::AmountActiveRadioRecordings(void) const
{
  return AmountActiveRecordings(TimerKindRadio);
}

bool CPVRTimers::HasActiveTimers(void) const
{
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
      if ((*timerIt)->IsActive() && !(*timerIt)->IsTimerRule())
        return true;

  return false;
}

bool CPVRTimers::GetRootDirectory(const CPVRTimersPath &path, CFileItemList &items) const
{
  CFileItemPtr item(new CFileItem(CPVRTimersPath::PATH_ADDTIMER, false));
  item->SetLabel(g_localizeStrings.Get(19026)); // "Add timer..."
  item->SetLabelPreformatted(true);
  item->SetSpecialSort(SortSpecialOnTop);
  items.Add(item);

  bool bRadio = path.IsRadio();
  bool bRules = path.IsRules();

  bool bHideDisabled = m_settings.GetBoolValue(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : tagsEntry.second)
    {
      if ((bRadio == timer->m_bIsRadio) &&
          (bRules == timer->IsTimerRule()) &&
          (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
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

  bool bHideDisabled = m_settings.GetBoolValue(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  CFileItemPtr item;

  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timer : tagsEntry.second)
    {
      if ((timer->m_bIsRadio == bRadio) &&
          (timer->m_iParentClientIndex != PVR_TIMER_NO_PARENT) &&
          (timer->m_iClientId == iClientId) &&
          (timer->m_iParentClientIndex == iParentId) &&
          (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
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

bool CPVRTimers::GetDirectory(const std::string& strPath, CFileItemList &items) const
{
  CPVRTimersPath path(strPath);
  if (path.IsValid())
  {
    if (path.IsTimersRoot())
    {
      /* Root folder containing either timer rules or timers. */
      return GetRootDirectory(path, items);
    }
    else if (path.IsTimerRule())
    {
      /* Sub folder containing the timers scheduled by the given timer rule. */
      return GetSubDirectory(path, items);
    }
  }

  CLog::Log(LOGERROR,"CPVRTimers - %s - invalid URL %s", __FUNCTION__, strPath.c_str());
  return false;
}

/********** channel methods **********/

bool CPVRTimers::DeleteTimersOnChannel(const CPVRChannelPtr &channel, bool bDeleteTimerRules /* = true */, bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  bool bChanged = false;
  {
    CSingleLock lock(m_critSection);

    for (MapTags::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); ++it)
    {
      for (VecTimerInfoTag::iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
      {
        bool bDeleteActiveItem = !bCurrentlyActiveOnly || (*timerIt)->IsRecording();
        bool bDeleteTimerRuleItem = bDeleteTimerRules || !(*timerIt)->IsTimerRule();
        bool bChannelsMatch = (*timerIt)->ChannelTag() == channel;

        if (bDeleteActiveItem && bDeleteTimerRuleItem && bChannelsMatch)
        {
          CLog::Log(LOGDEBUG,"PVRTimers - %s - deleted timer %d on client %d", __FUNCTION__, (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
          bReturn = (*timerIt)->DeleteFromClient(true) || bReturn;
          bChanged = true;
        }
      }
    }
  }

  if (bChanged)
    CServiceBroker::GetPVRManager().SetChanged();

  CServiceBroker::GetPVRManager().NotifyObservers(ObservableMessageTimersReset);

  return bReturn;
}

/********** static methods **********/

bool CPVRTimers::AddTimer(const CPVRTimerInfoTagPtr &item)
{
  return item->AddToClient();
}

bool CPVRTimers::DeleteTimer(const CPVRTimerInfoTagPtr &tag, bool bForce /* = false */, bool bDeleteRule /* = false */)
{
  if (!tag)
    return false;

  if (bDeleteRule)
  {
    /* delete the timer rule that scheduled this timer. */
    CPVRTimerInfoTagPtr ruleTag = CServiceBroker::GetPVRManager().Timers()->GetByClient(tag->m_iClientId, tag->GetTimerRuleId());
    if (!ruleTag)
    {
      CLog::Log(LOGERROR, "PVRTimers - %s - unable to obtain timer rule for given timer", __FUNCTION__);
      return false;
    }
    return ruleTag->DeleteFromClient(bForce);
  }

  return tag->DeleteFromClient(bForce);
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

bool CPVRTimers::UpdateTimer(const CPVRTimerInfoTagPtr &item)
{
  return item->UpdateOnClient();
}

bool CPVRTimers::IsRecordingOnChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
    {
      if ((*timerIt)->IsRecording() &&
          (*timerIt)->m_iClientChannelUid == channel.UniqueID() &&
          (*timerIt)->m_iClientId == channel.ClientID())
        return true;
    }
  }

  return false;
}

CPVRTimerInfoTagPtr CPVRTimers::GetActiveTimerForChannel(const CPVRChannelPtr &channel) const
{
  CSingleLock lock(m_critSection);
  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (timersEntry->IsRecording() &&
          timersEntry->m_iClientChannelUid == channel->UniqueID() &&
          timersEntry->m_iClientId == channel->ClientID())
        return timersEntry;
    }
  }

  return CPVRTimerInfoTagPtr();
}

CPVRTimerInfoTagPtr CPVRTimers::GetTimerForEpgTag(const CPVREpgInfoTagPtr &epgTag) const
{
  if (epgTag)
  {
    // already a timer assigned to tag?
    const CPVRTimerInfoTagPtr timer(epgTag->Timer());
    if (timer)
      return timer;

    // try to find a matching timer for the tag.
    const CPVRChannelPtr channel(epgTag->ChannelTag());
    if (channel)
    {
      CSingleLock lock(m_critSection);

      for (const auto &tagsEntry : m_tags)
      {
        for (const auto &timersEntry : tagsEntry.second)
        {
          if (timersEntry->IsTimerRule())
            continue;

          if (timersEntry->GetEpgInfoTag(false) == epgTag)
            return timersEntry;

          if (timersEntry->m_iClientChannelUid != PVR_CHANNEL_INVALID_UID &&
              timersEntry->m_iClientChannelUid == channel->UniqueID())
          {
            if (timersEntry->m_iEpgUid != EPG_TAG_INVALID_UID &&
                timersEntry->m_iEpgUid == epgTag->UniqueBroadcastID())
              return timersEntry;

            if (timersEntry->m_bIsRadio == channel->IsRadio() &&
                timersEntry->StartAsUTC() <= epgTag->StartAsUTC() &&
                timersEntry->EndAsUTC() >= epgTag->EndAsUTC())
              return timersEntry;
          }
        }
      }
    }
  }

  return CPVRTimerInfoTagPtr();
}

bool CPVRTimers::HasRecordingTimerForRecording(const CPVRRecording &recording) const
{
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (timersEntry->IsRecording() &&
          !timersEntry->IsTimerRule() &&
          timersEntry->m_iClientId == recording.ClientID() &&
          timersEntry->ChannelTag()->UniqueID() == recording.ChannelUid() &&
          timersEntry->StartAsUTC() <= recording.RecordingTimeAsUTC() &&
          timersEntry->EndAsUTC() >= recording.EndTimeAsUTC())
      {
        return true;
      }
    }
  }

  return false;
}

CPVRTimerInfoTagPtr CPVRTimers::GetTimerRule(const CPVRTimerInfoTagPtr &timer) const
{
  if (timer)
  {
    unsigned int iRuleId = timer->GetTimerRuleId();
    if (iRuleId != PVR_TIMER_NO_PARENT)
    {
      int iClientId = timer->m_iClientId;

      CSingleLock lock(m_critSection);
      for (const auto &tagsEntry : m_tags)
      {
        for (const auto &timersEntry : tagsEntry.second)
        {
          if (timersEntry->m_iClientId == iClientId && timersEntry->m_iClientIndex == iRuleId)
            return timersEntry;
        }
      }
    }
  }
  return CPVRTimerInfoTagPtr();
}

CFileItemPtr CPVRTimers::GetTimerRule(const CFileItemPtr &item) const
{
  CPVRTimerInfoTagPtr timer;
  if (item && item->HasEPGInfoTag())
    timer = item->GetEPGInfoTag()->Timer();
  else if (item && item->HasPVRTimerInfoTag())
    timer = item->GetPVRTimerInfoTag();

  if (timer)
  {
    timer = GetTimerRule(timer);
    if (timer)
      return CFileItemPtr(new CFileItem(timer));
  }
  return CFileItemPtr();
}

void CPVRTimers::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageEpgContainer)
    CServiceBroker::GetPVRManager().TriggerTimersUpdate();
}

CDateTime CPVRTimers::GetNextEventTime(void) const
{
  const bool dailywakup = m_settings.GetBoolValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP);
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP), 0);
  const CDateTimeSpan idle(0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);

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
    dailywakeuptime.SetFromDBTime(m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
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

void CPVRTimers::UpdateChannels(void)
{
  CSingleLock lock(m_critSection);
  for (MapTags::iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
      (*timerIt)->UpdateChannel();
  }
}

void CPVRTimers::GetAll(CFileItemList& items) const
{
  CFileItemPtr item;
  CSingleLock lock(m_critSection);
  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
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
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); !item && timerIt != it->second.end(); ++timerIt)
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
                                 m_bRadio      ? "radio" : "tv",
                                 m_bTimerRules ? "rules" : "timers",
                                 iClientId,
                                 iParentId);
    m_iClientId = iClientId;
    m_iParentId = iParentId;
    m_bRoot = false;
  }
}

CPVRTimersPath::CPVRTimersPath(bool bRadio, bool bTimerRules) :
  m_path(StringUtils::Format(
    "pvr://timers/%s/%s", bRadio ? "radio" : "tv", bTimerRules ? "rules" : "timers")),
  m_bValid(true),
  m_bRoot(true),
  m_bRadio(bRadio),
  m_bTimerRules(bTimerRules),
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
                ((segments.at(3) == "rules") || (segments.at(3) == "timers")));
  m_bRoot    = (m_bValid && (segments.size() == 4));
  m_bRadio   = (m_bValid && (segments.at(2) == "radio"));
  m_bTimerRules = (m_bValid && (segments.at(3) == "rules"));

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
