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

#include "EpgContainer.h"

#include <utility>

#include "Application.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/PVRGUIProgressHandler.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"

namespace PVR
{

class CEpgUpdateRequest
{
public:
  CEpgUpdateRequest() : CEpgUpdateRequest(-1, PVR_CHANNEL_INVALID_UID) {}
  CEpgUpdateRequest(int iClientID, unsigned int iUniqueChannelID) : m_iClientID(iClientID), m_iUniqueChannelID(iUniqueChannelID) {}

  void Deliver();

private:
  int m_iClientID;
  unsigned int m_iUniqueChannelID;
};

void CEpgUpdateRequest::Deliver()
{
  const CPVRChannelPtr channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_iUniqueChannelID, m_iClientID);
  if (!channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel (%d)! Unable to do the epg update!", __FUNCTION__, m_iUniqueChannelID);
    return;
  }

  const CPVREpgPtr epg = channel->GetEPG();
  if (!epg)
  {
    CLog::Log(LOGERROR, "PVR - %s - channel '%s' does not have an EPG! Unable to do the epg update!", __FUNCTION__, channel->ChannelName().c_str());
    return;
  }

  epg->ForceUpdate();
}

class CEpgTagStateChange
{
public:
  CEpgTagStateChange() : m_state(EPG_EVENT_CREATED) {}
  CEpgTagStateChange(const CPVREpgInfoTagPtr tag, EPG_EVENT_STATE eNewState) : m_epgtag(tag), m_state(eNewState) {}

  void Deliver();

private:
  CPVREpgInfoTagPtr m_epgtag;
  EPG_EVENT_STATE m_state;
};

void CEpgTagStateChange::Deliver()
{
  const CPVRChannelPtr channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_epgtag->UniqueChannelID(), m_epgtag->ClientID());
  if (!channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel (%d)! Unable to deliver state change for tag '%d'!",
              __FUNCTION__, m_epgtag->UniqueChannelID(), m_epgtag->UniqueBroadcastID());
    return;
  }

  const CPVREpgPtr epg = channel->GetEPG();
  if (!epg)
  {
    CLog::Log(LOGERROR, "PVR - %s - channel '%s' does not have an EPG! Unable to deliver state change for tag '%d'!",
              __FUNCTION__, channel->ChannelName().c_str(), m_epgtag->UniqueBroadcastID());
    return;
  }

  // update
  if (!epg->UpdateEntry(m_epgtag, m_state, false))
    CLog::Log(LOGERROR, "PVR - %s - update failed for epgtag change for channel '%s'", __FUNCTION__, channel->ChannelName().c_str());
}

CPVREpgContainer::CPVREpgContainer(void) :
  CThread("EPGUpdater"),
  m_bUpdateNotificationPending(false),
  m_settings({
    CSettings::SETTING_EPG_IGNOREDBFORCLIENT,
    CSettings::SETTING_EPG_EPGUPDATE,
    CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY,
    CSettings::SETTING_EPG_PAST_DAYSTODISPLAY,
    CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV
  })
{
  m_bStop = true;
  m_bIsUpdating = false;
  m_bIsInitialising = true;
  m_iNextEpgId = 0;
  m_bPreventUpdates = false;
  m_updateEvent.Reset();
  m_bStarted = false;
  m_bLoaded = false;
  m_pendingUpdates = 0;
  m_iLastEpgCleanup = 0;
  m_iNextEpgActiveTagCheck = 0;
  m_iNextEpgUpdate = 0;
}

CPVREpgContainer::~CPVREpgContainer(void)
{
  Unload();
}

void CPVREpgContainer::Unload(void)
{
  Stop();
  Clear(false);
}

bool CPVREpgContainer::IsStarted(void) const
{
  CSingleLock lock(m_critSection);
  return m_bStarted;
}

unsigned int CPVREpgContainer::NextEpgId(void)
{
  CSingleLock lock(m_critSection);
  return ++m_iNextEpgId;
}

void CPVREpgContainer::Clear(bool bClearDb /* = false */)
{
  /* make sure the update thread is stopped */
  bool bThreadRunning = !m_bStop;
  if (bThreadRunning && !Stop())
  {
    CLog::Log(LOGERROR, "%s - cannot stop the update thread", __FUNCTION__);
    return;
  }

  {
    CSingleLock lock(m_critSection);
    /* clear all epg tables and remove pointers to epg tables on channels */
    for (const auto &epgEntry : m_epgs)
    {
      epgEntry.second->UnregisterObserver(this);
    }
    m_epgs.clear();
    m_iNextEpgUpdate  = 0;
    m_bStarted = false;
    m_bIsInitialising = true;
    m_iNextEpgId = 0;
    m_bUpdateNotificationPending = false;
  }

  /* clear the database entries */
  if (bClearDb && !IgnoreDB())
  {
    if (!m_database.IsOpen())
      m_database.Open();

    if (m_database.IsOpen())
      m_database.DeleteEpg();
  }

  SetChanged();
  {
    CSingleExit ex(m_critSection);
    NotifyObservers(ObservableMessageEpgContainer);
  }

  if (bThreadRunning)
    Start(true);
}

class CPVREpgContainerStartJob : public CJob
{
public:
  CPVREpgContainerStartJob() = default;
  ~CPVREpgContainerStartJob(void) override = default;

  bool DoWork(void) override
  {
    CServiceBroker::GetPVRManager().EpgContainer().Start(false);
    return true;
  }
};

void CPVREpgContainer::Start(bool bAsync)
{
  if (bAsync)
  {
    CPVREpgContainerStartJob *job = new CPVREpgContainerStartJob();
    CJobManager::GetInstance().AddJob(job, NULL);
    return;
  }

  Stop();

  {
    CSingleLock lock(m_critSection);

    if (!m_database.IsOpen())
      m_database.Open();

    m_bIsInitialising = true;
    m_bStop = false;

    m_iNextEpgUpdate  = 0;
    m_iNextEpgActiveTagCheck = 0;
    m_bUpdateNotificationPending = false;
  }

  LoadFromDB();

  bool bStop = false;
  {
    CSingleLock lock(m_critSection);
    bStop = m_bStop;
    if (!m_bStop)
    {
      CheckPlayingEvents();

      Create();
      SetPriority(-1);

      m_bStarted = true;
    }
  }

  if (!bStop)
  {
    CServiceBroker::GetPVRManager().TriggerEpgsCreate();
    CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
  }
}

bool CPVREpgContainer::Stop(void)
{
  StopThread();

  if (m_database.IsOpen())
    m_database.Close();

  CSingleLock lock(m_critSection);
  m_bStarted = false;

  return true;
}

void CPVREpgContainer::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageEpgItemUpdate)
  {
    // there can be many of these notifications during short time period. Thus, announce async and not every event.
    CSingleLock lock(m_critSection);
    m_bUpdateNotificationPending = true;
    return;
  }

  SetChanged();
  CSingleExit ex(m_critSection);
  NotifyObservers(msg);
}

void CPVREpgContainer::LoadFromDB(void)
{
  CSingleLock lock(m_critSection);

  if (m_bLoaded || IgnoreDB())
    return;

  if (!m_database.IsOpen())
    m_database.Open();

  m_iNextEpgId = m_database.GetLastEPGId();

  bool bLoaded(true);
  unsigned int iCounter(0);
  if (m_database.IsOpen())
  {
    CPVRGUIProgressHandler* progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19250)); // Loading guide from database

    const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(GetPastDaysToDisplay(), 0, 0, 0));
    m_database.DeleteEpgEntries(cleanupTime);
    m_database.Get(*this);

    for (const auto &epgEntry : m_epgs)
    {
      if (m_bStop)
        break;

      progressHandler->UpdateProgress(epgEntry.second->Name(), ++iCounter, m_epgs.size());

      lock.Leave();
      epgEntry.second->Load();
      lock.Enter();
    }

    progressHandler->DestroyProgress();
  }

  m_bLoaded = bLoaded;
}

bool CPVREpgContainer::PersistAll(void)
{
  bool bReturn(true);
  m_critSection.lock();
  auto copy = m_epgs;
  m_critSection.unlock();

  for (EPGMAP::const_iterator it = copy.begin(); it != copy.end() && !m_bStop; ++it)
  {
    CPVREpgPtr epg = it->second;
    if (epg && epg->NeedsSave())
    {
      bReturn &= epg->Persist();
    }
  }

  return bReturn;
}

void CPVREpgContainer::Process(void)
{
  time_t iNow(0), iLastSave(0);
  bool bUpdateEpg(true);
  bool bHasPendingUpdates(false);

  while (!m_bStop && !g_application.m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    {
      CSingleLock lock(m_critSection);
      bUpdateEpg = (iNow >= m_iNextEpgUpdate);
    }

    /* update the EPG */
    if (!InterruptUpdate() && bUpdateEpg && CServiceBroker::GetPVRManager().EpgsCreated() && UpdateEPG())
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && iNow >= m_iLastEpgCleanup + g_advancedSettings.m_iEpgCleanupInterval)
      RemoveOldEntries();

    /* check for pending manual EPG updates */

    while (!m_bStop)
    {
      CEpgUpdateRequest request;
      {
        CSingleLock lock(m_updateRequestsLock);
        if (m_updateRequests.empty())
          break;

        request = m_updateRequests.front();
        m_updateRequests.pop_front();
      }

      // do the update
      request.Deliver();
    }

    /* check for pending EPG tag changes */

    // during Kodi startup, addons may push updates very early, even before EPGs are ready to use.
    if (!m_bStop && CServiceBroker::GetPVRManager().EpgsCreated())
    {
      unsigned int iProcessed = 0;
      XbmcThreads::EndTime processTimeslice(1000); // max 1 sec per cycle, regardless of how many events are in the queue

      while (!m_bStop)
      {
        CEpgTagStateChange change;
        {
          CSingleLock lock(m_epgTagChangesLock);
          if (processTimeslice.IsTimePast() || m_epgTagChanges.empty())
          {
            if (iProcessed > 0)
              CLog::Log(LOGDEBUG, "PVR - %s - processed %ld queued epg event changes.", __FUNCTION__, iProcessed);

            break;
          }

          change = m_epgTagChanges.front();
          m_epgTagChanges.pop_front();
        }

        iProcessed++;

        // deliver the updated tag to the respective epg
        change.Deliver();
      }
    }

    if (!m_bStop)
    {
      {
        CSingleLock lock(m_critSection);
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
      CSingleLock lock(m_critSection);
      if (m_bUpdateNotificationPending)
      {
        m_bUpdateNotificationPending = false;
        SetChanged();

        CSingleExit ex(m_critSection);
        NotifyObservers(ObservableMessageEpg);
      }
    }

    /* check for changes that need to be saved every 60 seconds */
    if (iNow - iLastSave > 60)
    {
      PersistAll();
      iLastSave = iNow;
    }

    Sleep(1000);
  }
}

CPVREpgPtr CPVREpgContainer::GetById(int iEpgId) const
{
  if (iEpgId < 0)
    return CPVREpgPtr();

  CSingleLock lock(m_critSection);
  const auto &epgEntry = m_epgs.find((unsigned int) iEpgId);
  return epgEntry != m_epgs.end() ? epgEntry->second : CPVREpgPtr();
}

CPVREpgInfoTagPtr CPVREpgContainer::GetTagById(const CPVRChannelPtr &channel, unsigned int iBroadcastId) const
{
  CPVREpgInfoTagPtr retval;

  if (iBroadcastId == EPG_TAG_INVALID_UID)
    return retval;

  if (channel)
  {
    const CPVREpgPtr epg(channel->GetEPG());
    if (epg)
      retval = epg->GetTagByBroadcastId(iBroadcastId);
  }
  else
  {
    for (const auto &epgEntry : m_epgs)
    {
      retval = epgEntry.second->GetTagByBroadcastId(iBroadcastId);
      if (retval)
        break;
    }
  }

  return retval;
}

std::vector<CPVREpgInfoTagPtr> CPVREpgContainer::GetEpgTagsForTimer(const CPVRTimerInfoTagPtr &timer) const
{
  CPVRChannelPtr channel(timer->Channel());

  if (!channel)
    channel = timer->UpdateChannel();

  if (channel)
  {
    const CPVREpgPtr epg(channel->GetEPG());
    if (epg)
      return epg->GetTagsBetween(timer->StartAsUTC(), timer->EndAsUTC());
  }
  return std::vector<CPVREpgInfoTagPtr>();
}

void CPVREpgContainer::InsertFromDatabase(int iEpgID, const std::string &strName, const std::string &strScraperName)
{
  // table might already have been created when pvr channels were loaded
  CPVREpgPtr epg = GetById(iEpgID);
  if (epg)
  {
    if (epg->Name() != strName || epg->ScraperName() != strScraperName)
    {
      // current table data differs from the info in the db
      epg->SetChanged();
      SetChanged();
    }
  }
  else
  {
    // create a new epg table
    epg.reset(new CPVREpg(iEpgID, strName, strScraperName, true));
    if (epg)
    {
      m_epgs.insert(std::make_pair(iEpgID, epg));
      SetChanged();
      epg->RegisterObserver(this);
    }
  }
}

CPVREpgPtr CPVREpgContainer::CreateChannelEpg(const CPVRChannelPtr &channel)
{
  if (!channel)
    return CPVREpgPtr();

  WaitForUpdateFinish(true);
  LoadFromDB();

  CPVREpgPtr epg;
  if (channel->EpgID() > 0)
    epg = GetById(channel->EpgID());

  if (!epg)
  {
    channel->SetEpgID(NextEpgId());
    epg.reset(new CPVREpg(channel, false));

    CSingleLock lock(m_critSection);
    m_epgs.insert(std::make_pair((unsigned int)epg->EpgID(), epg));
    SetChanged();
    epg->RegisterObserver(this);
  }

  epg->SetChannel(channel);

  {
    CSingleLock lock(m_critSection);
    m_bPreventUpdates = false;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
  }

  CSingleExit ex(m_critSection);
  NotifyObservers(ObservableMessageEpgContainer);

  return epg;
}

bool CPVREpgContainer::RemoveOldEntries(void)
{
  const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(GetPastDaysToDisplay(), 0, 0, 0));

  /* call Cleanup() on all known EPG tables */
  for (const auto &epgEntry : m_epgs)
    epgEntry.second->Cleanup(cleanupTime);

  /* remove the old entries from the database */
  if (!IgnoreDB() && m_database.IsOpen())
    m_database.DeleteEpgEntries(cleanupTime);

  CSingleLock lock(m_critSection);
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iLastEpgCleanup);

  return true;
}

bool CPVREpgContainer::DeleteEpg(const CPVREpg &epg, bool bDeleteFromDatabase /* = false */)
{
  if (epg.EpgID() < 0)
    return false;

  CSingleLock lock(m_critSection);

  const auto &epgEntry = m_epgs.find((unsigned int)epg.EpgID());
  if (epgEntry == m_epgs.end())
    return false;

  CLog::Log(LOGDEBUG, "deleting EPG table %s (%d)", epg.Name().c_str(), epg.EpgID());
  if (bDeleteFromDatabase && !IgnoreDB() && m_database.IsOpen())
    m_database.Delete(*epgEntry->second);

  epgEntry->second->UnregisterObserver(this);
  m_epgs.erase(epgEntry);

  return true;
}

bool CPVREpgContainer::IgnoreDB() const
{
  return m_settings.GetBoolValue(CSettings::SETTING_EPG_IGNOREDBFORCLIENT);
}

bool CPVREpgContainer::InterruptUpdate(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  bReturn = g_application.m_bStop || m_bStop || m_bPreventUpdates;

  return bReturn ||
    (m_settings.GetBoolValue(CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV) &&
     g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying());
}

void CPVREpgContainer::WaitForUpdateFinish(bool bInterrupt /* = true */)
{
  {
    CSingleLock lock(m_critSection);
    if (bInterrupt)
      m_bPreventUpdates = true;

    if (!m_bIsUpdating)
      return;

    m_updateEvent.Reset();
  }

  m_updateEvent.Wait();
}

bool CPVREpgContainer::UpdateEPG(bool bOnlyPending /* = false */)
{
  bool bInterrupted(false);
  unsigned int iUpdatedTables(0);
  bool bShowProgress(false);
  int pendingUpdates(0);

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetUTCDateTime().GetAsTime(start);
  end = start + GetFutureDaysToDisplay() * 24 * 60 * 60;
  start -= GetPastDaysToDisplay() * 24 * 60 * 60;
  bShowProgress = g_advancedSettings.m_bEpgDisplayUpdatePopup && (m_bIsInitialising || g_advancedSettings.m_bEpgDisplayIncrementalUpdatePopup);

  {
    CSingleLock lock(m_critSection);
    if (m_bIsUpdating || InterruptUpdate())
      return false;
    m_bIsUpdating = true;
    pendingUpdates = m_pendingUpdates;
  }

  if (!IgnoreDB() && !m_database.IsOpen())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - could not open the database", __FUNCTION__);

    {
      CSingleLock lock(m_critSection);
      m_bIsUpdating = false;
      m_updateEvent.Set();
    }

    return false;
  }

  std::vector<CPVREpgPtr> invalidTables;

  CPVRGUIProgressHandler* progressHandler = nullptr;
  if (bShowProgress && !bOnlyPending)
    progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19004)); // Importing guide from clients

  /* load or update all EPG tables */
  unsigned int iCounter(0);
  for (const auto &epgEntry : m_epgs)
  {
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    CPVREpgPtr epg = epgEntry.second;
    if (!epg)
      continue;

    if (bShowProgress && !bOnlyPending)
      progressHandler->UpdateProgress(epg->Name(), ++iCounter, m_epgs.size());

    // we currently only support update via pvr add-ons. skip update when the pvr manager isn't started
    if (!CServiceBroker::GetPVRManager().IsStarted())
      continue;

    // check the pvr manager when the channel pointer isn't set
    if (!epg->Channel())
    {
      CPVRChannelPtr channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelByEpgId(epg->EpgID());
      if (channel)
        epg->SetChannel(channel);
    }

    if ((!bOnlyPending || epg->UpdatePending()) &&
        epg->Update(start, end, m_settings.GetIntValue(CSettings::SETTING_EPG_EPGUPDATE) * 60, bOnlyPending))
      iUpdatedTables++;
    else if (!epg->IsValid())
      invalidTables.push_back(epg);
  }

  if (bShowProgress && !bOnlyPending)
    progressHandler->DestroyProgress();

  for (auto it = invalidTables.begin(); it != invalidTables.end(); ++it)
    DeleteEpg(**it, true);

  if (bInterrupted)
  {
    /* the update has been interrupted. try again later */
    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    m_iNextEpgUpdate = iNow + g_advancedSettings.m_iEpgRetryInterruptedUpdateInterval;
  }
  else
  {
    CSingleLock lock(m_critSection);
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
    m_iNextEpgUpdate += g_advancedSettings.m_iEpgUpdateCheckInterval;
    if (m_pendingUpdates == pendingUpdates)
      m_pendingUpdates = 0;
  }

  /* notify observers */
  if (iUpdatedTables > 0)
  {
    SetChanged();
    CSingleExit ex(m_critSection);
    NotifyObservers(ObservableMessageEpgContainer);
  }

  CSingleLock lock(m_critSection);
  m_bIsUpdating = false;
  m_updateEvent.Set();

  return !bInterrupted;
}

const CDateTime CPVREpgContainer::GetFirstEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (const auto &epgEntry : m_epgs)
  {
    lock.Leave();
    CDateTime entry = epgEntry.second->GetFirstDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry < returnValue))
      returnValue = entry;
    lock.Enter();
  }

  return returnValue;
}

const CDateTime CPVREpgContainer::GetLastEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (const auto &epgEntry : m_epgs)
  {
    lock.Leave();
    CDateTime entry = epgEntry.second->GetLastDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry > returnValue))
      returnValue = entry;
    lock.Enter();
  }

  return returnValue;
}

int CPVREpgContainer::GetEPGSearch(CFileItemList &results, const CPVREpgSearchFilter &filter)
{
  int iInitialSize = results.Size();

  /* get filtered results from all tables */
  {
    CSingleLock lock(m_critSection);
    for (const auto &epgEntry : m_epgs)
      epgEntry.second->Get(results, filter);
  }

  /* remove duplicate entries */
  if (filter.ShouldRemoveDuplicates())
    filter.RemoveDuplicates(results);

  return results.Size() - iInitialSize;
}

bool CPVREpgContainer::CheckPlayingEvents(void)
{
  bool bReturn(false);
  bool bFoundChanges(false);

  {
    time_t iNextEpgActiveTagCheck;
    {
      CSingleLock lock(m_critSection);
      iNextEpgActiveTagCheck = m_iNextEpgActiveTagCheck;
    }

    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    if (iNow >= iNextEpgActiveTagCheck)
    {
      for (const auto &epgEntry : m_epgs)
        bFoundChanges = epgEntry.second->CheckPlayingEvent() || bFoundChanges;

      CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNextEpgActiveTagCheck);
      iNextEpgActiveTagCheck += g_advancedSettings.m_iEpgActiveTagCheckInterval;

      /* pvr tags always start on the full minute */
      if (CServiceBroker::GetPVRManager().IsStarted())
        iNextEpgActiveTagCheck -= iNextEpgActiveTagCheck % 60;

      bReturn = true;
    }

    if (bReturn)
    {
      CSingleLock lock(m_critSection);
      m_iNextEpgActiveTagCheck = iNextEpgActiveTagCheck;
    }
  }

  if (bFoundChanges)
  {
    SetChanged();
    CSingleExit ex(m_critSection);
    NotifyObservers(ObservableMessageEpgActiveItem);
  }
  return bReturn;
}

void CPVREpgContainer::SetHasPendingUpdates(bool bHasPendingUpdates /* = true */)
{
  CSingleLock lock(m_critSection);
  if (bHasPendingUpdates)
    m_pendingUpdates++;
  else
    m_pendingUpdates = 0;
}

void CPVREpgContainer::UpdateRequest(int iClientID, unsigned int iUniqueChannelID)
{
  CSingleLock lock(m_updateRequestsLock);
  m_updateRequests.emplace_back(CEpgUpdateRequest(iClientID, iUniqueChannelID));
}

void CPVREpgContainer::UpdateFromClient(const CPVREpgInfoTagPtr tag, EPG_EVENT_STATE eNewState)
{
  CSingleLock lock(m_epgTagChangesLock);
  m_epgTagChanges.emplace_back(CEpgTagStateChange(tag, eNewState));
}

int CPVREpgContainer::GetPastDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY);
}

int CPVREpgContainer::GetFutureDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY);
}

} // namespace PVR
