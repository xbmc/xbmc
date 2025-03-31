/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgContainer.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h" // PVR_CHANNEL_INVALID_UID
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace PVR
{

class CEpgUpdateRequest
{
public:
  CEpgUpdateRequest() : CEpgUpdateRequest(PVR_CLIENT_INVALID_UID, PVR_CHANNEL_INVALID_UID) {}
  CEpgUpdateRequest(int iClientID, int iUniqueChannelID) : m_iClientID(iClientID), m_iUniqueChannelID(iUniqueChannelID) {}

  void Deliver(const std::shared_ptr<CPVREpg>& epg);

  int GetClientID() const { return m_iClientID; }
  int GetUniqueChannelID() const { return m_iUniqueChannelID; }

private:
  int m_iClientID;
  int m_iUniqueChannelID;
};

void CEpgUpdateRequest::Deliver(const std::shared_ptr<CPVREpg>& epg)
{
  epg->ForceUpdate();
}

class CEpgTagStateChange
{
public:
  CEpgTagStateChange() = default;
  CEpgTagStateChange(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE eNewState) : m_epgtag(tag), m_state(eNewState) {}

  void Deliver(const std::shared_ptr<CPVREpg>& epg);

  std::shared_ptr<CPVREpgInfoTag> GetTag() const { return m_epgtag; }

private:
  std::shared_ptr<CPVREpgInfoTag> m_epgtag;
  EPG_EVENT_STATE m_state = EPG_EVENT_CREATED;
};

void CEpgTagStateChange::Deliver(const std::shared_ptr<CPVREpg>& epg)
{
  if (m_epgtag->EpgID() < 0)
  {
    // now that we have the epg instance, fully initialize the tag
    m_epgtag->SetEpgID(epg->EpgID());
    m_epgtag->SetChannelData(epg->GetChannelData());
  }

  epg->UpdateEntry(m_epgtag, m_state);
}

CPVREpgContainer::CPVREpgContainer(CEventSource<PVREvent>& eventSource)
  : CThread("EPGUpdater"),
    m_database(new CPVREpgDatabase),
    m_settings({CSettings::SETTING_EPG_EPGUPDATE, CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY,
                CSettings::SETTING_EPG_PAST_DAYSTODISPLAY,
                CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV}),
    m_events(eventSource)
{
  m_bStop = true; // base class member
  m_updateEvent.Reset();
}

CPVREpgContainer::~CPVREpgContainer()
{
  Stop();
  Unload();
}

std::shared_ptr<CPVREpgDatabase> CPVREpgContainer::GetEpgDatabase() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!m_database->IsOpen())
    m_database->Open();

  return m_database;
}

bool CPVREpgContainer::IsStarted() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bStarted;
}

int CPVREpgContainer::NextEpgId()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return ++m_iNextEpgId;
}

void CPVREpgContainer::Start()
{
  Stop();

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_bIsInitialising = true;

    Create();
    SetPriority(ThreadPriority::BELOW_NORMAL);

    m_bStarted = true;
  }
}

void CPVREpgContainer::Stop()
{
  StopThread();

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_bStarted = false;
  }
}

bool CPVREpgContainer::Load()
{
  return true;
}

void CPVREpgContainer::Unload()
{
  {
    std::unique_lock<CCriticalSection> lock(m_updateRequestsLock);
    m_updateRequests.clear();
  }

  {
    std::unique_lock<CCriticalSection> lock(m_epgTagChangesLock);
    m_epgTagChanges.clear();
  }

  std::vector<std::shared_ptr<CPVREpg>> epgs;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    /* clear all epg tables and remove pointers to epg tables on channels */
    std::transform(m_epgIdToEpgMap.cbegin(), m_epgIdToEpgMap.cend(), std::back_inserter(epgs),
                   [](const auto& epgEntry) { return epgEntry.second; });

    m_epgIdToEpgMap.clear();
    m_channelUidToEpgMap.clear();

    m_iNextEpgUpdate = 0;
    m_iNextEpgId = 0;
    m_iNextEpgActiveTagCheck = 0;
    m_bUpdateNotificationPending = false;
    m_bLoaded = false;

    m_database->Close();
  }

  for (const auto& epg : epgs)
  {
    epg->Events().Unsubscribe(this);
    epg->RemovedFromContainer();
  }
}

void CPVREpgContainer::Notify(const PVREvent& event)
{
  if (event == PVREvent::EpgItemUpdate)
  {
    // there can be many of these notifications during short time period. Thus, announce async and not every event.
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_bUpdateNotificationPending = true;
    return;
  }
  else if (event == PVREvent::EpgUpdatePending)
  {
    SetHasPendingUpdates(true);
    return;
  }
  else if (event == PVREvent::EpgActiveItem)
  {
    // No need to propagate the change. See CPVREpgContainer::CheckPlayingEvents
    return;
  }

  m_events.Publish(event);
}

void CPVREpgContainer::LoadFromDatabase()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bLoaded)
    return;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  database->Lock();
  m_iNextEpgId = database->GetLastEPGId();
  const std::vector<std::shared_ptr<CPVREpg>> result = database->GetAll();
  database->Unlock();

  for (const auto& entry : result)
    InsertFromDB(entry);

  m_bLoaded = true;
}

bool CPVREpgContainer::PersistAll(unsigned int iMaxTimeslice) const
{
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  std::vector<std::shared_ptr<CPVREpg>> changedEpgs;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& epg : m_epgIdToEpgMap)
    {
      if (epg.second && epg.second->NeedsSave())
      {
        // Note: We need to obtain a lock for every epg instance before we can lock
        //       the epg db. This order is important. Otherwise deadlocks may occur.
        epg.second->Lock();
        changedEpgs.emplace_back(epg.second);
      }
    }
  }

  bool bReturn = true;

  if (!changedEpgs.empty())
  {
    // Note: We must lock the db the whole time, otherwise races may occur.
    database->Lock();

    XbmcThreads::EndTime<> processTimeslice{std::chrono::milliseconds(iMaxTimeslice)};
    for (const auto& epg : changedEpgs)
    {
      if (!processTimeslice.IsTimePast())
      {
        CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: Persisting events for channel '{}'...",
                    epg->GetChannelData()->ChannelName());

        bReturn &= epg->QueuePersistQuery(database);

        size_t queryCount = database->GetInsertQueriesCount() + database->GetDeleteQueriesCount();
        if (queryCount > EPG_COMMIT_QUERY_COUNT_LIMIT)
        {
          CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: committing {} queries in loop.",
                      queryCount);
          database->CommitDeleteQueries();
          database->CommitInsertQueries();
          CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: committed {} queries in loop.", queryCount);
        }
      }

      epg->Unlock();
    }

    if (bReturn)
    {
      database->CommitDeleteQueries();
      database->CommitInsertQueries();
    }

    database->Unlock();
  }

  return bReturn;
}

void CPVREpgContainer::Process()
{
  time_t iNow = 0;
  time_t iLastSave = 0;

  SetPriority(ThreadPriority::LOWEST);

  while (!m_bStop)
  {
    time_t iLastEpgCleanup = 0;
    bool bUpdateEpg = true;

    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      bUpdateEpg = (iNow >= m_iNextEpgUpdate) && IsAwake();
      iLastEpgCleanup = m_iLastEpgCleanup;
    }

    /* update the EPG */
    if (!InterruptUpdate() && bUpdateEpg && UpdateEPG())
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && IsAwake() &&
        iNow >= iLastEpgCleanup + CServiceBroker::GetSettingsComponent()
                                      ->GetAdvancedSettings()
                                      ->m_iEpgCleanupInterval)
      RemoveOldEntries();

    /* check for pending manual EPG updates */

    while (!m_bStop && IsAwake())
    {
      CEpgUpdateRequest request;
      std::shared_ptr<CPVREpg> epg;
      {
        std::unique_lock<CCriticalSection> lock(m_updateRequestsLock);
        if (m_updateRequests.empty())
          break;

        request = m_updateRequests.front();
        m_updateRequests.pop_front();

        epg = GetByChannelUid(request.GetClientID(), request.GetUniqueChannelID());
        if (!epg)
        {
          // try later; channel might not have been loaded yet
          m_updateRequests.emplace_back(request);
          break;
        }
      }

      // do the update
      request.Deliver(epg);
    }

    /* check for pending EPG tag changes */

    if (!m_bStop && IsAwake())
    {
      unsigned int iProcessed = 0;
      XbmcThreads::EndTime<> processTimeslice(
          1000ms); // max 1 sec per cycle, regardless of how many events are in the queue

      while (!InterruptUpdate())
      {
        CEpgTagStateChange change;
        std::shared_ptr<CPVREpg> epg;
        {
          std::unique_lock<CCriticalSection> lock(m_epgTagChangesLock);
          if (processTimeslice.IsTimePast() || m_epgTagChanges.empty())
          {
            if (iProcessed > 0)
              CLog::LogFC(LOGDEBUG, LOGEPG, "Processed {} queued epg event changes.", iProcessed);

            break;
          }

          change = m_epgTagChanges.front();
          m_epgTagChanges.pop_front();

          epg = GetByChannelUid(change.GetTag()->ClientID(), change.GetTag()->UniqueChannelID());
          if (!epg)
          {
            // try later; channel might not have been loaded yet
            m_epgTagChanges.emplace_back(change);
            continue;
          }
        }

        iProcessed++;

        // deliver the updated tag to the respective epg
        change.Deliver(epg);
      }
    }

    if (!m_bStop && IsAwake())
    {
      bool bHasPendingUpdates = false;

      {
        std::unique_lock<CCriticalSection> lock(m_critSection);
        bHasPendingUpdates = (m_pendingUpdates > 0);
      }

      if (bHasPendingUpdates)
        UpdateEPG(true);
    }

    /* check for updated active tag */
    if (!m_bStop)
      CheckPlayingEvents();

    /* check for pending update notifications */
    if (!m_bStop)
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      if (m_bUpdateNotificationPending)
      {
        m_bUpdateNotificationPending = false;
        m_events.Publish(PVREvent::Epg);
      }
    }

    /* check for changes that need to be saved every 60 seconds */
    if ((iNow - iLastSave > 60) && !InterruptUpdate())
    {
      PersistAll(1000);
      iLastSave = iNow;
    }

    CThread::Sleep(1000ms);
  }

  // store data on exit
  CLog::Log(LOGINFO, "EPG Container: Persisting unsaved events...");
  PersistAll(std::numeric_limits<unsigned int>::max());
  CLog::Log(LOGINFO, "EPG Container: Persisting events done");
}

std::vector<std::shared_ptr<CPVREpg>> CPVREpgContainer::GetAllEpgs() const
{
  std::vector<std::shared_ptr<CPVREpg>> epgs;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::transform(m_epgIdToEpgMap.cbegin(), m_epgIdToEpgMap.cend(), std::back_inserter(epgs),
                 [](const auto& epgEntry) { return epgEntry.second; });

  return epgs;
}

std::shared_ptr<CPVREpg> CPVREpgContainer::GetById(int iEpgId) const
{
  std::shared_ptr<CPVREpg> retval;

  if (iEpgId < 0)
    return retval;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto& epgEntry = m_epgIdToEpgMap.find(iEpgId);
  if (epgEntry != m_epgIdToEpgMap.end())
    retval = epgEntry->second;

  return retval;
}

std::shared_ptr<CPVREpg> CPVREpgContainer::GetByChannelUid(int iClientId, int iChannelUid) const
{
  std::shared_ptr<CPVREpg> epg;

  if (iClientId < 0 || iChannelUid < 0)
    return epg;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto& epgEntry = m_channelUidToEpgMap.find(std::pair<int, int>(iClientId, iChannelUid));
  if (epgEntry != m_channelUidToEpgMap.end())
    epg = epgEntry->second;

  return epg;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgContainer::GetTagById(
    const std::shared_ptr<const CPVREpg>& epg, unsigned int iBroadcastId) const
{
  std::shared_ptr<CPVREpgInfoTag> retval;

  if (iBroadcastId == EPG_TAG_INVALID_UID)
    return retval;

  if (epg)
  {
    retval = epg->GetTagByBroadcastId(iBroadcastId);
  }

  return retval;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgContainer::GetTagByDatabaseId(int iDatabaseId) const
{
  std::shared_ptr<CPVREpgInfoTag> retval;

  if (iDatabaseId <= 0)
    return retval;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
  {
    retval = epgEntry.second->GetTagByDatabaseId(iDatabaseId);
    if (retval)
      break;
  }

  return retval;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgContainer::GetTags(
    const PVREpgSearchData& searchData) const
{
  // make sure we have up-to-date data in the database.
  PersistAll(std::numeric_limits<unsigned int>::max());

  const std::shared_ptr<const CPVREpgDatabase> database = GetEpgDatabase();
  std::vector<std::shared_ptr<CPVREpgInfoTag>> results = database->GetEpgTags(searchData);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& tag : results)
  {
    const auto& it = m_epgIdToEpgMap.find(tag->EpgID());
    if (it != m_epgIdToEpgMap.cend())
      tag->SetChannelData((*it).second->GetChannelData());
  }

  return results;
}

void CPVREpgContainer::InsertFromDB(const std::shared_ptr<CPVREpg>& newEpg)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // table might already have been created when pvr channels were loaded
  std::shared_ptr<CPVREpg> epg = GetById(newEpg->EpgID());
  if (!epg)
  {
    // create a new epg table
    epg = newEpg;
    m_epgIdToEpgMap.insert({epg->EpgID(), epg});
    epg->Events().Subscribe(this, &CPVREpgContainer::Notify);
  }
}

std::shared_ptr<CPVREpg> CPVREpgContainer::CreateChannelEpg(int iEpgId, const std::string& strScraperName, const std::shared_ptr<CPVREpgChannelData>& channelData)
{
  std::shared_ptr<CPVREpg> epg;

  WaitForUpdateFinish();
  LoadFromDatabase();

  if (iEpgId > 0)
    epg = GetById(iEpgId);

  if (!epg)
  {
    if (iEpgId <= 0)
      iEpgId = NextEpgId();

    epg = std::make_shared<CPVREpg>(iEpgId, channelData->ChannelName(), strScraperName, channelData,
                                    GetEpgDatabase());

    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_epgIdToEpgMap.insert({iEpgId, epg});
    m_channelUidToEpgMap.insert({{channelData->ClientId(), channelData->UniqueClientChannelId()}, epg});
    epg->Events().Subscribe(this, &CPVREpgContainer::Notify);
  }
  else if (epg->ChannelID() == -1)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_channelUidToEpgMap.insert({{channelData->ClientId(), channelData->UniqueClientChannelId()}, epg});
    epg->SetChannelData(channelData);
  }

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_bPreventUpdates = false;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
  }

  m_events.Publish(PVREvent::EpgContainer);

  return epg;
}

bool CPVREpgContainer::RemoveOldEntries()
{
  const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(GetPastDaysToDisplay(), 0, 0, 0));

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
    epgEntry.second->Cleanup(cleanupTime);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iLastEpgCleanup);

  return true;
}

bool CPVREpgContainer::QueueDeleteEpgs(const std::vector<std::shared_ptr<CPVREpg>>& epgs)
{
  if (epgs.empty())
    return true;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  for (const auto& epg : epgs)
  {
    // Note: We need to obtain a lock for every epg instance before we can lock
    //       the epg db. This order is important. Otherwise deadlocks may occur.
    epg->Lock();
  }

  database->Lock();
  for (const auto& epg : epgs)
  {
    QueueDeleteEpg(epg, database);
    epg->Unlock();

    size_t queryCount = database->GetDeleteQueriesCount();
    if (queryCount > EPG_COMMIT_QUERY_COUNT_LIMIT)
      database->CommitDeleteQueries();
  }
  database->CommitDeleteQueries();
  database->Unlock();

  return true;
}

bool CPVREpgContainer::QueueDeleteEpg(const std::shared_ptr<const CPVREpg>& epg,
                                      const std::shared_ptr<CPVREpgDatabase>& database)
{
  if (!epg || epg->EpgID() < 0)
    return false;

  std::shared_ptr<CPVREpg> epgToDelete;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    const auto& epgEntry = m_epgIdToEpgMap.find(epg->EpgID());
    if (epgEntry == m_epgIdToEpgMap.end())
      return false;

    const auto& epgEntry1 = m_channelUidToEpgMap.find(std::make_pair(
        epg->GetChannelData()->ClientId(), epg->GetChannelData()->UniqueClientChannelId()));
    if (epgEntry1 != m_channelUidToEpgMap.end())
      m_channelUidToEpgMap.erase(epgEntry1);

    CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting EPG table {} ({})", epg->Name(), epg->EpgID());
    epgEntry->second->QueueDeleteQueries(database);

    epgToDelete = epgEntry->second;
    m_epgIdToEpgMap.erase(epgEntry);
  }

  epgToDelete->Events().Unsubscribe(this);
  epgToDelete->RemovedFromContainer();
  return true;
}

bool CPVREpgContainer::InterruptUpdate() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bStop ||
         m_bPreventUpdates ||
         (m_bPlaying && m_settings.GetBoolValue(CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV));
}

void CPVREpgContainer::WaitForUpdateFinish()
{
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_bPreventUpdates = true;

    if (!m_bIsUpdating)
      return;

    m_updateEvent.Reset();
  }

  m_updateEvent.Wait();
}

bool CPVREpgContainer::UpdateEPG(bool bOnlyPending /* = false */)
{
  bool bInterrupted = false;
  unsigned int iUpdatedTables = 0;
  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetUTCDateTime().GetAsTime(start);
  end = start + GetFutureDaysToDisplay() * 24 * 60 * 60;
  start -= GetPastDaysToDisplay() * 24 * 60 * 60;

  bool bShowProgress = (m_bIsInitialising || advancedSettings->m_bEpgDisplayIncrementalUpdatePopup) &&
                       advancedSettings->m_bEpgDisplayUpdatePopup;
  int pendingUpdates = 0;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (m_bIsUpdating || InterruptUpdate())
      return false;

    m_bIsUpdating = true;
    pendingUpdates = m_pendingUpdates;
  }

  std::vector<std::shared_ptr<CPVREpg>> invalidTables;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();

  m_critSection.lock();
  const auto epgsToUpdate = m_epgIdToEpgMap;
  m_critSection.unlock();

  std::unique_ptr<CPVRGUIProgressHandler> progressHandler;
  if (bShowProgress && !bOnlyPending && !epgsToUpdate.empty())
    progressHandler = std::make_unique<CPVRGUIProgressHandler>(
        g_localizeStrings.Get(19004)); // Loading programme guide

  size_t counter = 0;
  for (const auto& epgEntry : epgsToUpdate)
  {
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    const std::shared_ptr<CPVREpg> epg = epgEntry.second;
    if (!epg)
      continue;

    if (progressHandler)
      progressHandler->UpdateProgress(epg->GetChannelData()->ChannelName(), ++counter,
                                      epgsToUpdate.size());

    if ((!bOnlyPending || epg->UpdatePending()) &&
        epg->Update(start,
                    end,
                    m_settings.GetIntValue(CSettings::SETTING_EPG_EPGUPDATE) * 60,
                    m_settings.GetIntValue(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY),
                    database,
                    bOnlyPending))
    {
      iUpdatedTables++;
    }
    else if (!epg->IsValid())
    {
      invalidTables.push_back(epg);
    }
  }

  progressHandler.reset();

  QueueDeleteEpgs(invalidTables);

  if (bInterrupted)
  {
    /* the update has been interrupted. try again later */
    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);

    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_iNextEpgUpdate = iNow + advancedSettings->m_iEpgRetryInterruptedUpdateInterval;
  }
  else
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
    m_iNextEpgUpdate += advancedSettings->m_iEpgUpdateCheckInterval;
    if (m_pendingUpdates == pendingUpdates)
      m_pendingUpdates = 0;
  }

  if (iUpdatedTables > 0)
    m_events.Publish(PVREvent::EpgContainer);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_bIsUpdating = false;
  m_updateEvent.Set();

  return !bInterrupted;
}

std::pair<CDateTime, CDateTime> CPVREpgContainer::GetFirstAndLastEPGDate() const
{
  // Get values from db
  std::pair<CDateTime, CDateTime> dbDates;
  const std::shared_ptr<const CPVREpgDatabase> database = GetEpgDatabase();
  if (database)
    dbDates = database->GetFirstAndLastEPGDate();

  // Merge not yet committed changes
  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  CDateTime first(dbDates.first);
  CDateTime last(dbDates.second);

  for (const auto& epgEntry : epgs)
  {
    const auto dates = epgEntry.second->GetFirstAndLastUncommitedEPGDate();

    if (dates.first.IsValid() && (!first.IsValid() || dates.first < first))
      first = dates.first;

    if (dates.second.IsValid() && (!last.IsValid() || dates.second > last))
      last = dates.second;
  }

  return {first, last};
}

bool CPVREpgContainer::CheckPlayingEvents()
{
  bool bReturn = false;
  bool bFoundChanges = false;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  time_t iNextEpgActiveTagCheck = m_iNextEpgActiveTagCheck;
  m_critSection.unlock();

  time_t iNow;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
  if (iNow >= iNextEpgActiveTagCheck)
  {
    bFoundChanges = std::accumulate(epgs.cbegin(), epgs.cend(), bFoundChanges,
                                    [](bool found, const auto& epgEntry) {
                                      return epgEntry.second->CheckPlayingEvent() ? true : found;
                                    });

    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNextEpgActiveTagCheck);
    iNextEpgActiveTagCheck += CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEpgActiveTagCheckInterval;

    /* pvr tags always start on the full minute */
    if (CServiceBroker::GetPVRManager().IsStarted())
      iNextEpgActiveTagCheck -= iNextEpgActiveTagCheck % 60;

    bReturn = true;
  }

  if (bReturn)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_iNextEpgActiveTagCheck = iNextEpgActiveTagCheck;
  }

  if (bFoundChanges)
    m_events.Publish(PVREvent::EpgActiveItem);

  return bReturn;
}

void CPVREpgContainer::SetHasPendingUpdates(bool bHasPendingUpdates /* = true */)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (bHasPendingUpdates)
    m_pendingUpdates++;
  else
    m_pendingUpdates = 0;
}

void CPVREpgContainer::UpdateRequest(int iClientID, int iUniqueChannelID)
{
  std::unique_lock<CCriticalSection> lock(m_updateRequestsLock);
  m_updateRequests.emplace_back(iClientID, iUniqueChannelID);
}

void CPVREpgContainer::UpdateFromClient(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE eNewState)
{
  std::unique_lock<CCriticalSection> lock(m_epgTagChangesLock);
  m_epgTagChanges.emplace_back(tag, eNewState);
}

int CPVREpgContainer::GetPastDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY);
}

int CPVREpgContainer::GetFutureDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY);
}

void CPVREpgContainer::OnPlaybackStarted()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_bPlaying = true;
}

void CPVREpgContainer::OnPlaybackStopped()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_bPlaying = false;
}

int CPVREpgContainer::CleanupCachedImages()
{
  const std::shared_ptr<const CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return 0;
  }

  // Processing can take some time. Do not block.
  m_critSection.lock();
  const std::map<int, std::shared_ptr<CPVREpg>> epgIdToEpgMap = m_epgIdToEpgMap;
  m_critSection.unlock();

  return std::accumulate(epgIdToEpgMap.cbegin(), epgIdToEpgMap.cend(), 0,
                         [&database](int cleanedImages, const auto& epg) {
                           return cleanedImages + epg.second->CleanupCachedImages(database);
                         });
}

std::vector<std::shared_ptr<CPVREpgSearchFilter>> CPVREpgContainer::GetSavedSearches(
    bool bRadio) const
{
  const std::shared_ptr<const CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return {};
  }

  return database->GetSavedSearches(bRadio);
}

std::shared_ptr<CPVREpgSearchFilter> CPVREpgContainer::GetSavedSearchById(bool bRadio,
                                                                          int iId) const
{
  const std::shared_ptr<const CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return {};
  }

  return database->GetSavedSearchById(bRadio, iId);
}

bool CPVREpgContainer::PersistSavedSearch(CPVREpgSearchFilter& search)
{
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return {};
  }

  if (database->Persist(search))
  {
    m_events.Publish(PVREvent::SavedSearchesInvalidated);
    return true;
  }
  return false;
}

bool CPVREpgContainer::UpdateSavedSearchLastExecuted(const CPVREpgSearchFilter& epgSearch)
{
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return {};
  }

  return database->UpdateSavedSearchLastExecuted(epgSearch);
}

bool CPVREpgContainer::DeleteSavedSearch(const CPVREpgSearchFilter& search)
{
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return {};
  }

  if (database->Delete(search))
  {
    m_events.Publish(PVREvent::SavedSearchesInvalidated);
    return true;
  }
  return false;
}

} // namespace PVR
