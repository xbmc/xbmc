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

#include "settings/GUISettings.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "log.h"
#include "TimeUtils.h"

#include "EpgContainer.h"
#include "Epg.h"
#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

#define EPGUPDATE                60  /* check if tables need to be updated every minute */
#define EPGCLEANUPINTERVAL       900 /* remove old entries from the EPG every 15 minutes */

using namespace std;

CEpgContainer g_EpgContainer;

CEpgContainer::CEpgContainer(void)
{
  m_bStop = true;
  Clear(false);
}

CEpgContainer::~CEpgContainer(void)
{
  Clear();
}

void CEpgContainer::Clear(bool bClearDb /* = false */)
{
  /* make sure the update thread is stopped */
  bool bThreadRunning = !m_bStop;
  if (bThreadRunning && !Stop())
    return;

  /* clear all epg tables and remove pointers to epg tables on channels */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CEpg *epg = at(iEpgPtr);
    epg->Clear();
  }

  /* remove all EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    delete at(iEpgPtr);
  }
  clear();

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
  m_First           = CDateTime::GetCurrentDateTime();
  m_Last            = m_First;

  if (bThreadRunning)
    Start();
}

void CEpgContainer::Start(void)
{
  m_bStop = false;
  LoadSettings();
  g_guiSettings.AddObserver(this);

  if (m_database.Open())
  {
    m_database.Get(this);
    m_database.Close();
  }

  AutoCreateTablesHook();

  UpdateEPG(true);

  Create();
  SetName("XBMC EPG thread");
  SetPriority(0);
  CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
}

bool CEpgContainer::Stop(void)
{
  StopThread();

  g_guiSettings.RemoveObserver(this);

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
  time_t iNow          = 0;
  m_iLastEpgCleanup    = 0;
  m_iLastEpgUpdate     = 0;

  while (!m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsTime(iNow);

    /* load or update the EPG */
    if (!m_bStop && (iNow > m_iLastEpgUpdate + EPGUPDATE || !m_bDatabaseLoaded))
      UpdateEPG(false);

    /* clean up old entries */
    if (!m_bStop && iNow > m_iLastEpgCleanup + EPGCLEANUPINTERVAL)
      RemoveOldEntries();

    /* call the update hook */
    ProcessHook(iNow);

    Sleep(1000);
  }
}

CEpg *CEpgContainer::GetById(int iEpgId) const
{
  CEpg *epg = NULL;

  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    if (at(iEpgPtr)->EpgID() == iEpgId)
    {
      epg = at(iEpgPtr);
      break;
    }
  }

  return epg;
}

bool CEpgContainer::UpdateEntry(const CEpg &entry, bool bUpdateDatabase /* = false */)
{
  bool bReturn = false;

  /* make sure the update thread is stopped */
  bool bThreadRunning = !m_bStop && m_bDatabaseLoaded;
  if (bThreadRunning && !Stop())
    return bReturn;

  CEpg *epg = GetById(entry.EpgID());

  if (!epg)
  {
    epg = CreateEpg(entry.EpgID());
    if (epg)
      push_back(epg);
  }

  bReturn = epg ? epg->Update(entry, bUpdateDatabase) : false;

  if (bThreadRunning)
    Start();

  return bReturn;
}

bool CEpgContainer::LoadSettings(void)
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("epg.ignoredbforclient");
  m_iUpdateTime        = g_guiSettings.GetInt ("epg.epgupdate") * 60;
  m_iLingerTime        = g_guiSettings.GetInt ("epg.lingertime") * 60;
  m_iDisplayTime       = g_guiSettings.GetInt ("epg.daystodisplay") * 24 * 60 * 60;

  return true;
}

bool CEpgContainer::RemoveOldEntries(void)
{
  bool bReturn = false;
  CLog::Log(LOGINFO, "EpgContainer - %s - removing old EPG entries",
      __FUNCTION__);

  CDateTime now = CDateTime::GetCurrentDateTime();

  if (!m_database.Open())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - cannot open the database",
        __FUNCTION__);
    return bReturn;
  }

  /* call Cleanup() on all known EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    at(iEpgPtr)->Cleanup(now);
  }

  /* remove the old entries from the database */
  bReturn = m_database.DeleteOldEpgEntries();

  if (bReturn)
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgCleanup);

  return bReturn;
}

CEpg *CEpgContainer::CreateEpg(int iEpgId)
{
  return new CEpg(iEpgId);
}

bool CEpgContainer::UpdateEPG(bool bShowProgress /* = false */)
{
  long iStartTime                         = CTimeUtils::GetTimeMS();
  unsigned int iEpgCount                  = size();
  bool bUpdateSuccess                     = true;
  CGUIDialogExtendedProgressBar *progress = NULL;

  if (!m_bDatabaseLoaded)
    CLog::Log(LOGNOTICE, "EpgContainer - %s - loading EPG entries for %i tables from the database",
        __FUNCTION__, iEpgCount);
  else
    CLog::Log(LOGNOTICE, "EpgContainer - %s - starting EPG update for %i tables (update time = %d)",
        __FUNCTION__, iEpgCount, m_iUpdateTime);

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsTime(start); // NOTE: XBMC stores the EPG times as local time
  end = start;
  start -= m_iLingerTime;
  end += m_iDisplayTime;

  /* open the database */
  if (!m_database.Open())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - could not open the database", __FUNCTION__);
    return false;
  }

  /* show the progress bar */
  if (bShowProgress)
  {
    progress = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    progress->Show();
    progress->SetHeader(g_localizeStrings.Get(19004));
  }

  /* load or update all EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < iEpgCount; iEpgPtr++)
  {
    /* interrupt the update on exit */
    if (m_bStop)
    {
      CLog::Log(LOGNOTICE, "EpgContainer - %s - EPG load/update interrupted", __FUNCTION__);
      bUpdateSuccess = false;
      break;
    }

    bool bCurrent = m_bDatabaseLoaded || m_bIgnoreDbForClient ?
        at(iEpgPtr)->Update(start, end, m_iUpdateTime, !m_bIgnoreDbForClient) :
        at(iEpgPtr)->Load() && bUpdateSuccess;

    if (!bCurrent)
      CLog::Log(LOGERROR, "EpgContainer - %s - failed to update table %d",
          __FUNCTION__, iEpgPtr);

    bUpdateSuccess = bCurrent && bUpdateSuccess;

    if (bShowProgress)
    {
      /* update the progress bar */
      progress->SetProgress(iEpgPtr, iEpgCount);
      progress->SetTitle(at(iEpgPtr)->Name());
      progress->UpdateState();
    }
  }

  /* update the last scan time if the update was successful and if we did a full update */
  if (bUpdateSuccess && (m_bDatabaseLoaded || m_bIgnoreDbForClient))
  {
    if (m_bIgnoreDbForClient)
      m_database.PersistLastEpgScanTime();
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgUpdate);
  }

  m_database.Close();

  if (bShowProgress)
    progress->Close();

  long lUpdateTime = CTimeUtils::GetTimeMS() - iStartTime;
  CLog::Log(LOGINFO, "EpgContainer - %s - finished %s %d EPG tables after %li.%li seconds",
      __FUNCTION__, m_bDatabaseLoaded ? "updating" : "loading", iEpgCount, lUpdateTime / 1000, lUpdateTime % 1000);

  /* only try to load the database once */
  m_bDatabaseLoaded = true;

  return bUpdateSuccess;
}

int CEpgContainer::GetEPGAll(CFileItemList* results)
{
  int iInitialSize = results->Size();

  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
    at(iEpgPtr)->Get(results);

  return results->Size() - iInitialSize;
}

void CEpgContainer::UpdateFirstAndLastEPGDates(const CEpgInfoTag &tag)
{
  if (tag.Start() < m_First)
    m_First = tag.Start();
  if (tag.End() > m_Last)
    m_Last = tag.End();
}

int CEpgContainer::GetEPGSearch(CFileItemList* results, const EpgSearchFilter &filter)
{
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
    at(iEpgPtr)->Get(results, filter);

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
  {
    unsigned int iSize = results->Size();
    for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
    {
      const CEpgInfoTag *epgentry_1 = results->Get(iResultPtr)->GetEPGInfoTag();
      for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
      {
        const CEpgInfoTag *epgentry_2 = results->Get(iTagPtr)->GetEPGInfoTag();
        if (iResultPtr == iTagPtr)
          continue;

        if (epgentry_1->Title()       != epgentry_2->Title() ||
            epgentry_1->Plot()        != epgentry_2->Plot() ||
            epgentry_1->PlotOutline() != epgentry_2->PlotOutline())
          continue;

        results->Remove(iTagPtr);
        iSize = results->Size();
        iResultPtr--;
        iTagPtr--;
      }
    }
  }

  return results->Size();
}
