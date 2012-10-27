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
#include "threads/SingleLock.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"

#include "EpgContainer.h"
#include "Epg.h"
#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

using namespace std;
using namespace EPG;
using namespace PVR;

typedef std::map<int, CEpg*>::iterator EPGITR;

CEpgContainer::CEpgContainer(void) :
    CThread("EPG updater")
{
  m_progressHandle = NULL;
  m_bStop = true;
  m_bIsUpdating = false;
  m_bIsInitialising = true;
  m_iNextEpgId = 0;
  m_bPreventUpdates = false;
  m_updateEvent.Reset();
  m_bLoaded = false;
  m_bHasPendingUpdates = false;
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
    for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
    {
      it->second->UnregisterObserver(this);
      delete it->second;
    }
    m_epgs.clear();
    m_iNextEpgUpdate  = 0;
    m_bIsInitialising = true;
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

  CSingleLock lock(m_critSection);

  if (!m_database.IsOpen())
    m_database.Open();

  m_bIsInitialising = true;
  m_bStop = false;
  g_guiSettings.RegisterObserver(this);
  LoadSettings();

  m_iNextEpgUpdate  = 0;
  m_iNextEpgActiveTagCheck = 0;

  Create();
  SetPriority(-1);
  CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
}

bool CEpgContainer::Stop(void)
{
  StopThread();

  if (m_database.IsOpen())
    m_database.Close();

  return true;
}

void CEpgContainer::Notify(const Observable &obs, const ObservableMessage msg)
{
  /* settings were updated */
  if (msg == ObservableMessageGuiSettings)
    LoadSettings();
  else
  {
    SetChanged();
    NotifyObservers(msg);
  }
}

void CEpgContainer::LoadFromDB(void)
{
  bool bLoaded(true);
  unsigned int iCounter(0);
  if (!m_bIgnoreDbForClient && m_database.IsOpen())
  {
    ShowProgressDialog(false);

    m_database.DeleteOldEpgEntries();
    m_database.Get(*this);

    for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
    {
      UpdateProgressDialog(++iCounter, m_epgs.size(), it->second->Name());
      it->second->Load();
    }

    CloseProgressDialog();
  }

  CSingleLock lock(m_critSection);
  m_bLoaded = bLoaded;
}

bool CEpgContainer::PersistTables(void)
{
  return m_database.Persist(*this);
}

bool CEpgContainer::PersistAll(void)
{
  bool bReturn(true);
  CSingleLock lock(m_critSection);
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
  {
    CEpg *epg = it->second;
    lock.Leave();
    if (epg)
      bReturn &= epg->Persist(false);
    lock.Enter();
  }

  return bReturn;
}

void CEpgContainer::Process(void)
{
  time_t iNow       = 0;

  bool bUpdateEpg(true);
  bool bHasPendingUpdates(false);

  if (!m_bLoaded)
  {
    LoadFromDB();
    CheckPlayingEvents();
  }

  while (!m_bStop && !g_application.m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    {
      CSingleLock lock(m_critSection);
      bUpdateEpg = (iNow >= m_iNextEpgUpdate);
    }

    /* update the EPG */
    if (!InterruptUpdate() && bUpdateEpg && UpdateEPG())
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && iNow >= m_iLastEpgCleanup)
      RemoveOldEntries();

    /* check for pending manual EPG updates */
    if (!m_bStop)
    {
      {
        CSingleLock lock(m_critSection);
        bHasPendingUpdates = m_bHasPendingUpdates;
      }

      if (bHasPendingUpdates)
        UpdateEPG(true);
    }

    /* check for updated active tag */
    if (!m_bStop)
      CheckPlayingEvents();

    Sleep(1000);
  }

  g_guiSettings.UnregisterObserver(this);
}

CEpg *CEpgContainer::GetById(int iEpgId) const
{
  if (iEpgId < 0)
    return NULL;

  CSingleLock lock(m_critSection);
  map<unsigned int, CEpg *>::const_iterator it = m_epgs.find((unsigned int) iEpgId);
  return it != m_epgs.end() ? it->second : NULL;
}

CEpg *CEpgContainer::GetByChannel(const CPVRChannel &channel) const
{
  CSingleLock lock(m_critSection);
  for (map<unsigned int, CEpg *>::const_iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
    if (channel.ChannelID() == it->second->ChannelID())
      return it->second;

  return NULL;
}

void CEpgContainer::InsertFromDatabase(int iEpgID, const CStdString &strName, const CStdString &strScraperName)
{
  CEpg* epg = GetById(iEpgID);
  if (epg)
  {
    if (!epg->Name().Equals(strName) || !epg->ScraperName().Equals(strScraperName))
      SetChanged();
  }
  else
  {
    epg = new CEpg(iEpgID, strName, strScraperName, true);
    if (epg)
    {
      m_epgs.insert(make_pair(iEpgID, epg));
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
  CSingleLock lock(m_critSection);

  CEpg *epg(NULL);
  if (channel->EpgID() > 0)
    epg = GetById(channel->EpgID());

  if (!epg)
  {
    channel->SetEpgID(NextEpgId());
    epg = new CEpg(channel, false);
    m_epgs.insert(make_pair((unsigned int)epg->EpgID(), epg));
    SetChanged();
    epg->RegisterObserver(this);
  }

  epg->SetChannel(channel);

  m_bPreventUpdates = false;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);

  NotifyObservers(ObservableMessageEpgContainer);

  return epg;
}

bool CEpgContainer::LoadSettings(void)
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("epg.ignoredbforclient");
  m_iUpdateTime        = g_guiSettings.GetInt ("epg.epgupdate") * 60;
  m_iDisplayTime       = g_guiSettings.GetInt ("epg.daystodisplay") * 24 * 60 * 60;

  return true;
}

bool CEpgContainer::RemoveOldEntries(void)
{
  CDateTime now = CDateTime::GetUTCDateTime() -
      CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);

  /* call Cleanup() on all known EPG tables */
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
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

  map<unsigned int, CEpg *>::iterator it = m_epgs.find((unsigned int)epg.EpgID());
  if (it == m_epgs.end())
    return false;

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

void CEpgContainer::UpdateProgressDialog(int iCurrent, int iMax, const CStdString &strText)
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
  lock.Leave();

  return bReturn ||
    (g_guiSettings.GetBool("epg.preventupdateswhileplayingtv") &&
     g_PVRManager.IsStarted() &&
     g_PVRManager.IsPlaying());
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
  }

  if (bShowProgress && !bOnlyPending)
    ShowProgressDialog();

  if (!m_bIgnoreDbForClient && !m_database.IsOpen())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - could not open the database", __FUNCTION__);

    CSingleLock lock(m_critSection);
    m_bIsUpdating = false;
    m_updateEvent.Set();

    if (bShowProgress && !bOnlyPending)
      CloseProgressDialog();

    return false;
  }

  /* load or update all EPG tables */
  CEpg *epg;
  unsigned int iCounter(0);
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
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

    if ((!bOnlyPending || epg->UpdatePending()) && epg->Update(start, end, m_iUpdateTime, bOnlyPending))
      ++iUpdatedTables;
  }

  if (bInterrupted)
  {
    /* the update has been interrupted. try again later */
    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    m_iNextEpgUpdate = iNow + g_advancedSettings.m_iEpgRetryInterruptedUpdateInterval;
  }
  else
  {
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
    m_iNextEpgUpdate += g_advancedSettings.m_iEpgUpdateCheckInterval;
    m_bHasPendingUpdates = false;
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
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
    it->second->Get(results);

  return results.Size() - iInitialSize;
}

const CDateTime CEpgContainer::GetFirstEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
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
  for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
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
    for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
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
  CSingleLock lock(m_critSection);

  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
  if (iNow >= m_iNextEpgActiveTagCheck)
  {
    bool bFoundChanges(false);
    CSingleLock lock(m_critSection);

    for (map<unsigned int, CEpg *>::iterator it = m_epgs.begin(); it != m_epgs.end(); it++)
      bFoundChanges = it->second->CheckPlayingEvent() || bFoundChanges;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgActiveTagCheck);
    m_iNextEpgActiveTagCheck += g_advancedSettings.m_iEpgActiveTagCheckInterval;

    if (bFoundChanges)
    {
      SetChanged();
      NotifyObservers(ObservableMessageEpgActiveItem);
    }

    /* pvr tags always start on the full minute */
    if (g_PVRManager.IsStarted())
      m_iNextEpgActiveTagCheck -= m_iNextEpgActiveTagCheck % 60;

    bReturn = true;
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
  m_bHasPendingUpdates = bHasPendingUpdates;
}
