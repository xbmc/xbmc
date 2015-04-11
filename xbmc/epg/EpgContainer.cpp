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

#include "Application.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "Epg.h"
#include "EpgContainer.h"
#include "EpgSearchFilter.h"

using namespace EPG;
using namespace PVR;

CEpgContainer::CEpgContainer(void) :
    CThread("EPGUpdater")
{
  m_progressHandle = NULL;
  m_bStop = true;
  m_bIsUpdating = false;
  m_bIsInitialising = true;
  m_iNextEpgId = 0;
  m_bPreventUpdates = false;
  m_updateEvent.Reset();
  m_bStarted = false;
  m_bLoaded = false;
  m_pendingUpdates = 0;
  m_iUpdateTime = 5 * 60;
  m_iLastEpgCleanup = 0;
  m_iNextEpgActiveTagCheck = 0;
  m_iNextEpgUpdate = 0;
  m_iDisplayTime = 24 * 60 * 60;
  m_bIgnoreDbForClient = false;
}

CEpgContainer::~CEpgContainer(void)
{
  Unload();
}

CEpgContainer &CEpgContainer::Get(void)
{
  static CEpgContainer epgInstance;
  return epgInstance;
}

void CEpgContainer::Unload(void)
{
  Stop();
  Clear(false);
}

bool CEpgContainer::IsStarted(void) const
{
  CSingleLock lock(m_critSection);
  return m_bStarted;
}

unsigned int CEpgContainer::NextEpgId(void)
{
  CSingleLock lock(m_critSection);
  return ++m_iNextEpgId;
}

void CEpgContainer::Clear(bool bClearDb /* = false */)
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
    for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
    {
      it->second->UnregisterObserver(this);
      delete it->second;
    }
    m_epgs.clear();
    m_iNextEpgUpdate  = 0;
    m_bStarted = false;
    m_bIsInitialising = true;
    m_iNextEpgId = 0;
  }

  /* clear the database entries */
  if (bClearDb && !m_bIgnoreDbForClient)
  {
    if (!m_database.IsOpen())
      m_database.Open();

    if (m_database.IsOpen())
      m_database.DeleteEpg();
  }

  SetChanged();
  NotifyObservers(ObservableMessageEpgContainer);

  if (bThreadRunning)
    Start();
}

void CEpgContainer::Start(void)
{
  Stop();

  {
    CSingleLock lock(m_critSection);

    if (!m_database.IsOpen())
      m_database.Open();

    m_bIsInitialising = true;
    m_bStop = false;
    LoadSettings();

    m_iNextEpgUpdate  = 0;
    m_iNextEpgActiveTagCheck = 0;
  }

  LoadFromDB();
  if (g_PVRManager.IsStarted())
    g_PVRManager.Recordings()->UpdateEpgTags();

  CSingleLock lock(m_critSection);
  if (!m_bStop)
  {
    CheckPlayingEvents();

    Create();
    SetPriority(-1);

    m_bStarted = true;

    CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
  }
}

bool CEpgContainer::Stop(void)
{
  StopThread();

  if (m_database.IsOpen())
    m_database.Close();

  CSingleLock lock(m_critSection);
  m_bStarted = false;

  return true;
}

void CEpgContainer::Notify(const Observable &obs, const ObservableMessage msg)
{
  SetChanged();
  NotifyObservers(msg);
}

void CEpgContainer::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "epg.ignoredbforclient" || settingId == "epg.epgupdate" ||
      settingId == "epg.daystodisplay")
    LoadSettings();
}

void CEpgContainer::LoadFromDB(void)
{
  CSingleLock lock(m_critSection);

  if (m_bLoaded || m_bIgnoreDbForClient)
    return;

  if (!m_database.IsOpen())
    m_database.Open();

  m_iNextEpgId = m_database.GetLastEPGId();

  bool bLoaded(true);
  unsigned int iCounter(0);
  if (m_database.IsOpen())
  {
    ShowProgressDialog(false);

    m_database.DeleteOldEpgEntries();
    m_database.Get(*this);

    for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
    {
      if (m_bStop)
        break;
      UpdateProgressDialog(++iCounter, m_epgs.size(), it->second->Name());
      lock.Leave();
      it->second->Load();
      lock.Enter();
    }

    CloseProgressDialog();
  }

  m_bLoaded = bLoaded;
}

bool CEpgContainer::PersistTables(void)
{
  m_critSection.lock();
  std::map<unsigned int, CEpg*> copy = m_epgs;
  m_critSection.unlock();
  return m_database.Persist(copy);
}

bool CEpgContainer::PersistAll(void)
{
  bool bReturn(true);
  m_critSection.lock();
  std::map<unsigned int, CEpg*> copy = m_epgs;
  m_critSection.unlock();

  for (EPGMAP_CITR it = copy.begin(); it != copy.end() && !m_bStop; it++)
  {
    CEpg *epg = it->second;
    if (epg && epg->NeedsSave())
    {
      bReturn &= epg->Persist();
    }
  }

  return bReturn;
}

void CEpgContainer::Process(void)
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
    if (!InterruptUpdate() && bUpdateEpg && g_PVRManager.EpgsCreated() && UpdateEPG())
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && iNow >= m_iLastEpgCleanup)
      RemoveOldEntries();

    /* check for pending manual EPG updates */
    while (!m_bStop)
    {
      SUpdateRequest request;
      {
        CSingleLock lock(m_updateRequestsLock);
        if (m_updateRequests.empty())
          break;
        request = m_updateRequests.front();
        m_updateRequests.pop_front();
      }

      // get the channel
      CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(request.channelID, request.clientID);
      CEpg* epg(NULL);

      // get the EPG for the channel
      if (!channel || (epg = channel->GetEPG()) == NULL)
      {
        CLog::Log(LOGERROR, "PVR - %s - invalid channel or channel doesn't have an EPG", __FUNCTION__);
        continue;
      }

      // force an update
      epg->ForceUpdate();
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

    /* check for changes that need to be saved every 60 seconds */
    if (iNow - iLastSave > 60)
    {
      PersistAll();
      iLastSave = iNow;
    }

    Sleep(1000);
  }
}

CEpg *CEpgContainer::GetById(int iEpgId) const
{
  if (iEpgId < 0)
    return NULL;

  CSingleLock lock(m_critSection);
  std::map<unsigned int, CEpg *>::const_iterator it = m_epgs.find((unsigned int) iEpgId);
  return it != m_epgs.end() ? it->second : NULL;
}

CEpgInfoTagPtr CEpgContainer::GetTagById(int iBroadcastId) const
{
  CEpgInfoTagPtr retval;
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CEpg *>::const_iterator it = m_epgs.begin(); !retval && it != m_epgs.end(); ++it)
    retval = it->second->GetTag(iBroadcastId);
  return retval;
}

CEpg *CEpgContainer::GetByChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);
  for (std::map<unsigned int, CEpg *>::const_iterator it = m_epgs.begin(); it != m_epgs.end(); ++it)
    if (channel.ChannelID() == it->second->ChannelID())
      return it->second;

  return NULL;
}

void CEpgContainer::InsertFromDatabase(int iEpgID, const std::string &strName, const std::string &strScraperName)
{
  // table might already have been created when pvr channels were loaded
  CEpg* epg = GetById(iEpgID);
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
    epg = new CEpg(iEpgID, strName, strScraperName, true);
    if (epg)
    {
      m_epgs.insert(std::make_pair(iEpgID, epg));
      SetChanged();
      epg->RegisterObserver(this);
    }
  }
}

CEpg *CEpgContainer::CreateChannelEpg(CPVRChannelPtr channel)
{
  if (!channel)
    return NULL;

  WaitForUpdateFinish(true);
  LoadFromDB();

  CEpg *epg(NULL);
  if (channel->EpgID() > 0)
    epg = GetById(channel->EpgID());

  if (!epg)
  {
    channel->SetEpgID(NextEpgId());
    epg = new CEpg(channel, false);

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

  NotifyObservers(ObservableMessageEpgContainer);

  return epg;
}

bool CEpgContainer::LoadSettings(void)
{
  m_bIgnoreDbForClient = CSettings::Get().GetBool("epg.ignoredbforclient");
  m_iUpdateTime        = CSettings::Get().GetInt ("epg.epgupdate") * 60;
  m_iDisplayTime       = CSettings::Get().GetInt ("epg.daystodisplay") * 24 * 60 * 60;

  return true;
}

bool CEpgContainer::RemoveOldEntries(void)
{
  CDateTime now = CDateTime::GetUTCDateTime() -
      CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);

  /* call Cleanup() on all known EPG tables */
  for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
    it->second->Cleanup(now);

  /* remove the old entries from the database */
  if (!m_bIgnoreDbForClient && m_database.IsOpen())
    m_database.DeleteOldEpgEntries();

  CSingleLock lock(m_critSection);
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iLastEpgCleanup);
  m_iLastEpgCleanup += g_advancedSettings.m_iEpgCleanupInterval;

  return true;
}

bool CEpgContainer::DeleteEpg(const CEpg &epg, bool bDeleteFromDatabase /* = false */)
{
  if (epg.EpgID() < 0)
    return false;

  CSingleLock lock(m_critSection);

  EPGMAP_ITR it = m_epgs.find((unsigned int)epg.EpgID());
  if (it == m_epgs.end())
    return false;

  CLog::Log(LOGDEBUG, "deleting EPG table %s (%d)", epg.Name().c_str(), epg.EpgID());
  if (bDeleteFromDatabase && !m_bIgnoreDbForClient && m_database.IsOpen())
    m_database.Delete(*it->second);

  it->second->UnregisterObserver(this);
  delete it->second;
  m_epgs.erase(it);

  return true;
}

void CEpgContainer::CloseProgressDialog(void)
{
  if (m_progressHandle)
  {
    m_progressHandle->MarkFinished();
    m_progressHandle = NULL;
  }
}

void CEpgContainer::ShowProgressDialog(bool bUpdating /* = true */)
{
  if (!m_progressHandle)
  {
    CGUIDialogExtendedProgressBar *progressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    if (progressDialog)
      m_progressHandle = progressDialog->GetHandle(bUpdating ? g_localizeStrings.Get(19004) : g_localizeStrings.Get(19250));
  }
}

void CEpgContainer::UpdateProgressDialog(int iCurrent, int iMax, const std::string &strText)
{
  if (!m_progressHandle)
    ShowProgressDialog();

  if (m_progressHandle)
  {
    m_progressHandle->SetProgress(iCurrent, iMax);
    m_progressHandle->SetText(strText);
  }
}

bool CEpgContainer::InterruptUpdate(void) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  bReturn = g_application.m_bStop || m_bStop || m_bPreventUpdates;

  return bReturn ||
    (CSettings::Get().GetBool("epg.preventupdateswhileplayingtv") &&
     g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying());
}

void CEpgContainer::WaitForUpdateFinish(bool bInterrupt /* = true */)
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

bool CEpgContainer::UpdateEPG(bool bOnlyPending /* = false */)
{
  bool bInterrupted(false);
  unsigned int iUpdatedTables(0);
  bool bShowProgress(false);
  int pendingUpdates(0);

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(start);
  end = start + m_iDisplayTime;
  start -= g_advancedSettings.m_iEpgLingerTime * 60;
  bShowProgress = g_advancedSettings.m_bEpgDisplayUpdatePopup && (m_bIsInitialising || g_advancedSettings.m_bEpgDisplayIncrementalUpdatePopup);

  {
    CSingleLock lock(m_critSection);
    if (m_bIsUpdating || InterruptUpdate())
      return false;
    m_bIsUpdating = true;
    pendingUpdates = m_pendingUpdates;
  }

  if (bShowProgress && !bOnlyPending)
    ShowProgressDialog();

  if (!m_bIgnoreDbForClient && !m_database.IsOpen())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - could not open the database", __FUNCTION__);

    {
      CSingleLock lock(m_critSection);
      m_bIsUpdating = false;
      m_updateEvent.Set();
    }

    g_PVRManager.Recordings()->UpdateEpgTags();

    if (bShowProgress && !bOnlyPending)
      CloseProgressDialog();

    return false;
  }

  std::vector<CEpg*> invalidTables;

  /* load or update all EPG tables */
  CEpg *epg;
  unsigned int iCounter(0);
  for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
  {
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    epg = it->second;
    if (!epg)
      continue;

    if (bShowProgress && !bOnlyPending)
      UpdateProgressDialog(++iCounter, m_epgs.size(), epg->Name());

    // we currently only support update via pvr add-ons. skip update when the pvr manager isn't started
    if (!g_PVRManager.IsStarted())
      continue;

    // check the pvr manager when the channel pointer isn't set
    if (!epg->Channel())
    {
      CPVRChannelPtr channel = g_PVRChannelGroups->GetChannelByEpgId(epg->EpgID());
      if (channel)
        epg->SetChannel(channel);
    }

    if ((!bOnlyPending || epg->UpdatePending()) && epg->Update(start, end, m_iUpdateTime, bOnlyPending))
      iUpdatedTables++;
    else if (!epg->IsValid())
      invalidTables.push_back(epg);
  }

  for (std::vector<CEpg*>::iterator it = invalidTables.begin(); it != invalidTables.end(); ++it)
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

  if (bShowProgress && !bOnlyPending)
    CloseProgressDialog();

  /* notify observers */
  if (iUpdatedTables > 0)
  {
    SetChanged();
    NotifyObservers(ObservableMessageEpgContainer);
  }

  CSingleLock lock(m_critSection);
  m_bIsUpdating = false;
  m_updateEvent.Set();

  return !bInterrupted;
}

int CEpgContainer::GetEPGAll(CFileItemList &results)
{
  int iInitialSize = results.Size();

  CSingleLock lock(m_critSection);
  for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
    it->second->Get(results);

  return results.Size() - iInitialSize;
}

const CDateTime CEpgContainer::GetFirstEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
  {
    lock.Leave();
    CDateTime entry = it->second->GetFirstDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry < returnValue))
      returnValue = entry;
    lock.Enter();
  }

  return returnValue;
}

const CDateTime CEpgContainer::GetLastEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
  {
    lock.Leave();
    CDateTime entry = it->second->GetLastDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry > returnValue))
      returnValue = entry;
    lock.Enter();
  }

  return returnValue;
}

int CEpgContainer::GetEPGSearch(CFileItemList &results, const EpgSearchFilter &filter)
{
  int iInitialSize = results.Size();

  /* get filtered results from all tables */
  {
    CSingleLock lock(m_critSection);
    for (EPGMAP_CITR it = m_epgs.begin(); it != m_epgs.end(); it++)
      it->second->Get(results, filter);
  }

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
    EpgSearchFilter::RemoveDuplicates(results);

  return results.Size() - iInitialSize;
}

bool CEpgContainer::CheckPlayingEvents(void)
{
  bool bReturn(false);
  time_t iNow;
  bool bFoundChanges(false);

  {
    CSingleLock lock(m_critSection);
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    if (iNow >= m_iNextEpgActiveTagCheck)
    {
      for (EPGMAP_ITR it = m_epgs.begin(); it != m_epgs.end(); it++)
        bFoundChanges = it->second->CheckPlayingEvent() || bFoundChanges;
      CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgActiveTagCheck);
      m_iNextEpgActiveTagCheck += g_advancedSettings.m_iEpgActiveTagCheckInterval;

      /* pvr tags always start on the full minute */
      if (g_PVRManager.IsStarted())
        m_iNextEpgActiveTagCheck -= m_iNextEpgActiveTagCheck % 60;

      bReturn = true;
    }
  }

  if (bFoundChanges)
  {
    SetChanged();
    NotifyObservers(ObservableMessageEpgActiveItem);
  }
  return bReturn;
}

bool CEpgContainer::IsInitialising(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsInitialising;
}

void CEpgContainer::SetHasPendingUpdates(bool bHasPendingUpdates /* = true */)
{
  CSingleLock lock(m_critSection);
  if (bHasPendingUpdates)
    m_pendingUpdates++;
  else
    m_pendingUpdates = 0;
}

void CEpgContainer::UpdateRequest(int clientID, unsigned int channelID)
{
  CSingleLock lock(m_updateRequestsLock);
  SUpdateRequest request;
  request.clientID = clientID;
  request.channelID = channelID;
  m_updateRequests.push_back(request);
}
