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

#include "threads/SingleLock.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

#include "EpgContainer.h"
#include "Epg.h"
#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

using namespace std;
using namespace EPG;

typedef std::map<int, CEpg*>::iterator EPGITR;

CEpgContainer::CEpgContainer(void) :
    CThread("EPG updater")
{
  m_progressDialog = NULL;
  m_bStop = true;
  m_bIsUpdating = false;
  m_iNextEpgId = 0;
  Clear(false);
}

CEpgContainer::~CEpgContainer(void)
{
  Clear();
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
  CSingleLock lock(m_critSection);

  /* make sure the update thread is stopped */
  bool bThreadRunning = !m_bStop;
  if (bThreadRunning && !Stop())
  {
    CLog::Log(LOGERROR, "%s - cannot stop the update thread", __FUNCTION__);
    return;
  }

  /* clear all epg tables and remove pointers to epg tables on channels */
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
    delete m_epgs[(*itr).first];
  m_epgs.clear();

  /* clear the database entries */
  if (bClearDb)
  {
    if (m_database.Open())
    {
      m_database.DeleteEpg();
      m_database.Close();
    }
  }

  m_iLastEpgUpdate  = 0;
  m_bDatabaseLoaded = false;

  lock.Leave();

  if (bThreadRunning)
    Start();
}

void CEpgContainer::Start(void)
{
  CSingleLock lock(m_critSection);

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

  g_guiSettings.UnregisterObserver(this);

  return true;
}

void CEpgContainer::Notify(const Observable &obs, const CStdString& msg)
{
  /* settings were updated */
  if (msg == "settings")
    LoadSettings();
}

void CEpgContainer::Process(void)
{
  time_t iNow       = 0;
  m_iLastEpgUpdate  = 0;
  m_iLastEpgActiveTagCheck = 0;
  CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgCleanup);

  if (m_database.Open())
  {
    m_database.DeleteOldEpgEntries();
    m_database.Get(*this);
    m_database.Close();
  }

  AutoCreateTablesHook();

  while (!m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsTime(iNow);

    /* load or update the EPG */
    if (!m_bStop && !InterruptUpdate() &&
        (iNow > m_iLastEpgUpdate + g_advancedSettings.m_iEpgUpdateCheckInterval || !m_bDatabaseLoaded))
    {
      if (!UpdateEPG(!m_bDatabaseLoaded))
      {
        /* the update has been interrupted. try again later */
        CDateTime::GetCurrentDateTime().GetAsTime(iNow);
        m_iLastEpgUpdate = iNow - g_advancedSettings.m_iEpgUpdateCheckInterval - g_advancedSettings.m_iEpgRetryInterruptedUpdateInterval;
      }
    }

    /* clean up old entries */
    if (!m_bStop && iNow > m_iLastEpgCleanup + g_advancedSettings.m_iEpgCleanupInterval)
      RemoveOldEntries();

    /* check for updated active tag */
    if (!m_bStop)
      CheckPlayingEvents();

    /* call the update hook */
    ProcessHook(iNow);

    Sleep(1000);
  }
}

CEpg *CEpgContainer::GetById(int iEpgId)
{
  CEpg *epg = NULL;

  CSingleLock lock(m_critSection);
  EPGITR itr = m_epgs.find(iEpgId);
  if (itr != m_epgs.end())
    epg = m_epgs[(*itr).first];

  return epg;
}

bool CEpgContainer::UpdateEntry(const CEpg &entry, bool bUpdateDatabase /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  /* make sure the update thread is stopped */
  bool bThreadRunning = !m_bStop && m_bDatabaseLoaded;
  if (bThreadRunning && !Stop())
    return bReturn;

  CEpg *epg = GetById(entry.EpgID());

  if (!epg)
  {
    epg = CreateEpg(entry.EpgID());
    if (epg)
      m_epgs.insert(std::make_pair(entry.EpgID(), epg));
  }

  bReturn = epg ? epg->Update(entry, bUpdateDatabase) : false;

  lock.Leave();

  if (bThreadRunning)
    Start();

  return bReturn;
}

void CEpgContainer::InsertEpg(CEpg *epg)
{
  CSingleLock lock(m_critSection);

  EPGITR itr = m_epgs.find(epg->EpgID());
  if (itr != m_epgs.end())
  {
    delete m_epgs[(*itr).first];
    m_epgs.erase((*itr).first);
  }

  m_epgs.insert(std::make_pair(epg->EpgID(), epg));
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
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
    m_epgs[(*itr).first]->Cleanup(now);

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
  CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgCleanup);

  return true;
}

CEpg *CEpgContainer::CreateEpg(int iEpgId)
{
  return new CEpg(iEpgId);
}

bool CEpgContainer::DeleteEpg(const CEpg &epg, bool bDeleteFromDatabase /* = false */)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  EPGITR itr = m_epgs.find(epg.EpgID());
  if (itr != m_epgs.end())
  {
    if (bDeleteFromDatabase && !m_bIgnoreDbForClient && m_database.Open())
    {
      m_database.Delete(*m_epgs[(*itr).first]);
      m_database.Close();
    }

    delete m_epgs[(*itr).first];
    m_epgs.erase((*itr).first);
    bReturn = true;
  }

  return bReturn;
}

void CEpgContainer::CloseProgressDialog(void)
{
  CSingleLock lock(m_critSection);
  if (m_progressDialog)
  {
    m_progressDialog->Close(true);
    m_progressDialog = NULL;
  }
}

void CEpgContainer::ShowProgressDialog(void)
{
  CSingleLock lock(m_critSection);
  if (!m_progressDialog)
  {
    m_progressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    m_progressDialog->Show();
    m_progressDialog->SetHeader(g_localizeStrings.Get(19004));
  }
}

void CEpgContainer::UpdateProgressDialog(int iCurrent, int iMax, const CStdString &strText)
{
  CSingleLock lock(m_critSection);
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
  CSingleLock lock(m_critSection);
  return m_bStop;
}

bool CEpgContainer::UpdateEPG(bool bShowProgress /* = false */)
{
  long iStartTime(XbmcThreads::SystemClockMillis());
  bool bInterrupted(false);
  unsigned int iUpdatedTables(0);

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(start);
  end = start;
  start -= g_advancedSettings.m_iEpgLingerTime * 60;
  end += m_iDisplayTime;

  CSingleLock lock(m_critSection);
  unsigned int iEpgCount(m_epgs.size());
  m_bIsUpdating = true;
  lock.Leave();

  CLog::Log(LOGNOTICE, "EpgContainer - %s - %s EPG for %i tables", __FUNCTION__, m_bDatabaseLoaded ? "updating" : "loading", iEpgCount);
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
  int iEpgPtr(0);
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
  {
    ++iEpgPtr;
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    epg = m_epgs[(*itr).first];
    if (!epg)
      continue;

    if (epg->Update(start, end, m_iUpdateTime, !m_bDatabaseLoaded && !m_bIgnoreDbForClient))
      ++iUpdatedTables;

    if (bShowProgress)
      UpdateProgressDialog(iEpgPtr, iEpgCount, epg->Name());
  }

  if (!bInterrupted)
  {
    lock.Enter();
    /* only try to load the database once */
    m_bDatabaseLoaded = true;
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgUpdate);
    lock.Leave();

    /* update the last scan time if we did a full update */
    if (m_bDatabaseLoaded && !m_bIgnoreDbForClient)
      m_database.PersistLastEpgScanTime(0);
  }

  if (!m_bIgnoreDbForClient)
    m_database.Close();

  lock.Enter();
  m_bIsUpdating = false;
  lock.Leave();

  CloseProgressDialog();
  long lUpdateTime = XbmcThreads::SystemClockMillis() - iStartTime;
  CLog::Log(LOGINFO, "EpgContainer - %s - finished %s %d EPG tables after %li.%li seconds",
      __FUNCTION__, m_bDatabaseLoaded ? "updating" : "loading", iEpgCount, lUpdateTime / 1000, lUpdateTime % 1000);

  /* notify observers */
  if (iUpdatedTables > 0)
  {
    CheckPlayingEvents();
    SetChanged();
    NotifyObservers("epg", true);
  }

  return !bInterrupted;
}

int CEpgContainer::GetEPGAll(CFileItemList* results)
{
  int iInitialSize = results->Size();

  CSingleLock lock(m_critSection);
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
    m_epgs[(*itr).first]->Get(results);

  return results->Size() - iInitialSize;
}

const CDateTime CEpgContainer::GetFirstEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
  {
    CDateTime entry = m_epgs[(*itr).first]->GetFirstDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry < returnValue))
      returnValue = entry;
  }

  return returnValue;
}

const CDateTime CEpgContainer::GetLastEPGDate(void)
{
  CDateTime returnValue;

  CSingleLock lock(m_critSection);
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
  {
    CDateTime entry = m_epgs[(*itr).first]->GetLastDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry > returnValue))
      returnValue = entry;
  }

  return returnValue;
}

int CEpgContainer::GetEPGSearch(CFileItemList* results, const EpgSearchFilter &filter)
{
  /* get filtered results from all tables */
  CSingleLock lock(m_critSection);
  for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
    m_epgs[(*itr).first]->Get(results, filter);
  lock.Leave();

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
    EpgSearchFilter::RemoveDuplicates(results);

  return results->Size();
}

bool CEpgContainer::CheckPlayingEvents(void)
{
  bool bReturn(false);
  time_t iNow;
  CSingleLock lock(m_critSection);

  CDateTime::GetCurrentDateTime().GetAsTime(iNow);
  if (iNow >= m_iLastEpgActiveTagCheck + g_advancedSettings.m_iEpgActiveTagCheckInterval)
  {
    bool bFoundChanges(false);
    CSingleLock lock(m_critSection);

    for (EPGITR itr = m_epgs.begin(); itr != m_epgs.end(); itr++)
      bFoundChanges = m_epgs[(*itr).first]->CheckPlayingEvent() || bFoundChanges;
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgActiveTagCheck);

    if (bFoundChanges)
    {
      SetChanged();
      NotifyObservers("epg-now", true);
    }

    bReturn = true;
  }

  return bReturn;
}
