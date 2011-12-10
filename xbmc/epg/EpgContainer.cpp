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
  m_progressDialog = NULL;
  m_bStop = true;
  m_bIsUpdating = false;
  m_bIsInitialising = true;
  m_iNextEpgId = 0;
  m_bPreventUpdates = false;
  m_updateEvent.Reset();
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

  if (g_PVRManager.IsStarted())
  {
    // XXX stop the timers from being updated while clearing tags
    /* remove all pointers to epg tables on timers */
    CPVRTimers *timers = g_PVRTimers;
    for (unsigned int iTimerPtr = 0; timers != NULL && iTimerPtr < timers->size(); iTimerPtr++)
      timers->at(iTimerPtr)->SetEpgInfoTag(NULL);
  }

  {
    CSingleLock lock(m_critSection);
    /* clear all epg tables and remove pointers to epg tables on channels */
    for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
      delete m_epgs[iEpgPtr];
    m_epgs.clear();
    m_iNextEpgUpdate  = 0;
    m_bIsInitialising = true;
  }

  /* clear the database entries */
  if (bClearDb && !m_bIgnoreDbForClient)
  {
    if (m_database.Open())
    {
      m_database.DeleteEpg();
      m_database.Close();
    }
  }

  SetChanged();
  NotifyObservers("epg", true);

  if (bThreadRunning)
    Start();
}

void CEpgContainer::Start(void)
{
  CSingleLock lock(m_critSection);

  m_bIsInitialising = true;
  m_bStop = false;
  g_guiSettings.RegisterObserver(this);
  LoadSettings();

  Create();
  SetPriority(-1);
  CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
}

bool CEpgContainer::Stop(void)
{
  StopThread();
  return true;
}

void CEpgContainer::Notify(const Observable &obs, const CStdString& msg)
{
  /* settings were updated */
  if (msg == "settings")
    LoadSettings();
}

void CEpgContainer::LoadFromDB(void)
{
  if (!m_bIgnoreDbForClient && m_database.Open())
  {
    m_database.DeleteOldEpgEntries();
    m_database.Get(*this);
    m_database.Close();
  }
}

void CEpgContainer::Process(void)
{
  bool bLoaded(false);
  time_t iNow       = 0;
  m_iNextEpgUpdate  = 0;
  m_iNextEpgActiveTagCheck = 0;
  CSingleLock lock(m_critSection);
  LoadFromDB();
  CheckPlayingEvents();
  lock.Leave();

  bool bUpdateEpg(true);
  while (!m_bStop && !g_application.m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    lock.Enter();
    bUpdateEpg = (iNow >= m_iNextEpgUpdate || !bLoaded);
    lock.Leave();

    /* load or update the EPG */
    if (!InterruptUpdate() && bUpdateEpg && UpdateEPG(m_bIsInitialising))
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && iNow >= m_iLastEpgCleanup)
      RemoveOldEntries();

    /* check for updated active tag */
    if (!m_bStop)
      CheckPlayingEvents();

    bLoaded = true;

    Sleep(1000);
  }

  g_guiSettings.UnregisterObserver(this);
}

CEpg *CEpgContainer::GetById(int iEpgId) const
{
  CEpg *epg = NULL;

  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    if (m_epgs[iEpgPtr]->EpgID() == iEpgId)
    {
      epg = m_epgs[iEpgPtr];
      break;
    }
  }

  return epg;
}

CEpg *CEpgContainer::GetByChannel(const CPVRChannel &channel) const
{
  CEpg *epg = NULL;

  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    const CPVRChannel *thisChannel = m_epgs[iEpgPtr]->Channel();
    if (thisChannel && *thisChannel == channel)
    {
      epg = m_epgs[iEpgPtr];
      break;
    }
  }

  return epg;
}

bool CEpgContainer::UpdateEntry(const CEpg &entry, bool bUpdateDatabase /* = false */)
{
  CEpg *epg(NULL);
  bool bReturn(false);
  WaitForUpdateFinish(true);

  CSingleLock lock(m_critSection);
  epg = entry.EpgID() > 0 ? GetById(entry.EpgID()) : NULL;
  if (!epg)
  {
    /* table does not exist yet, create a new one */
    unsigned int iEpgId = !m_bIgnoreDbForClient ? entry.EpgID() : NextEpgId();
    epg = CreateEpg(iEpgId);
    if (epg)
      InsertEpg(epg);
  }

  bReturn = epg ? epg->UpdateMetadata(entry, bUpdateDatabase) : false;
  m_bPreventUpdates = false;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
  lock.Leave();

  return bReturn;
}

void CEpgContainer::InsertEpg(CEpg *epg)
{
  WaitForUpdateFinish(true);

  CSingleLock lock(m_critSection);
  DeleteEpg(*epg, false);
  m_epgs.push_back(epg);
  m_bPreventUpdates = false;
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
  CLog::Log(LOGINFO, "EpgContainer - %s - removing old EPG entries",
      __FUNCTION__);

  CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();

  /* call Cleanup() on all known EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
    m_epgs[iEpgPtr]->Cleanup(now);

  /* remove the old entries from the database */
  if (!m_bIgnoreDbForClient)
  {
    if (m_database.Open())
    {
      m_database.DeleteOldEpgEntries();
      m_database.Close();
    }
  }

  CSingleLock lock(m_critSection);
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iLastEpgCleanup);
  m_iLastEpgCleanup += g_advancedSettings.m_iEpgCleanupInterval;

  return true;
}

CEpg *CEpgContainer::CreateEpg(int iEpgId)
{
  if (g_PVRManager.IsStarted())
  {
    CPVRChannel *channel = (CPVRChannel *) g_PVRChannelGroups->GetChannelByEpgId(iEpgId);
    if (channel)
    {
      CEpg *epg = new CEpg(channel, true);
      channel->Persist();
      return epg;
    }
  }

  return new CEpg(iEpgId);
}

bool CEpgContainer::DeleteEpg(const CEpg &epg, bool bDeleteFromDatabase /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    if (m_epgs[iEpgPtr]->EpgID() == epg.EpgID())
    {
      if (bDeleteFromDatabase && !m_bIgnoreDbForClient && m_database.Open())
      {
        m_database.Delete(*m_epgs[iEpgPtr]);
        m_database.Close();
      }

      delete m_epgs[iEpgPtr];
      m_epgs.erase(m_epgs.begin() + iEpgPtr);
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

void CEpgContainer::CloseProgressDialog(void)
{
  if (m_progressDialog)
  {
    m_progressDialog->Close(true, 0, true, false);
    m_progressDialog = NULL;
  }
}

void CEpgContainer::ShowProgressDialog(void)
{
  if (!m_progressDialog && !g_PVRManager.IsInitialising())
  {
    m_progressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    m_progressDialog->Show();
    m_progressDialog->SetHeader(g_localizeStrings.Get(19004));
  }
}

void CEpgContainer::UpdateProgressDialog(int iCurrent, int iMax, const CStdString &strText)
{
  if (!m_progressDialog)
    ShowProgressDialog();

  if (m_progressDialog)
  {
    m_progressDialog->SetProgress(iCurrent, iMax);
    m_progressDialog->SetTitle(strText);
    m_progressDialog->UpdateState();
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

bool CEpgContainer::UpdateEPG(bool bShowProgress /* = false */)
{
  bool bInterrupted(false);
  unsigned int iUpdatedTables(0);

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(start);
  end = start + m_iDisplayTime;
  start -= g_advancedSettings.m_iEpgLingerTime * 60;

  CSingleLock lock(m_critSection);
  if (m_bIsUpdating || InterruptUpdate())
      return false;
  m_bIsUpdating = true;
  lock.Leave();

  if (bShowProgress)
    ShowProgressDialog();

  /* open the database */
  if (!m_bIgnoreDbForClient && !m_database.Open())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - could not open the database", __FUNCTION__);
    return false;
  }

  /* load or update all EPG tables */
  CEpg *epg;
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    epg = m_epgs[iEpgPtr];
    if (!epg)
      continue;

    if (epg->Update(start, end, m_iUpdateTime))
      ++iUpdatedTables;

    if (bShowProgress)
      UpdateProgressDialog(iEpgPtr, m_epgs.size(), epg->Name());
  }

  if (!bInterrupted)
  {
    lock.Enter();
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
    m_iNextEpgUpdate += g_advancedSettings.m_iEpgUpdateCheckInterval;
    lock.Leave();
  }

  if (!m_bIgnoreDbForClient)
    m_database.Close();

  lock.Enter();
  m_bIsUpdating = false;
  if (bInterrupted)
  {
    /* the update has been interrupted. try again later */
    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    m_iNextEpgUpdate = iNow + g_advancedSettings.m_iEpgRetryInterruptedUpdateInterval;
  }
  lock.Leave();

  if (bShowProgress)
    CloseProgressDialog();

  /* notify observers */
  if (iUpdatedTables > 0)
  {
    CheckPlayingEvents();
    SetChanged();
    NotifyObservers("epg", true);
  }

  m_updateEvent.Set();

  return !bInterrupted;
}

int CEpgContainer::GetEPGAll(CFileItemList &results)
{
  int iInitialSize = results.Size();

  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
    m_epgs[iEpgPtr]->Get(results);

  return results.Size() - iInitialSize;
}

const CDateTime CEpgContainer::GetFirstEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    CDateTime entry = m_epgs[iEpgPtr]->GetFirstDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry < returnValue))
      returnValue = entry;
  }

  return returnValue;
}

const CDateTime CEpgContainer::GetLastEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
  {
    CDateTime entry = m_epgs[iEpgPtr]->GetLastDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry > returnValue))
      returnValue = entry;
  }

  return returnValue;
}

int CEpgContainer::GetEPGSearch(CFileItemList &results, const EpgSearchFilter &filter)
{
  int iInitialSize = results.Size();

  /* get filtered results from all tables */
  CSingleLock lock(m_critSection);
  for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
    m_epgs[iEpgPtr]->Get(results, filter);
  lock.Leave();

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

    for (unsigned int iEpgPtr = 0; iEpgPtr < m_epgs.size(); iEpgPtr++)
      bFoundChanges = m_epgs[iEpgPtr]->CheckPlayingEvent() || bFoundChanges;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgActiveTagCheck);
    m_iNextEpgActiveTagCheck += g_advancedSettings.m_iEpgActiveTagCheckInterval;

    if (bFoundChanges)
    {
      SetChanged();
      NotifyObservers("epg-now", true);
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
