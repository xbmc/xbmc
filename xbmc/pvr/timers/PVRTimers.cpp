/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimers.h"

#include <utility>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

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
    it->second.emplace_back(newTimer);
  }
}

CPVRTimers::CPVRTimers(void)
: m_settings({
    CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP,
    CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP,
    CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME,
    CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
    CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS
  })
{
}

CPVRTimers::~CPVRTimers(void)
{
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

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating timers");
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
        if (existingTimer->UpdateEntry(*timerIt))
        {
          bChanged = true;
          existingTimer->ResetChildState();

          if (bStateChanged)
          {
            std::string strMessage;
            existingTimer->GetNotificationText(strMessage);
            timerNotifications.push_back(std::make_pair((*timerIt)->m_iClientId, strMessage));
          }

          CLog::LogFC(LOGDEBUG, LOGPVR, "Updated timer %d on client %d",
                      (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
        }
      }
      else
      {
        /* new timer */
        CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTagPtr(new CPVRTimerInfoTag);
        newTimer->UpdateEntry(*timerIt);
        newTimer->m_iTimerId = ++m_iLastId;
        InsertTimer(newTimer);

        bChanged = true;
        bAddedOrDeleted = true;

         std::string strMessage;
         newTimer->GetNotificationText(strMessage);
         timerNotifications.push_back(std::make_pair(newTimer->m_iClientId, strMessage));

        CLog::LogFC(LOGDEBUG, LOGPVR, "Added timer %d on client %d",
                    (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
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

        CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted timer %d on client %d",
                    timer->m_iClientIndex, timer->m_iClientId);

        timerNotifications.push_back(std::make_pair(timer->m_iClientId, timer->GetDeletedNotificationText()));

        it2 = it->second.erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else if ((timer->m_bStartAnyTime && it->first != CDateTime()) ||
               (!timer->m_bStartAnyTime && timer->StartAsUTC() != it->first))
      {
        /* timer start has changed */
        CLog::LogFC(LOGDEBUG, LOGPVR, "Changed start time timer %d on client %d",
                    timer->m_iClientIndex, timer->m_iClientId);

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
    InsertTimer(*timerIt);

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
        const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(entry.first);
        if (client)
        {
          job->AddEvent(m_settings.GetBoolValue(CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS),
                        false, // info, no error
                        client->Name(),
                        entry.second,
                        client->Icon());
        }
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

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTimer(const TimerKind &eKind) const
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
        return timersEntry;
    }
  }

  return std::shared_ptr<CPVRTimerInfoTag>();
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTimer(void) const
{
  return GetNextActiveTimer(TimerKindAny);
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTVTimer(void) const
{
  return GetNextActiveTimer(TimerKindTV);
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveRadioTimer(void) const
{
  return GetNextActiveTimer(TimerKindRadio);
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveTimers(void) const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags;
  CSingleLock lock(m_critSection);

  for (MapTags::const_iterator it = m_tags.begin(); it != m_tags.end(); ++it)
  {
    for (VecTimerInfoTag::const_iterator timerIt = it->second.begin(); timerIt != it->second.end(); ++timerIt)
    {
      CPVRTimerInfoTagPtr current = *timerIt;
      if (current->IsActive() && !current->IsTimerRule() && !current->IsBroken())
      {
        tags.emplace_back(current);
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
          !timersEntry->IsTimerRule() &&
          !timersEntry->IsBroken())
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

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveRecordings(const TimerKind& eKind) const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags;
  CSingleLock lock(m_critSection);

  for (const auto &tagsEntry : m_tags)
  {
    for (const auto &timersEntry : tagsEntry.second)
    {
      if (KindMatchesTag(eKind, timersEntry) &&
          timersEntry->IsRecording() &&
          !timersEntry->IsTimerRule() &&
          !timersEntry->IsBroken())
      {
        tags.emplace_back(timersEntry);
      }
    }
  }

  return tags;
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveRecordings() const
{
  return GetActiveRecordings(TimerKindAny);
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveTVRecordings() const
{
  return GetActiveRecordings(TimerKindTV);
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveRadioRecordings() const
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
          !timersEntry->IsTimerRule() &&
          !timersEntry->IsBroken())
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
      if ((*timerIt)->IsActive() && !(*timerIt)->IsTimerRule() && !(*timerIt)->IsBroken())
        return true;

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
        bool bChannelsMatch = (*timerIt)->HasChannel() && (*timerIt)->Channel() == channel;

        if (bDeleteActiveItem && bDeleteTimerRuleItem && bChannelsMatch)
        {
          CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted timer %d on client %d",
                      (*timerIt)->m_iClientIndex, (*timerIt)->m_iClientId);
          bReturn = ((*timerIt)->DeleteFromClient(true) == TimerOperationResult::OK) || bReturn;
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

bool CPVRTimers::AddTimer(const CPVRTimerInfoTagPtr &tag)
{
  return tag->AddToClient();
}

TimerOperationResult CPVRTimers::DeleteTimer(const CPVRTimerInfoTagPtr &tag, bool bForce /* = false */, bool bDeleteRule /* = false */)
{
  if (!tag)
    return TimerOperationResult::FAILED;

  if (bDeleteRule)
  {
    /* delete the timer rule that scheduled this timer. */
    CPVRTimerInfoTagPtr ruleTag = CServiceBroker::GetPVRManager().Timers()->GetByClient(tag->m_iClientId, tag->GetTimerRuleId());
    if (!ruleTag)
    {
      CLog::LogF(LOGERROR, "Unable to obtain timer rule for given timer");
      return TimerOperationResult::FAILED;
    }

    return ruleTag->DeleteFromClient(bForce);
  }

  return tag->DeleteFromClient(bForce);
}

bool CPVRTimers::RenameTimer(const CPVRTimerInfoTagPtr &tag, const std::string &strNewName)
{
  return tag->RenameOnClient(strNewName);
}

bool CPVRTimers::UpdateTimer(const CPVRTimerInfoTagPtr &tag)
{
  return tag->UpdateOnClient();
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
            timersEntry->m_iClientChannelUid == epgTag->UniqueChannelID())
        {
          if (timersEntry->UniqueBroadcastID() != EPG_TAG_INVALID_UID &&
              timersEntry->UniqueBroadcastID() == epgTag->UniqueBroadcastID())
            return timersEntry;

          if (timersEntry->m_bIsRadio == epgTag->IsRadio() &&
              timersEntry->StartAsUTC() <= epgTag->StartAsUTC() &&
              timersEntry->EndAsUTC() >= epgTag->EndAsUTC())
            return timersEntry;
        }
      }
    }
  }

  return CPVRTimerInfoTagPtr();
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
    timer = GetTimerForEpgTag(item->GetEPGInfoTag());
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
  const std::shared_ptr<CPVRTimerInfoTag> timer = GetNextActiveTimer();
  if (timer)
  {
    const CDateTimeSpan prestart(0, 0, timer->MarginStart(), 0);
    const CDateTime start = timer->StartAsUTC();
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

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetAll() const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> timers;

  CSingleLock lock(m_critSection);
  for (const auto& tagsEntry : m_tags)
  {
    for (const auto& timer : tagsEntry.second)
    {
      timers.emplace_back(timer);
    }
  }

  return timers;
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
