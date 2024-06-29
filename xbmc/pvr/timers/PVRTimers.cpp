/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimers.h"

#include "ServiceBroker.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerRuleMatcher.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace std::chrono_literals;

namespace
{
constexpr auto MAX_NOTIFICATION_DELAY = 10s;
}

bool CPVRTimersContainer::UpdateFromClient(const std::shared_ptr<CPVRTimerInfoTag>& timer)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::shared_ptr<CPVRTimerInfoTag> tag = GetByClient(timer->ClientID(), timer->ClientIndex());
  if (tag)
  {
    return tag->UpdateEntry(timer);
  }
  else
  {
    timer->SetTimerID(++m_iLastId);
    InsertEntry(timer);
  }

  return true;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimersContainer::GetByClient(int iClientId,
                                                                   int iClientIndex) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& startDates : m_tags)
  {
    const auto it = std::find_if(startDates.second.cbegin(), startDates.second.cend(),
                                 [iClientId, iClientIndex](const auto& timer) {
                                   return timer->ClientID() == iClientId &&
                                          timer->ClientIndex() == iClientIndex;
                                 });
    if (it != startDates.second.cend())
      return (*it);
  }

  return {};
}

void CPVRTimersContainer::InsertEntry(const std::shared_ptr<CPVRTimerInfoTag>& newTimer)
{
  auto it = m_tags.find(newTimer->IsStartAnyTime() ? CDateTime() : newTimer->StartAsUTC());
  if (it == m_tags.end())
  {
    VecTimerInfoTag addEntry({newTimer});
    m_tags.insert(std::make_pair(newTimer->IsStartAnyTime() ? CDateTime() : newTimer->StartAsUTC(),
                                 addEntry));
  }
  else
  {
    it->second.emplace_back(newTimer);
  }
}

CPVRTimers::CPVRTimers()
  : CThread("PVRTimers"),
    m_settings({CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP,
                CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP,
                CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME,
                CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
                CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS})
{
}

bool CPVRTimers::Update(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  return LoadFromDatabase(clients) && UpdateFromClients(clients);
}

bool CPVRTimers::LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  // load local timers from database
  const std::shared_ptr<const CPVRDatabase> database =
      CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
  {
    const std::vector<std::shared_ptr<CPVRTimerInfoTag>> timers =
        database->GetTimers(*this, clients);

    if (std::accumulate(timers.cbegin(), timers.cend(), false,
                        [this](bool changed, const auto& timer) {
                          return (UpdateEntry(timer) != nullptr) ? true : changed;
                        }))
      NotifyTimersEvent();
  }

  // ensure that every timer has its channel set
  UpdateChannels();
  return true;
}

void CPVRTimers::Unload()
{
  // remove all tags
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_tags.clear();
}

void CPVRTimers::Start()
{
  Stop();

  CServiceBroker::GetPVRManager().Events().Subscribe(this, &CPVRTimers::Notify);
  Create();
}

void CPVRTimers::Stop()
{
  StopThread();
  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
}

bool CPVRTimers::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (m_bIsUpdating)
      return false;
    m_bIsUpdating = true;
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating timer types");
  std::vector<int> failedClients;
  CServiceBroker::GetPVRManager().Clients()->UpdateTimerTypes(clients, failedClients);

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating timers");
  CPVRTimersContainer newTimerList;
  CServiceBroker::GetPVRManager().Clients()->GetTimers(clients, &newTimerList, failedClients);
  return UpdateEntries(newTimerList, failedClients);
}

void CPVRTimers::Process()
{
  while (!m_bStop)
  {
    // update all timers not owned by a client (e.g. reminders)
    UpdateEntries(MAX_NOTIFICATION_DELAY.count());

    CThread::Sleep(MAX_NOTIFICATION_DELAY);
  }
}

bool CPVRTimers::IsRecording() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    if (std::any_of(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                    [](const auto& timersEntry) { return timersEntry->IsRecording(); }))
      return true;
  }

  return false;
}

void CPVRTimers::RemoveEntry(const std::shared_ptr<const CPVRTimerInfoTag>& tag)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  auto it = m_tags.find(tag->IsStartAnyTime() ? CDateTime() : tag->StartAsUTC());
  if (it != m_tags.end())
  {
    it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
                                    [&tag](const std::shared_ptr<const CPVRTimerInfoTag>& timer) {
                                      return tag->ClientID() == timer->ClientID() &&
                                             tag->ClientIndex() == timer->ClientIndex();
                                    }),
                     it->second.end());

    if (it->second.empty())
      m_tags.erase(it);
  }
}

bool CPVRTimers::CheckAndAppendTimerNotification(
    std::vector<std::pair<int, std::string>>& timerNotifications,
    const std::shared_ptr<const CPVRTimerInfoTag>& tag,
    bool bDeleted) const
{
  // no notification on first update or if previous update failed for tag's client.
  if (!m_bFirstUpdate && std::find(m_failedClients.cbegin(), m_failedClients.cend(),
                                   tag->ClientID()) == m_failedClients.cend())
  {
    const std::string strMessage =
        bDeleted ? tag->GetDeletedNotificationText() : tag->GetNotificationText();
    timerNotifications.emplace_back(tag->ClientID(), strMessage);
    return true;
  }
  return false;
}

bool CPVRTimers::UpdateEntries(const CPVRTimersContainer& timers,
                               const std::vector<int>& failedClients)
{
  bool bChanged(false);
  bool bAddedOrDeleted(false);
  std::vector<std::pair<int, std::string>> timerNotifications;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  /* go through the timer list and check for updated or new timers */
  for (const auto& tagsEntry : timers.GetTags())
  {
    for (const auto& timersEntry : tagsEntry.second)
    {
      /* check if this timer is present in this container */
      const std::shared_ptr<CPVRTimerInfoTag> existingTimer =
          GetByClient(timersEntry->ClientID(), timersEntry->ClientIndex());
      if (existingTimer)
      {
        /* if it's present, update the current tag */
        bool bStateChanged(existingTimer->State() != timersEntry->State());
        if (existingTimer->UpdateEntry(timersEntry))
        {
          bChanged = true;
          existingTimer->ResetChildState();

          if (bStateChanged)
            CheckAndAppendTimerNotification(timerNotifications, existingTimer, false);

          CLog::LogFC(LOGDEBUG, LOGPVR, "Updated timer {} on client {}", timersEntry->ClientIndex(),
                      timersEntry->ClientID());
        }
      }
      else
      {
        /* new timer */
        std::shared_ptr<CPVRTimerInfoTag> newTimer = std::make_shared<CPVRTimerInfoTag>();
        newTimer->UpdateEntry(timersEntry);
        newTimer->SetTimerID(++m_iLastId);
        InsertEntry(newTimer);

        bChanged = true;
        bAddedOrDeleted = true;

        CheckAndAppendTimerNotification(timerNotifications, newTimer, false);

        CLog::LogFC(LOGDEBUG, LOGPVR, "Added timer {} on client {}", timersEntry->ClientIndex(),
                    timersEntry->ClientID());
      }
    }
  }

  /* to collect timer with changed starting time */
  VecTimerInfoTag timersToMove;

  /* check for deleted timers */
  for (auto it = m_tags.begin(); it != m_tags.end();)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end();)
    {
      const std::shared_ptr<CPVRTimerInfoTag> timer = *it2;
      if (!timers.GetByClient(timer->ClientID(), timer->ClientIndex()))
      {
        /* timer was not found */
        bool bIgnoreTimer = !timer->IsOwnedByClient();
        if (!bIgnoreTimer)
        {
          bIgnoreTimer = std::any_of(
              failedClients.cbegin(), failedClients.cend(),
              [&timer](const auto& failedClient) { return failedClient == timer->ClientID(); });
        }

        if (bIgnoreTimer)
        {
          ++it2;
          continue;
        }

        CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted timer {} on client {}", timer->ClientIndex(),
                    timer->ClientID());

        CheckAndAppendTimerNotification(timerNotifications, timer, true);

        it2 = it->second.erase(it2);

        bChanged = true;
        bAddedOrDeleted = true;
      }
      else if ((timer->IsStartAnyTime() && it->first != CDateTime()) ||
               (!timer->IsStartAnyTime() && timer->StartAsUTC() != it->first))
      {
        /* timer start has changed */
        CLog::LogFC(LOGDEBUG, LOGPVR, "Changed start time timer {} on client {}",
                    timer->ClientIndex(), timer->ClientID());

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
  for (const auto& timer : timersToMove)
  {
    InsertEntry(timer);
  }

  /* update child information for all parent timers */
  for (const auto& tagsEntry : m_tags)
  {
    for (const auto& timersEntry : tagsEntry.second)
    {
      if (timersEntry->IsTimerRule())
        timersEntry->ResetChildState();
    }

    for (const auto& timersEntry : tagsEntry.second)
    {
      const std::shared_ptr<CPVRTimerInfoTag> parentTimer = GetTimerRule(timersEntry);
      if (parentTimer)
        parentTimer->UpdateChildState(timersEntry, true);
    }
  }

  m_failedClients = failedClients;
  m_bFirstUpdate = false;
  m_bIsUpdating = false;

  if (bChanged)
  {
    UpdateChannels();
    lock.unlock();

    NotifyTimersEvent(bAddedOrDeleted);

    if (!timerNotifications.empty())
    {
      CPVREventLogJob* job = new CPVREventLogJob;

      /* queue notifications / fill eventlog */
      for (const auto& entry : timerNotifications)
      {
        const std::shared_ptr<const CPVRClient> client =
            CServiceBroker::GetPVRManager().GetClient(entry.first);
        if (client)
        {
          job->AddEvent(m_settings.GetBoolValue(CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS),
                        EventLevel::Information, // info, no error
                        client->GetFullClientName(), entry.second, client->Icon());
        }
      }

      CServiceBroker::GetJobManager()->AddJob(job, nullptr);
    }
  }

  return true;
}

namespace
{
std::vector<std::shared_ptr<CPVREpgInfoTag>> GetEpgTagsForTimerRule(
    const CPVRTimerRuleMatcher& matcher)
{
  std::vector<std::shared_ptr<CPVREpgInfoTag>> matches;

  const std::shared_ptr<const CPVRChannel> channel = matcher.GetChannel();
  if (channel)
  {
    // match single channel
    const std::shared_ptr<const CPVREpg> epg = channel->GetEPG();
    if (epg)
    {
      const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags = epg->GetTags();
      std::copy_if(tags.cbegin(), tags.cend(), std::back_inserter(matches),
                   [&matcher](const auto& tag) { return matcher.Matches(tag); });
    }
  }
  else
  {
    // match any channel
    const std::vector<std::shared_ptr<CPVREpg>> epgs =
        CServiceBroker::GetPVRManager().EpgContainer().GetAllEpgs();

    for (const auto& epg : epgs)
    {
      const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags = epg->GetTags();
      std::copy_if(tags.cbegin(), tags.cend(), std::back_inserter(matches),
                   [&matcher](const auto& tag) { return matcher.Matches(tag); });
    }
  }

  return matches;
}

void AddTimerRuleToEpgMap(
    const std::shared_ptr<CPVRTimerInfoTag>& timer,
    const CDateTime& now,
    std::map<std::shared_ptr<CPVREpg>, std::vector<std::shared_ptr<CPVRTimerRuleMatcher>>>& epgMap,
    bool& bFetchedAllEpgs)
{
  const std::shared_ptr<const CPVRChannel> channel = timer->Channel();
  if (channel)
  {
    const std::shared_ptr<CPVREpg> epg = channel->GetEPG();
    if (epg)
    {
      const std::shared_ptr<CPVRTimerRuleMatcher> matcher =
          std::make_shared<CPVRTimerRuleMatcher>(timer, now);
      auto it = epgMap.find(epg);
      if (it == epgMap.end())
        epgMap.insert({epg, {matcher}});
      else
        it->second.emplace_back(matcher);
    }
  }
  else
  {
    // rule matches "any channel" => we need to check all channels
    if (!bFetchedAllEpgs)
    {
      const std::vector<std::shared_ptr<CPVREpg>> epgs =
          CServiceBroker::GetPVRManager().EpgContainer().GetAllEpgs();
      for (const auto& epg : epgs)
      {
        const std::shared_ptr<CPVRTimerRuleMatcher> matcher =
            std::make_shared<CPVRTimerRuleMatcher>(timer, now);
        auto it = epgMap.find(epg);
        if (it == epgMap.end())
          epgMap.insert({epg, {matcher}});
        else
          it->second.emplace_back(matcher);
      }
      bFetchedAllEpgs = true;
    }
    else
    {
      for (auto& epgMapEntry : epgMap)
      {
        const std::shared_ptr<CPVRTimerRuleMatcher> matcher =
            std::make_shared<CPVRTimerRuleMatcher>(timer, now);
        epgMapEntry.second.emplace_back(matcher);
      }
    }
  }
}
} // unnamed namespace

bool CPVRTimers::UpdateEntries(int iMaxNotificationDelay)
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> timersToReinsert;
  std::vector<std::pair<std::shared_ptr<CPVRTimerInfoTag>, std::shared_ptr<CPVRTimerInfoTag>>>
      childTimersToInsert;
  bool bChanged = false;
  const CDateTime now = CDateTime::GetUTCDateTime();
  bool bFetchedAllEpgs = false;
  std::map<std::shared_ptr<CPVREpg>, std::vector<std::shared_ptr<CPVRTimerRuleMatcher>>> epgMap;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (auto it = m_tags.begin(); it != m_tags.end();)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end();)
    {
      std::shared_ptr<CPVRTimerInfoTag> timer = *it2;
      bool bDeleteTimer = false;
      if (!timer->IsOwnedByClient())
      {
        if (timer->IsEpgBased() && timer->Channel())
        {
          // update epg tag
          const std::shared_ptr<const CPVREpg> epg =
              CServiceBroker::GetPVRManager().EpgContainer().GetByChannelUid(
                  timer->Channel()->ClientID(), timer->Channel()->UniqueID());
          if (epg)
          {
            const std::shared_ptr<CPVREpgInfoTag> epgTag =
                epg->GetTagByBroadcastId(timer->UniqueBroadcastID());
            if (epgTag)
            {
              timer->SetEpgInfoTag(epgTag);

              bool bStartChanged =
                  !timer->IsStartAnyTime() && epgTag->StartAsUTC() != timer->StartAsUTC();
              bool bEndChanged = !timer->IsEndAnyTime() && epgTag->EndAsUTC() != timer->EndAsUTC();
              if (bStartChanged || bEndChanged)
              {
                if (bStartChanged)
                  timer->SetStartFromUTC(epgTag->StartAsUTC());
                if (bEndChanged)
                  timer->SetEndFromUTC(epgTag->EndAsUTC());

                timer->UpdateSummary();
                bChanged = true;

                if (bStartChanged)
                {
                  // start time changed. timer must be reinserted in timer map
                  bDeleteTimer = true;
                  timersToReinsert.emplace_back(timer); // remember and reinsert/save later
                }
                else
                {
                  // save changes to database
                  timer->Persist();
                }
              }

              // check for epg tag uids that were re-used for a different event (which is actually
              // an add-on/a backend bug)
              if (!timer->IsTimerRule() && (epgTag->Title() != timer->Title()))
              {
                const std::shared_ptr<CPVRTimerInfoTag> parent{GetTimerRule(timer)};
                if (parent)
                {
                  const CPVRTimerRuleMatcher matcher{parent, now};
                  if (!matcher.Matches(epgTag))
                  {
                    // epg event no longer matches the rule. delete the timer
                    bDeleteTimer = true;
                    timer->DeleteFromDatabase();
                  }
                }
              }
            }
            else if (!timer->IsTimerRule())
            {
              // epg event no longer present. delete the timer
              bDeleteTimer = true;
              timer->DeleteFromDatabase();
            }
          }
        }

        // check for due timers and announce/delete them
        int iMarginStart = timer->GetTimerType()->SupportsStartMargin() ? timer->MarginStart() : 0;
        if (!timer->IsTimerRule() &&
            (timer->StartAsUTC() - CDateTimeSpan(0, 0, iMarginStart, iMaxNotificationDelay)) < now)
        {
          if (timer->IsReminder() && !timer->IsDisabled())
          {
            // reminder is due / over due. announce it.
            m_remindersToAnnounce.push(timer);
          }

          if (timer->EndAsUTC() >= now)
          {
            // disable timer until timer's end time is due
            if (!timer->IsDisabled())
            {
              timer->SetState(PVR_TIMER_STATE_DISABLED);
              bChanged = true;
            }
          }
          else
          {
            // end time due. delete completed timer
            bChanged = true;
            bDeleteTimer = true;
            timer->DeleteFromDatabase();
          }
        }

        if (timer->IsTimerRule() && timer->IsReminder() && timer->IsActive())
        {
          if (timer->IsEpgBased())
          {
            if (m_bReminderRulesUpdatePending)
              AddTimerRuleToEpgMap(timer, now, epgMap, bFetchedAllEpgs);
          }
          else
          {
            // create new children of time-based reminder timer rules
            const CPVRTimerRuleMatcher matcher(timer, now);
            const CDateTime nextStart = matcher.GetNextTimerStart();
            if (nextStart.IsValid())
            {
              bool bCreate = false;
              const auto it1 = m_tags.find(nextStart);
              if (it1 == m_tags.end())
                bCreate = true;
              else
                bCreate =
                    std::none_of(it1->second.cbegin(), it1->second.cend(),
                                 [&timer](const std::shared_ptr<const CPVRTimerInfoTag>& tmr) {
                                   return tmr->ParentClientIndex() == timer->ClientIndex();
                                 });
              if (bCreate)
              {
                const CDateTimeSpan duration = timer->EndAsUTC() - timer->StartAsUTC();
                const std::shared_ptr<CPVRTimerInfoTag> childTimer =
                    CPVRTimerInfoTag::CreateReminderFromDate(
                        nextStart, duration.GetSecondsTotal() / 60, timer);
                if (childTimer)
                {
                  bChanged = true;
                  childTimersToInsert.emplace_back(timer,
                                                   childTimer); // remember and insert/save later
                }
              }
            }
          }
        }
      }

      if (bDeleteTimer)
      {
        const std::shared_ptr<CPVRTimerInfoTag> parent = GetTimerRule(timer);
        if (parent)
          parent->UpdateChildState(timer, false);

        it2 = it->second.erase(it2);
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

  // reinsert timers with changed timer start
  for (const auto& timer : timersToReinsert)
  {
    InsertEntry(timer);
    timer->Persist();
  }

  // create new children of local epg-based reminder timer rules
  for (const auto& epgMapEntry : epgMap)
  {
    const auto epgTags = epgMapEntry.first->GetTags();
    for (const auto& epgTag : epgTags)
    {
      if (GetTimerForEpgTag(epgTag))
        continue;

      for (const auto& matcher : epgMapEntry.second)
      {
        if (!matcher->Matches(epgTag))
          continue;

        const std::shared_ptr<CPVRTimerInfoTag> childTimer =
            CPVRTimerInfoTag::CreateReminderFromEpg(epgTag, matcher->GetTimerRule());
        if (childTimer)
        {
          bChanged = true;
          childTimersToInsert.emplace_back(matcher->GetTimerRule(),
                                           childTimer); // remember and insert/save later
        }
      }
    }
  }

  // persist and insert/update new children of local time-based and epg-based reminder timer rules
  for (const auto& timerPair : childTimersToInsert)
  {
    PersistAndUpdateLocalTimer(timerPair.second, timerPair.first);
  }

  m_bReminderRulesUpdatePending = false;

  // announce changes
  if (bChanged)
    NotifyTimersEvent();

  if (!m_remindersToAnnounce.empty())
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::AnnounceReminder);

  return bChanged;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextReminderToAnnnounce()
{
  std::shared_ptr<CPVRTimerInfoTag> ret;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!m_remindersToAnnounce.empty())
  {
    ret = m_remindersToAnnounce.front();
    m_remindersToAnnounce.pop();
  }
  return ret;
}

bool CPVRTimers::KindMatchesTag(const TimerKind& eKind,
                                const std::shared_ptr<const CPVRTimerInfoTag>& tag) const
{
  return (eKind == TimerKindAny) || (eKind == TimerKindTV && !tag->IsRadio()) ||
         (eKind == TimerKindRadio && tag->IsRadio());
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTimer(const TimerKind& eKind,
                                                                 bool bIgnoreReminders) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    for (const auto& timersEntry : tagsEntry.second)
    {
      if (bIgnoreReminders && timersEntry->IsReminder())
        continue;

      if (KindMatchesTag(eKind, timersEntry) && timersEntry->IsActive() &&
          !timersEntry->IsRecording() && !timersEntry->IsTimerRule() && !timersEntry->IsBroken())
        return timersEntry;
    }
  }

  return {};
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTimer(
    bool bIgnoreReminders /* = true */) const
{
  return GetNextActiveTimer(TimerKindAny, bIgnoreReminders);
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveTVTimer() const
{
  return GetNextActiveTimer(TimerKindTV, true);
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetNextActiveRadioTimer() const
{
  return GetNextActiveTimer(TimerKindRadio, true);
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveTimers() const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags;
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    std::copy_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(), std::back_inserter(tags),
                 [](const auto& timersEntry) {
                   return timersEntry->IsActive() && !timersEntry->IsBroken() &&
                          !timersEntry->IsReminder() && !timersEntry->IsTimerRule();
                 });
  }

  return tags;
}

int CPVRTimers::AmountActiveTimers(const TimerKind& eKind) const
{
  int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    iReturn += std::count_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                             [this, &eKind](const auto& timersEntry) {
                               return KindMatchesTag(eKind, timersEntry) &&
                                      timersEntry->IsActive() && !timersEntry->IsBroken() &&
                                      !timersEntry->IsReminder() && !timersEntry->IsTimerRule();
                             });
  }

  return iReturn;
}

int CPVRTimers::AmountActiveTimers() const
{
  return AmountActiveTimers(TimerKindAny);
}

int CPVRTimers::AmountActiveTVTimers() const
{
  return AmountActiveTimers(TimerKindTV);
}

int CPVRTimers::AmountActiveRadioTimers() const
{
  return AmountActiveTimers(TimerKindRadio);
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetActiveRecordings(
    const TimerKind& eKind) const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> tags;
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    std::copy_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(), std::back_inserter(tags),
                 [this, &eKind](const auto& timersEntry) {
                   return KindMatchesTag(eKind, timersEntry) && timersEntry->IsRecording() &&
                          !timersEntry->IsTimerRule() && !timersEntry->IsBroken() &&
                          !timersEntry->IsReminder();
                 });
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

int CPVRTimers::AmountActiveRecordings(const TimerKind& eKind) const
{
  int iReturn = 0;
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    iReturn += std::count_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                             [this, &eKind](const auto& timersEntry) {
                               return KindMatchesTag(eKind, timersEntry) &&
                                      timersEntry->IsRecording() && !timersEntry->IsTimerRule() &&
                                      !timersEntry->IsBroken() && !timersEntry->IsReminder();
                             });
  }

  return iReturn;
}

int CPVRTimers::AmountActiveRecordings() const
{
  return AmountActiveRecordings(TimerKindAny);
}

int CPVRTimers::AmountActiveTVRecordings() const
{
  return AmountActiveRecordings(TimerKindTV);
}

int CPVRTimers::AmountActiveRadioRecordings() const
{
  return AmountActiveRecordings(TimerKindRadio);
}

/********** channel methods **********/

bool CPVRTimers::DeleteTimersOnChannel(const std::shared_ptr<CPVRChannel>& channel,
                                       bool bDeleteTimerRules /* = true */,
                                       bool bCurrentlyActiveOnly /* = false */)
{
  bool bReturn = false;
  bool bChanged = false;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    for (MapTags::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); ++it)
    {
      for (const auto& timersEntry : (*it).second)
      {
        bool bDeleteActiveItem = !bCurrentlyActiveOnly || timersEntry->IsRecording();
        bool bDeleteTimerRuleItem = bDeleteTimerRules || !timersEntry->IsTimerRule();
        bool bChannelsMatch = timersEntry->HasChannel() && timersEntry->Channel() == channel;

        if (bDeleteActiveItem && bDeleteTimerRuleItem && bChannelsMatch)
        {
          CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted timer {} on client {}", timersEntry->ClientIndex(),
                      timersEntry->ClientID());
          bReturn = (timersEntry->DeleteFromClient(true) == TimerOperationResult::OK) || bReturn;
          bChanged = true;
        }
      }
    }
  }

  if (bChanged)
    NotifyTimersEvent();

  return bReturn;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::UpdateEntry(
    const std::shared_ptr<const CPVRTimerInfoTag>& timer)
{
  bool bChanged = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::shared_ptr<CPVRTimerInfoTag> tag = GetByClient(timer->ClientID(), timer->ClientIndex());
  if (tag)
  {
    bool bReinsert = tag->StartAsUTC() != timer->StartAsUTC();
    if (bReinsert)
    {
      RemoveEntry(tag);
    }

    bChanged = tag->UpdateEntry(timer);

    if (bReinsert)
    {
      InsertEntry(tag);
    }
  }
  else
  {
    tag = std::make_shared<CPVRTimerInfoTag>();
    if (tag->UpdateEntry(timer))
    {
      tag->SetTimerID(++m_iLastId);
      InsertEntry(tag);
      bChanged = true;
    }
  }

  return bChanged ? tag : std::shared_ptr<CPVRTimerInfoTag>();
}

bool CPVRTimers::AddTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag)
{
  bool bReturn = false;
  if (tag->IsOwnedByClient())
  {
    bReturn = tag->AddToClient();
  }
  else
  {
    bReturn = AddLocalTimer(tag, true);
  }
  return bReturn;
}

TimerOperationResult CPVRTimers::DeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag,
                                             bool bForce /* = false */,
                                             bool bDeleteRule /* = false */)
{
  TimerOperationResult ret = TimerOperationResult::FAILED;
  if (!tag)
    return ret;

  std::shared_ptr<CPVRTimerInfoTag> tagToDelete = tag;

  if (bDeleteRule)
  {
    /* delete the timer rule that scheduled this timer. */
    const std::shared_ptr<CPVRTimerInfoTag> ruleTag = GetTimerRule(tagToDelete);
    if (!ruleTag)
    {
      CLog::LogF(LOGERROR, "Unable to obtain timer rule for given timer");
      return ret;
    }

    tagToDelete = ruleTag;
  }

  if (tagToDelete->IsOwnedByClient())
  {
    ret = tagToDelete->DeleteFromClient(bForce);
  }
  else
  {
    if (DeleteLocalTimer(tagToDelete, true))
      ret = TimerOperationResult::OK;
  }

  return ret;
}

bool CPVRTimers::UpdateTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag)
{
  bool bReturn = false;
  if (tag->IsOwnedByClient())
  {
    bReturn = tag->UpdateOnClient();
  }
  else
  {
    bReturn = UpdateLocalTimer(tag);
  }
  return bReturn;
}

bool CPVRTimers::AddLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag, bool bNotify)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const std::shared_ptr<CPVRTimerInfoTag> persistedTimer = PersistAndUpdateLocalTimer(tag, nullptr);
  bool bReturn = !!persistedTimer;

  if (bReturn && persistedTimer->IsTimerRule() && persistedTimer->IsActive())
  {
    if (persistedTimer->IsEpgBased())
    {
      // create and persist children of local epg-based timer rule
      const std::vector<std::shared_ptr<CPVREpgInfoTag>> epgTags =
          GetEpgTagsForTimerRule(CPVRTimerRuleMatcher(persistedTimer, CDateTime::GetUTCDateTime()));
      for (const auto& epgTag : epgTags)
      {
        const std::shared_ptr<CPVRTimerInfoTag> childTimer =
            CPVRTimerInfoTag::CreateReminderFromEpg(epgTag, persistedTimer);
        if (childTimer)
        {
          PersistAndUpdateLocalTimer(childTimer, persistedTimer);
        }
      }
    }
    else
    {
      // create and persist children of local time-based timer rule
      const CDateTime nextStart =
          CPVRTimerRuleMatcher(persistedTimer, CDateTime::GetUTCDateTime()).GetNextTimerStart();
      if (nextStart.IsValid())
      {
        const CDateTimeSpan duration = persistedTimer->EndAsUTC() - persistedTimer->StartAsUTC();
        const std::shared_ptr<CPVRTimerInfoTag> childTimer =
            CPVRTimerInfoTag::CreateReminderFromDate(nextStart, duration.GetSecondsTotal() / 60,
                                                     persistedTimer);
        if (childTimer)
        {
          PersistAndUpdateLocalTimer(childTimer, persistedTimer);
        }
      }
    }
  }

  if (bNotify && bReturn)
  {
    lock.unlock();
    NotifyTimersEvent();
  }

  return bReturn;
}

bool CPVRTimers::DeleteLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag, bool bNotify)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  RemoveEntry(tag);

  bool bReturn = tag->DeleteFromDatabase();

  if (bReturn && tag->IsTimerRule())
  {
    // delete children of local timer rule
    for (auto it = m_tags.begin(); it != m_tags.end();)
    {
      for (auto it2 = it->second.begin(); it2 != it->second.end();)
      {
        std::shared_ptr<CPVRTimerInfoTag> timer = *it2;
        if (timer->ParentClientIndex() == tag->ClientIndex())
        {
          tag->UpdateChildState(timer, false);
          it2 = it->second.erase(it2);
          timer->DeleteFromDatabase();
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
  }

  if (bNotify && bReturn)
  {
    lock.unlock();
    NotifyTimersEvent();
  }

  return bReturn;
}

bool CPVRTimers::UpdateLocalTimer(const std::shared_ptr<CPVRTimerInfoTag>& tag)
{
  // delete and re-create timer and children, if any.
  bool bReturn = DeleteLocalTimer(tag, false);

  if (bReturn)
    bReturn = AddLocalTimer(tag, false);

  if (bReturn)
    NotifyTimersEvent();

  return bReturn;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::PersistAndUpdateLocalTimer(
    const std::shared_ptr<CPVRTimerInfoTag>& timer,
    const std::shared_ptr<CPVRTimerInfoTag>& parentTimer)
{
  std::shared_ptr<CPVRTimerInfoTag> tag;
  bool bReturn = timer->Persist();
  if (bReturn)
  {
    tag = UpdateEntry(timer);
    if (tag && parentTimer)
      parentTimer->UpdateChildState(timer, true);
  }
  return bReturn ? tag : std::shared_ptr<CPVRTimerInfoTag>();
}

bool CPVRTimers::IsRecordingOnChannel(const CPVRChannel& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& tagsEntry : m_tags)
  {
    if (std::any_of(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                    [&channel](const auto& timersEntry) {
                      return timersEntry->IsRecording() &&
                             timersEntry->ClientChannelUID() == channel.UniqueID() &&
                             timersEntry->ClientID() == channel.ClientID();
                    }))
      return true;
  }

  return false;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetActiveTimerForChannel(
    const std::shared_ptr<const CPVRChannel>& channel) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& tagsEntry : m_tags)
  {
    const auto it = std::find_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                                 [&channel](const auto& timersEntry) {
                                   return timersEntry->IsRecording() &&
                                          timersEntry->ClientChannelUID() == channel->UniqueID() &&
                                          timersEntry->ClientID() == channel->ClientID();
                                 });
    if (it != tagsEntry.second.cend())
      return (*it);
  }

  return {};
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetTimerForEpgTag(
    const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const
{
  if (epgTag)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    for (const auto& tagsEntry : m_tags)
    {
      for (const auto& timersEntry : tagsEntry.second)
      {
        if (timersEntry->IsTimerRule())
          continue;

        if (timersEntry->GetEpgInfoTag(false) == epgTag)
          return timersEntry;

        if (timersEntry->ClientChannelUID() != PVR_CHANNEL_INVALID_UID &&
            timersEntry->ClientChannelUID() == epgTag->UniqueChannelID() &&
            timersEntry->ClientID() == epgTag->ClientID())
        {
          if (timersEntry->UniqueBroadcastID() != EPG_TAG_INVALID_UID &&
              timersEntry->UniqueBroadcastID() == epgTag->UniqueBroadcastID())
            return timersEntry;

          if (timersEntry->IsRadio() == epgTag->IsRadio() &&
              timersEntry->StartAsUTC() <= epgTag->StartAsUTC() &&
              timersEntry->EndAsUTC() >= epgTag->EndAsUTC())
            return timersEntry;
        }
      }
    }
  }

  return {};
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetTimerRule(
    const std::shared_ptr<const CPVRTimerInfoTag>& timer) const
{
  if (timer)
  {
    const int iParentClientIndex = timer->ParentClientIndex();
    if (iParentClientIndex != PVR_TIMER_NO_PARENT)
    {
      int iClientId = timer->ClientID();

      std::unique_lock<CCriticalSection> lock(m_critSection);
      for (const auto& tagsEntry : m_tags)
      {
        const auto it = std::find_if(tagsEntry.second.cbegin(), tagsEntry.second.cend(),
                                     [iClientId, iParentClientIndex](const auto& timersEntry)
                                     {
                                       return (timersEntry->ClientID() == PVR_ANY_CLIENT_ID ||
                                               timersEntry->ClientID() == iClientId) &&
                                              timersEntry->ClientIndex() == iParentClientIndex;
                                     });
        if (it != tagsEntry.second.cend())
          return (*it);
      }
    }
  }

  return {};
}

void CPVRTimers::Notify(const PVREvent& event)
{
  switch (static_cast<PVREvent>(event))
  {
    case PVREvent::EpgContainer:
      CServiceBroker::GetPVRManager().TriggerTimersUpdate();
      break;
    case PVREvent::Epg:
    case PVREvent::EpgItemUpdate:
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      m_bReminderRulesUpdatePending = true;
      break;
    }
    default:
      break;
  }
}

CDateTime CPVRTimers::GetNextEventTime() const
{
  const bool dailywakup =
      m_settings.GetBoolValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP);
  const CDateTime now = CDateTime::GetUTCDateTime();
  const CDateTimeSpan prewakeup(
      0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP), 0);
  const CDateTimeSpan idle(
      0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);

  CDateTime wakeuptime;

  /* Check next active time */
  const std::shared_ptr<const CPVRTimerInfoTag> timer = GetNextActiveTimer(false);
  if (timer)
  {
    const CDateTimeSpan prestart(0, 0, timer->MarginStart(), 0);
    const CDateTime start = timer->StartAsUTC();
    wakeuptime =
        ((start - prestart - prewakeup - idle) > now) ? start - prestart - prewakeup : now + idle;
  }

  /* check daily wake up */
  if (dailywakup)
  {
    CDateTime dailywakeuptime;
    dailywakeuptime.SetFromDBTime(
        m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
    dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

    dailywakeuptime.SetDateTime(now.GetYear(), now.GetMonth(), now.GetDay(),
                                dailywakeuptime.GetHour(), dailywakeuptime.GetMinute(),
                                dailywakeuptime.GetSecond());

    if ((dailywakeuptime - idle) < now)
    {
      const CDateTimeSpan oneDay(1, 0, 0, 0);
      dailywakeuptime += oneDay;
    }
    if (!wakeuptime.IsValid() || dailywakeuptime < wakeuptime)
      wakeuptime = dailywakeuptime;
  }

  const CDateTime retVal(wakeuptime);
  return retVal;
}

void CPVRTimers::UpdateChannels()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& tagsEntry : m_tags)
  {
    for (const auto& timersEntry : tagsEntry.second)
      timersEntry->UpdateChannel();
  }
}

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRTimers::GetAll() const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> timers;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& tagsEntry : m_tags)
  {
    std::copy(tagsEntry.second.cbegin(), tagsEntry.second.cend(), std::back_inserter(timers));
  }

  return timers;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRTimers::GetById(unsigned int iTimerId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& tagsEntry : m_tags)
  {
    const auto it = std::find_if(
        tagsEntry.second.cbegin(), tagsEntry.second.cend(),
        [iTimerId](const auto& timersEntry) { return timersEntry->TimerID() == iTimerId; });
    if (it != tagsEntry.second.cend())
      return (*it);
  }

  return {};
}

void CPVRTimers::NotifyTimersEvent(bool bAddedOrDeleted /* = true */)
{
  CServiceBroker::GetPVRManager().PublishEvent(bAddedOrDeleted ? PVREvent::TimersInvalidated
                                                               : PVREvent::Timers);
}
