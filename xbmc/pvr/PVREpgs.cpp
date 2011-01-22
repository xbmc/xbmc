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

#include "GUISettings.h"
#include "GUIDialogPVRUpdateProgressBar.h"
#include "GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "TimeUtils.h"

#include "PVREpgs.h"
#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "PVRManager.h"
#include "PVREpgSearchFilter.h"
#include "PVRChannelGroupsContainer.h"
#include "PVRTimerInfoTag.h"

#define NOWPLAYINGUPDATEINTERVAL 30  /* update "now playing" tags every 30 seconds */
#define EPGCLEANUPINTERVAL       900 /* remove old entries from the EPG every 15 minutes */

using namespace std;

CPVREpgs PVREpgs;

CPVREpgs::CPVREpgs()
{
  m_bStop = true;
  Reset();
}

CPVREpgs::~CPVREpgs()
{
  Clear();
}

void CPVREpgs::Clear(bool bClearDb /* = false */)
{
  // XXX stop the timers from being updated while clearing tags
  /* remove all pointers to epg tables on timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < PVRTimers.size(); iTimerPtr++)
    PVRTimers[iTimerPtr].SetEpgInfoTag(NULL);

  /* clear all epg tables and remove pointers to epg tables on channels */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);
    epg->Clear();

    CPVRChannel *channel = (CPVRChannel *) epg->Channel();
    if (channel)
      channel->m_EPG = NULL;
  }

  /* remove all EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    delete at(iEpgPtr);
  }
  erase(begin(), end());

  /* clear the database entries */
  if (bClearDb)
  {
    CPVRDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    database->EraseEpg();
    database->Close();
  }

  m_iLastEpgUpdate  = 0;
  m_bDatabaseLoaded = false;
}

void CPVREpgs::Start()
{
  Clear();

  g_guiSettings.AddObserver(this);

  /* make sure the EPG is loaded before starting the thread */
  CreateChannelEpgs();
  LoadFromDb(true /* show progress */);

  Create();
  SetName("XBMC EPG thread");
  SetPriority(-15);
  CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
}

bool CPVREpgs::Stop()
{
  StopThread();

  g_guiSettings.RemoveObserver(this);

  return true;
}

bool CPVREpgs::Reset(bool bClearDb /* = false */)
{
  bool bThreadRunning = !m_bStop;

  if (bThreadRunning && !Stop())
    return false;

  Clear(bClearDb);

  m_bDatabaseLoaded = false;
  m_RadioFirst      = CDateTime::GetCurrentDateTime();
  m_RadioLast       = m_RadioFirst;
  m_TVFirst         = m_RadioFirst;
  m_TVLast          = m_RadioFirst;

  if (bThreadRunning)
    Start();

  return true;
}

void CPVREpgs::Notify(const Observable &obs, const CStdString& msg)
{
  /* settings were updated */
  if (msg == "settings")
    LoadSettings();
}

void CPVREpgs::Process()
{
  time_t iNow          = 0;
  m_iLastPointerUpdate = 0;
  m_iLastEpgCleanup    = 0;
  m_iLastEpgUpdate     = 0;

  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  if (!database->Open())
  {
    CLog::Log(LOGERROR, "PVREpgs - %s - cannot open the database",
        __FUNCTION__);
    return;
  }

  /* get the last EPG update time from the database */
  database->GetLastEpgScanTime().GetAsTime(m_iLastEpgUpdate);
  database->Close();

  LoadSettings();

  while (!m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsTime(iNow);

    /* update the EPG */
    if (!m_bStop && (iNow > m_iLastEpgUpdate + m_iUpdateTime || !m_bDatabaseLoaded))
      UpdateEPG(!m_bDatabaseLoaded);

    if (!m_bStop && iNow > m_iLastPointerUpdate + NOWPLAYINGUPDATEINTERVAL)
      UpdateAllChannelEPGPointers();

    if (!m_bStop && iNow > m_iLastEpgCleanup + EPGCLEANUPINTERVAL)
      RemoveOldEntries();

    Sleep(1000);
  }
}

bool CPVREpgs::LoadSettings()
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  m_iUpdateTime        = g_guiSettings.GetInt ("pvrepg.epgupdate") * 60;
  m_iLingerTime        = g_guiSettings.GetInt ("pvrmenu.lingertime") * 60;
  m_iDisplayTime       = g_guiSettings.GetInt ("pvrmenu.daystodisplay") * 24 * 60 * 60;

  return true;
}

bool CPVREpgs::RemoveOldEntries()
{
  bool bReturn = false;
  CLog::Log(LOGINFO, "PVREpgs - %s - removing old EPG entries",
      __FUNCTION__);

  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (!database->Open())
  {
    CLog::Log(LOGERROR, "PVREpgs - %s - cannot open the database",
        __FUNCTION__);
    return bReturn;
  }

  /* call Cleanup() on all known EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    at(iEpgPtr)->Cleanup(now);
  }

  /* remove the old entries from the database */
  bReturn = database->EraseOldEpgEntries();

  if (bReturn)
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgCleanup);

  return bReturn;
}

bool CPVREpgs::CreateChannelEpgs(void)
{
  for (int radio = 0; radio <= 1; radio++)
  {
    const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(radio == 1);
    for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
    {
      channels->at(iChannelPtr)->GetEPG();
    }
  }

  return true;
}

bool CPVREpgs::LoadFromDb(bool bShowProgress /* = false */)
{
  if (m_bDatabaseLoaded)
    return m_bDatabaseLoaded;

  CPVRDatabase *database = g_PVRManager.GetTVDatabase();

  /* show the progress bar */
  CGUIDialogPVRUpdateProgressBar *scanner = NULL;
  if (bShowProgress)
  {
    scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    scanner->SetHeader(g_localizeStrings.Get(19004));
  }

  /* open the database */
  database->Open();

  /* load all EPG tables */
  bool bLoaded = false;
  unsigned int iSize = size();
  for (unsigned int iEpgPtr = 0; iEpgPtr < iSize; iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);
    CPVRChannel *channel = epg->Channel();

    if (epg->LoadFromDb())
    {
      if (channel)
        channel->UpdateEPGPointers();

      bLoaded = true;
    }

    if (bShowProgress)
    {
      /* update the progress bar */
      if (channel)
        scanner->SetTitle(channel->ChannelName());

      scanner->SetProgress(iEpgPtr, iSize);
      scanner->UpdateState();
    }
  }

  /* close the database */
  database->Close();

  if (bShowProgress)
    scanner->Close();

  m_bDatabaseLoaded = bLoaded;

  return bLoaded;
}

bool CPVREpgs::UpdateEPG(bool bShowProgress /* = false */)
{
  long iStartTime                         = CTimeUtils::GetTimeMS();
  int iEpgCount                           = size();
  CPVRDatabase *database                   = g_PVRManager.GetTVDatabase();
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
    CLog::Log(LOGNOTICE, "PVREpgs - %s - loading initial EPG entries for %i tables from clients",
        __FUNCTION__, iEpgCount);
    end += 60 * 60 * 3; // load 3 hours
  }
  else
  {
    CLog::Log(LOGNOTICE, "PVREpgs - %s - starting EPG update for %i tables (update time = %d)",
        __FUNCTION__, iEpgCount, m_iUpdateTime);
    end += m_iDisplayTime;
  }

  /* show the progress bar */
  if (bShowProgress)
  {
    scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    scanner->SetHeader(g_localizeStrings.Get(19004));
  }

  /* open the database */
  database->Open();

  /* update all EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    /* interrupt the update on exit */
    if (m_bStop)
    {
      bUpdateSuccess = false;
      break;
    }

    CPVREpg *epg = at(iEpgPtr);

    bUpdateSuccess = epg->Update(start, end, !m_bIgnoreDbForClient) && bUpdateSuccess;

    if (bShowProgress)
    {
      /* update the progress bar */
      scanner->SetProgress(iEpgPtr, iEpgCount);
      scanner->SetTitle(epg->Channel()->ChannelName());
      scanner->UpdateState();
    }
  }

  /* update the last scan time if the update was successful and if we did a full update */
  if (bUpdateSuccess && m_bDatabaseLoaded)
  {
    database->UpdateLastEpgScanTime();
    CDateTime::GetCurrentDateTime().GetAsTime(m_iLastEpgUpdate);
  }
  database->Close();

  if (!m_bDatabaseLoaded)
  {
    UpdateAllChannelEPGPointers();
    m_bDatabaseLoaded = true;
  }

  if (bShowProgress)
    scanner->Close();

  long lUpdateTime = CTimeUtils::GetTimeMS() - iStartTime;
  CLog::Log(LOGINFO, "PVREpgs - %s - finished updating the EPG after %li.%li seconds",
      __FUNCTION__, lUpdateTime / 1000, lUpdateTime % 1000);

  return bUpdateSuccess;
}

bool CPVREpgs::UpdateAllChannelEPGPointers()
{
  for (unsigned int epgPtr = 0; epgPtr < PVREpgs.size(); epgPtr++)
  {
    CPVRChannel *channel = PVREpgs.at(epgPtr)->Channel();
    if (channel)
      channel->UpdateEPGPointers();
  }

  CDateTime::GetCurrentDateTime().GetAsTime(m_iLastPointerUpdate);

  return true;
}

int CPVREpgs::GetEPGAll(CFileItemList* results, bool bRadio /* = false */)
{
  int iInitialSize = results->Size();

  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);
    CPVRChannel *channel = at(iEpgPtr)->Channel();
    if (!channel || channel->IsRadio() != bRadio)
      continue;

    epg->Get(results);
  }

  return results->Size() - iInitialSize;
}

void CPVREpgs::UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag)
{
  if (!tag.ChannelTag())
    return;

  if (tag.ChannelTag()->IsRadio())
  {
    if (tag.Start() < m_RadioFirst)
      m_RadioFirst = tag.Start();

    if (tag.End() > m_RadioLast)
      m_RadioLast = tag.End();
  }
  else
  {
    if (tag.Start() < m_TVFirst)
      m_TVFirst = tag.Start();

    if (tag.End() > m_TVLast)
      m_TVLast = tag.End();
  }
}

CDateTime CPVREpgs::GetFirstEPGDate(bool bRadio /* = false */)
{
  return bRadio ? m_RadioFirst : m_TVFirst;
}

CDateTime CPVREpgs::GetLastEPGDate(bool bRadio /* = false */)
{
  return bRadio ? m_RadioLast : m_TVLast;
}

int CPVREpgs::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);
    epg->Get(results, filter);
  }

  /* filter recordings */
  if (filter.m_bIgnorePresentRecordings && PVRRecordings.size() > 0)
  {
    for (unsigned int iRecordingPtr = 0; iRecordingPtr < PVRRecordings.size(); iRecordingPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry  = results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRRecordingInfoTag *recording = &PVRRecordings[iRecordingPtr];
        if (epgentry)
        {
          if (epgentry->Title()       != recording->Title() ||
              epgentry->PlotOutline() != recording->PlotOutline() ||
              epgentry->Plot()        != recording->Plot())
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  /* filter timers */
  if (filter.m_bIgnorePresentTimers && PVRTimers.size() > 0)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < PVRTimers.size(); iTimerPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry = results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRTimerInfoTag *timer        = &PVRTimers[iTimerPtr];
        if (epgentry)
        {
          if (epgentry->ChannelTag()->ChannelNumber() != timer->ChannelNumber() ||
              epgentry->Start()                       <  timer->Start() ||
              epgentry->End()                         >  timer->Stop())
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
  {
    unsigned int iSize = results->Size();
    for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
    {
      const CPVREpgInfoTag *epgentry_1 = results->Get(iResultPtr)->GetEPGInfoTag();
      for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
      {
        const CPVREpgInfoTag *epgentry_2 = results->Get(iTagPtr)->GetEPGInfoTag();
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

int CPVREpgs::GetEPGNow(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = (CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(bRadio);
  int iInitialSize       = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries() || epg->IsUpdateRunning())
      continue;

    const CPVREpgInfoTag *epgNow = epg->InfoTagNow();
    if (!epgNow)
    {
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNow));
    entry->SetLabel2(epgNow->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}

int CPVREpgs::GetEPGNext(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = (CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(bRadio);
  int iInitialSize       = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries() || epg->IsUpdateRunning())
      continue;

    const CPVREpgInfoTag *epgNext = epg->InfoTagNext();
    if (!epgNext)
    {
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNext));
    entry->SetLabel2(epgNext->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}
