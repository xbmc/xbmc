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
#include "SingleLock.h"

#include "PVREpgs.h"
#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "PVRManager.h"
#include "PVREpgSearchFilter.h"
#include "PVRChannel.h"
#include "PVRTimerInfoTag.h"

using namespace std;

CPVREpgs PVREpgs;

CPVREpgs::CPVREpgs()
{
  m_bDatabaseLoaded = false;
  m_bInihibitUpdate = false;
  m_RadioFirst      = CDateTime::GetCurrentDateTime();
  m_RadioLast       = m_RadioLast;
  m_TVFirst         = m_RadioLast;
  m_TVLast          = m_RadioLast;
}

CPVREpgs::~CPVREpgs()
{
  Clear();
}

void CPVREpgs::Clear()
{
  /* remove all pointers to epg tables on timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < PVRTimers.size(); iTimerPtr++)
    PVRTimers[iTimerPtr].SetEpgInfoTag(NULL);

  /* remove all EPG references */
  clear();
}

void CPVREpgs::Start()
{
  /* make sure the EPG is loaded before starting the thread */
  LoadFromDb(true /* show progress */);
  PVRTimers.Update();

  Create();
  SetName("XBMC EPG thread");
  SetPriority(-15);
  CLog::Log(LOGNOTICE, "%s - EPG thread started", __FUNCTION__);
}

void CPVREpgs::Stop()
{
  StopThread();
}

void CPVREpgs::Process()
{
  int iNow               = 0;
  int iLastPointerUpdate = 0;
  int iLastTimerUpdate   = 0;
  int iLastEpgUpdate     = 0;

  while (!m_bStop)
  {
    iNow = CTimeUtils::GetTimeMS()/1000;

    /* update the EPG */
    if (iNow > iLastEpgUpdate + 60)
    {
      if (UpdateEPG())
      {
        iLastEpgUpdate = iNow;
      }
    }

    /* update the "now playing" pointers */
    if (iNow > iLastPointerUpdate + 180)
    {
      iLastPointerUpdate = iNow;
      UpdateAllChannelEPGPointers();
    }

    /* update the timers */
    if (iNow > iLastTimerUpdate + 60)
    {
      iLastTimerUpdate = iNow;
      PVRTimers.Update();
    }

    Sleep(1000);
  }
}

bool CPVREpgs::LoadSettings()
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  m_iUpdateTime        = g_guiSettings.GetInt ("pvrepg.epgupdate")*60;
  m_iLingerTime        = g_guiSettings.GetInt ("pvrmenu.lingertime")*60;
  m_iDaysToDisplay     = g_guiSettings.GetInt ("pvrmenu.daystodisplay")*24*60*60;

  return true;
}

bool CPVREpgs::RemoveOldEntries()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "PVREpgs - %s - removing old EPG entries",
      __FUNCTION__);

  CDateTime now = CDateTime::GetCurrentDateTime();

  /* call Cleanup() on all known EPG tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    at(iEpgPtr)->Cleanup(now);
  }

  return true;
}

bool CPVREpgs::RemoveAllEntries(bool bShowProgress /* = false */)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "PVREpgs - %s - removing all EPG entries",
      __FUNCTION__);

  CGUIDialogProgress* pDlgProgress = NULL;

  if (bShowProgress)
  {
    pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDlgProgress->SetLine(0, "");
    pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal();
    pDlgProgress->Progress();
  }

  /* remove all the EPG pointers from timers */
  int iTimerSize = PVRTimers.size();
  for (int iTimerPtr = 0; iTimerPtr < iTimerSize; iTimerPtr++)
  {
    PVRTimers[iTimerPtr].SetEpgInfoTag(NULL);

    if (bShowProgress)
      pDlgProgress->SetPercentage(iTimerPtr / iTimerSize * 10);
  }

  /* remove all EPG entries */
  int iEpgSize = size();
  for (int iEpgPtr = 0; iEpgPtr < iEpgSize; iEpgPtr++)
  {
    ((CPVRChannel *)at(iEpgPtr)->Channel())->ClearEPG();

    if (bShowProgress)
      pDlgProgress->SetPercentage((iEpgPtr / iEpgSize * 90) + 10);
  }

  /* clear the database entries */
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();
  database->EraseEpg();
  database->Close();

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
  }

  return true;
}

bool CPVREpgs::LoadFromDb(bool bShowProgress /* = false */)
{
  if (m_bDatabaseLoaded)
    return m_bDatabaseLoaded;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();

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
  bool bLoaded = true;
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = radio ? &PVRChannelsRadio : &PVRChannelsTV;
    for (unsigned int channelPtr = 0; channelPtr < channels->size() && !m_bStop; channelPtr++)
    {
      CPVRChannel *channel = channels->at(channelPtr);
      CPVREpg *epg = channel->GetEPG();
      if (!epg)
      {
        CLog::Log(LOGERROR, "%s - cannot get EPG table for channel '%s'",
            __FUNCTION__, channel->ChannelName().c_str());
        continue;
      }

      if (epg->LoadFromDb())
        channel->UpdateEPGPointers();
      else
        bLoaded = false;

      if (bShowProgress)
      {
        /* update the progress bar */
        scanner->SetProgress(channelPtr, channels->size());
        scanner->SetTitle(epg->Channel()->ChannelName());
        scanner->UpdateState();
      }
    }
  }

  /* open the database */
  database->Close();

  if (bShowProgress)
    scanner->Close();

  /* set this to true in any situation or it'll keep trying to load the channels */
  m_bDatabaseLoaded = true;

  return bLoaded;
}

bool CPVREpgs::UpdateEPG(bool bShowProgress /* = false */)
{
  long iStartTime       = CTimeUtils::GetTimeMS();
  int iChannelCount     = PVRChannelsTV.size() + PVRChannelsRadio.size();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  bool bUpdateSuccess   = true;
  bool bUpdate          = false;

  CSingleLock lock(m_critSection);

  /* bail out if we're already updating */
  if (m_bInihibitUpdate)
    return false;
  else
    m_bInihibitUpdate = true;

  /* make sure we got the latest settings */
  LoadSettings();

  /* open the database */
  database->Open();

  RemoveOldEntries();

  /* determine if we're going to do a full update */
  if (m_iUpdateTime > 0)
  {
    time_t iLastUpdateTime;
    time_t iCurrentTime;
    database->GetLastEpgScanTime().GetAsTime(iLastUpdateTime);
    CDateTime::GetCurrentDateTime().GetAsTime(iCurrentTime);
    bUpdate = (iLastUpdateTime + m_iUpdateTime <= iCurrentTime);
  }

  if (!bUpdate)
  {
    m_bInihibitUpdate = false;
    return true;
  }

  CLog::Log(LOGNOTICE, "PVREpgs - %s - starting EPG update for %i channels (full update = %d, update time = %d)",
        __FUNCTION__, iChannelCount, bUpdate ? 1 : 0, m_iUpdateTime);

  /* show the progress bar */
  CGUIDialogPVRUpdateProgressBar *scanner = NULL;
  if (bShowProgress)
  {
    scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    scanner->SetHeader(g_localizeStrings.Get(19004));
  }

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsTime(start); // NOTE: XBMC stores the EPG times as local time
  CDateTime::GetCurrentDateTime().GetAsTime(end);
  start -= m_iLingerTime;
  end   += m_iDaysToDisplay;

  /* update all EPG tables */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = &PVRChannelsTV;
    for (unsigned int channelPtr = 0; channelPtr < channels->size() && !m_bStop; channelPtr++)
    {
      CPVRChannel *channel = channels->at(channelPtr);
      CPVREpg *epg = channel->GetEPG();
      if (!epg)
      {
        CLog::Log(LOGERROR, "%s - cannot get EPG table for channel '%s'",
            __FUNCTION__, channel->ChannelName().c_str());
        continue;
      }

      bUpdateSuccess = epg->Update(start, end, !m_bIgnoreDbForClient) && bUpdateSuccess;

      if (bShowProgress)
      {
        /* update the progress bar */
        scanner->SetProgress(channelPtr, channels->size());
        scanner->SetTitle(epg->Channel()->ChannelName());
        scanner->UpdateState();
      }
    }
  }

  /* update the last scan time if the update was succesful and if we did a full update */
  if (bUpdateSuccess && bUpdate)
    database->UpdateLastEpgScanTime();

  database->Close();

  if (bShowProgress)
    scanner->Close();
  m_bInihibitUpdate = false;

  long lUpdateTime = CTimeUtils::GetTimeMS() - iStartTime;
  CLog::Log(LOGINFO, "PVREpgs - %s - finished updating the EPG after %li.%li seconds",
      __FUNCTION__, lUpdateTime / 1000, lUpdateTime % 1000);

  return bUpdateSuccess;
}

void CPVREpgs::UpdateAllChannelEPGPointers()
{
  for (unsigned int epgPtr = 0; epgPtr < PVREpgs.size(); epgPtr++)
  {
    ((CPVRChannel *)PVREpgs.at(epgPtr)->Channel())->UpdateEPGPointers();
  }
}

int CPVREpgs::GetEPGAll(CFileItemList* results, bool bRadio /* = false */)
{
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;

  int iInitialSize = results->Size();

  /* include all channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = channels->GetByIndex(iChannelPtr);
    if (channel)
      GetEPGForChannel(channel, results);
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
  /* include radio and tv channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;

    for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
    {
      const CPVREpg *epg = channels->GetByIndex(iChannelPtr)->GetEPG();
      if (!epg->HasValidEntries() || epg->IsUpdateRunning())
        continue;

      for (unsigned int iTagPtr = 0; iTagPtr < epg->size(); iTagPtr++)
      {
        if (filter.FilterEntry(*epg->at(iTagPtr)))
        {
          CFileItemPtr channel(new CFileItem(*epg->at(iTagPtr)));
          results->Add(channel);
        }
      }
    }
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

int CPVREpgs::GetEPGForChannel(CPVRChannel *channel, CFileItemList *results)
{
  int iInitialSize   = results->Size();

  CPVREpg *epg = channel->GetEPG();
  if (!epg->HasValidEntries() || epg->IsUpdateRunning())
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG table",
        __FUNCTION__, channel->ChannelName().c_str());
    return 0;
  }

  for (unsigned int iTagPtr = 0; iTagPtr < epg->size(); iTagPtr++)
  {
    CFileItemPtr channelFile(new CFileItem(*epg->at(iTagPtr)));
    channelFile->SetLabel2(epg->at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
    results->Add(channelFile);
  }

  return results->Size() - iInitialSize;
}

int CPVREpgs::GetEPGNow(CFileItemList* results, bool bRadio)
{
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;
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
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;
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
