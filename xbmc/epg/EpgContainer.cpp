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
#include "pvr/dialogs/GUIDialogPVRUpdateProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "log.h"
#include "TimeUtils.h"

#include "EpgContainer.h"
#include "Epg.h"
#include "EpgInfoTag.h"
#include "EpgSearchFilter.h"

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
  /* make sure the EPG is loaded before starting the thread */
  Load(true /* show progress */);

  g_guiSettings.AddObserver(this);

  Create();
  SetName("XBMC EPG thread");
  SetPriority(-15);
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
  bool bFirstUpdate    = true;
  time_t iNow          = 0;
  m_iLastEpgCleanup    = 0;
  m_iLastEpgUpdate     = 0;

  if (!m_database.Open())
  {
    CLog::Log(LOGERROR, "EpgContainer - %s - cannot open the database",
        __FUNCTION__);
    return;
  }

  /* get the last EPG update time from the database */
  m_database.GetLastEpgScanTime().GetAsTime(m_iLastEpgUpdate);
  m_database.Close();

  LoadSettings();

  while (!m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsTime(iNow);

    /* update the EPG */
    if (!m_bStop && (iNow > m_iLastEpgUpdate + m_iUpdateTime || !m_bDatabaseLoaded))
    {
      UpdateEPG(bFirstUpdate);
      if (bFirstUpdate)
        bFirstUpdate = false;
    }

    /* clean up old entries */
    if (!m_bStop && iNow > m_iLastEpgCleanup + EPGCLEANUPINTERVAL)
      RemoveOldEntries();

    /* call the update hook */
    ProcessHook(iNow);

    Sleep(1000);
  }
}

CEpg *CEpgContainer::GetById(int iEpgId)
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
  bool bThreadRunning = !m_bStop;
  if (bThreadRunning && !Stop())
    return bReturn;

  CEpg *epg = GetById(entry.EpgID());

  if (!epg)
  {
    epg = new CEpg(entry.EpgID());
    push_back(epg);
  }

  bReturn = epg->Update(entry, bUpdateDatabase);

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

bool CEpgContainer::Load(bool bShowProgress /* = false */)
{
  if (m_bDatabaseLoaded)
    return m_bDatabaseLoaded;

  bool bReturn = false;

  /* show the progress bar */
  CGUIDialogPVRUpdateProgressBar *scanner = NULL;
  if (bShowProgress)
  {
    scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    scanner->SetHeader(g_localizeStrings.Get(19004));
  }

  /* open the database */
  if (!m_database.Open())
  {
    CLog::Log(LOGERROR, "%s - failed to open the database", __FUNCTION__);
    return bReturn;
  }

  /* load all EPG tables */
  m_database.Get(this);

  /* load all entries in the EPG tables */
  unsigned int iSize = size();
  for (unsigned int iEpgPtr = 0; iEpgPtr < iSize; iEpgPtr++)
  {
    CEpg *epg = at(iEpgPtr);
    bReturn = epg->Load() || bReturn;

    if (bShowProgress)
    {
      /* update the progress bar */
      scanner->SetProgress(iEpgPtr, iSize);
      scanner->SetTitle(epg->Name());
      scanner->UpdateState();
    }
  }

  /* close the database */
  m_database.Close();

  if (bShowProgress)
    scanner->Close();

  /* only try to load the database once */
  m_bDatabaseLoaded = true;

  return bReturn;
}

bool CEpgContainer::UpdateEPG(bool bShowProgress /* = false */)
{
  long iStartTime                         = CTimeUtils::GetTimeMS();
  int iEpgCount                           = size();
  bool bUpdateSuccess                     = true;
  CGUIDialogPVRUpdateProgressBar *scanner = NULL;

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsTime(start); // NOTE: XBMC stores the EPG times as local time
  end = start;
  start -= m_iLingerTime;

  if (!m_bDatabaseLoaded)
  {
    CLog::Log(LOGNOTICE, "EpgContainer - %s - loading initial EPG entries for %i tables from clients",
        __FUNCTION__, iEpgCount);
    end += 60 * 60 * 3; // load 3 hours
  }
  else
  {
    CLog::Log(LOGNOTICE, "EpgContainer - %s - starting EPG update for %i tables (update time = %d)",
        __FUNCTION__, iEpgCount, m_iUpdateTime);
    end += m_iDisplayTime;
  }

  /* open the database */
  m_database.Open();

  /* show the progress bar */
  if (bShowProgress)
  {
    scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    scanner->SetHeader(g_localizeStrings.Get(19004));
  }

  /* update all EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    /* interrupt the update on exit */
    if (m_bStop)
    {
      bUpdateSuccess = false;
      break;
    }

    bUpdateSuccess = at(iEpgPtr)->Update(start, end, !m_bIgnoreDbForClient) && bUpdateSuccess;

    if (bShowProgress)
    {
      /* update the progress bar */
      scanner->SetProgress(iEpgPtr, iEpgCount);
      scanner->SetTitle(at(iEpgPtr)->Name());
      scanner->UpdateState();
    }
  }

  /* update the last scan time if the update was successful and if we did a full update */
  if (bUpdateSuccess && m_bDatabaseLoaded)
  {
    m_database.PersistLastEpgScanTime();
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgUpdate);
  }

  /* only try to load the database once */
  if (!m_bDatabaseLoaded)
    m_bDatabaseLoaded = true;

  m_database.Close();

  if (bShowProgress)
    scanner->Close();

  long lUpdateTime = CTimeUtils::GetTimeMS() - iStartTime;
  CLog::Log(LOGINFO, "EpgContainer - %s - finished updating the EPG after %li.%li seconds",
      __FUNCTION__, lUpdateTime / 1000, lUpdateTime % 1000);

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
