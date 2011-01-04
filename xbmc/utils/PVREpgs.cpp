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

using namespace std;

CPVREpgs PVREpgs;

CPVREpgs::CPVREpgs()
{
  m_bDatabaseLoaded = false;
  m_bInihibitUpdate = false;
}

CPVREpgs::~CPVREpgs()
{
  Unload();
}

const CPVREpg *CPVREpgs::CreateEPG(CPVRChannel *channel)
{
  const CPVREpg *epg = new CPVREpg(channel);
  push_back((CPVREpg *) epg);
  channel->m_Epg = epg;

  return channel->m_Epg;
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

bool CPVREpgs::RemoveAllEntries()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "PVREpgs - %s - removing all EPG entries",
      __FUNCTION__);

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->Progress();

  /* remove all the EPG pointers from timers */
  int iTimerSize = PVRTimers.size();
  for (int iTimerPtr = 0; iTimerPtr < iTimerSize; iTimerPtr++)
  {
    PVRTimers[iTimerPtr].SetEpg(NULL);
    pDlgProgress->SetPercentage(iTimerPtr / iTimerSize * 10);
  }

  /* remove all EPG entries */
  int iEpgSize = size();
  for (int iEpgPtr = 0; iEpgPtr < iEpgSize; iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);

    CPVRChannel *channel = (CPVRChannel *) epg->ChannelTag();
    if (channel)
      channel->ClearEPG(false); /* clear the database entries afterwards */

    pDlgProgress->SetPercentage((iEpgPtr / iEpgSize * 35) + 10);
  }

  /* clear the database entries */
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();
  database->EraseEPG();
  database->Close();

  pDlgProgress->SetPercentage(50);
  pDlgProgress->Close();

  return true;
}

bool CPVREpgs::ClearEPGForChannel(CPVRChannel *channel)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "PVREpgs - %s - clearing all EPG data for channel '%s'",
      __FUNCTION__, channel->ChannelName().c_str());

  return channel->ClearEPG();
}

bool CPVREpgs::Update()
{
  long iStartTime       = CTimeUtils::GetTimeMS();
  int iChannelCount     = PVRChannelsTV.size() + PVRChannelsRadio.size();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  bool bUpdateSuccess   = true;
  bool bUpdate          = false;

  /* bail out if we're already updating */
  if (m_bInihibitUpdate)
    return false;
  else
    m_bInihibitUpdate = true;

  LoadSettings();

  /* open the database */
  database->Open();

  /* determine if we're going to do a full update */
  if (m_iUpdateTime > 0)
  {
    time_t iLastUpdateTime;
    time_t iCurrentTime;
    database->GetLastEPGScanTime().GetAsTime(iLastUpdateTime);
    CDateTime::GetCurrentDateTime().GetAsTime(iCurrentTime);
    bUpdate = iLastUpdateTime + m_iUpdateTime <= iCurrentTime;
  }

  CLog::Log(LOGNOTICE, "PVREpgs - %s - starting EPG update for %i channels (full update = %d, update time = %d)",
      __FUNCTION__, iChannelCount, bUpdate ? 1 : 0, m_iUpdateTime);

  RemoveOldEntries();

  /* show the progress bar */
  CGUIDialogPVRUpdateProgressBar *scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
  scanner->Show();
  scanner->SetHeader(g_localizeStrings.Get(19004));

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsTime(start); // NOTE: XBMC stores the EPG times as local time
  CDateTime::GetCurrentDateTime().GetAsTime(end);
  start -= m_iLingerTime;
  end   += m_iDaysToDisplay;

  /* update all channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;
    for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
    {
      CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(ptr);
      bUpdateSuccess = UpdateEPGForChannel(channel, &start, &end, bUpdate) || bUpdateSuccess;

      /* update the progress bar */
      scanner->SetProgress(ptr, iChannelCount);
      scanner->SetTitle(channel->ChannelName());
      scanner->UpdateState();
    }
  }

  m_bDatabaseLoaded = true;

  /* update the last scan time if the update was succesful and if we did a full update */
  if (bUpdateSuccess && bUpdate)
    database->UpdateLastEPGScan(CDateTime::GetCurrentDateTime());

  database->Close();
  scanner->Close();
  m_bInihibitUpdate = false;

  long lUpdateTime = CTimeUtils::GetTimeMS() - iStartTime;
  CLog::Log(LOGINFO, "PVREpgs - %s - finished updating the EPG after %li.%li seconds",
      __FUNCTION__, lUpdateTime / 1000, lUpdateTime % 1000);

  return bUpdateSuccess;
}

void CPVREpgs::Unload()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "PVREpgs - %s - unloading all EPG information",
      __FUNCTION__);

  m_bInihibitUpdate = true;

  /* remove all epg tables */
  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CPVREpg *epg = at(iEpgPtr);
    if (epg)
    {
      for (unsigned int iTagPtr = 0; iTagPtr < epg->m_tags.size(); iTagPtr++)
      {
        delete epg->m_tags.at(iTagPtr);
      }
      epg->m_tags.clear();
    }
    delete epg;
    erase(begin() + iEpgPtr);
  }

  /* remove all pointers to epg tables on timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < PVRTimers.size(); iTimerPtr++)
    PVRTimers[iTimerPtr].SetEpg(NULL);
}

bool CPVREpgs::GrabEPGForChannelFromClient(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (g_PVRManager.GetClientProps(channel->ClientID())->SupportEPG &&
      g_PVRManager.Clients()->find(channel->ClientID())->second->ReadyToUse())
  {
    bGrabSuccess = g_PVRManager.Clients()->find(channel->ClientID())->second->GetEPGForChannel(*channel, epg, start, end) == PVR_ERROR_NO_ERROR;
  }
  else
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - client '%s' on client '%li' does not support EPGs",
        __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
  }

  return bGrabSuccess;
}

bool CPVREpgs::GrabEPGForChannelFromScraper(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (channel->Grabber().IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "PVREpgs - %s - no EPG grabber defined for channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
        __FUNCTION__, channel->ChannelName().c_str(), channel->Grabber().c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CPVREpgs::GrabEPGForChannel(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (channel->Grabber() == "client")
  {
      bGrabSuccess = GrabEPGForChannelFromClient(channel, epg, start, end);
  }
  else
  {
      bGrabSuccess = GrabEPGForChannelFromScraper(channel, epg, start, end);
  }

  return bGrabSuccess;
}

bool CPVREpgs::UpdateEPGForChannel(CPVRChannel *channel, time_t *start, time_t *end, bool bUpdate /* = false */)
{
  bool bGrabSuccess     = true;
  CTVDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  CSingleLock lock(m_critSection);

  /* check if this channel is marked for grabbing */
  if (!channel->GrabEpg())
    return false;

  /* get the EPG table for this channel or create it if it doesn't exist */
  CPVREpg *epg = (CPVREpg *) GetEPG(channel, true);
  if (!epg)
    return false;

  /* mark the EPG as being updated */
  epg->SetUpdate(true);

  /* request the epg for this channel from the database */
  if (!m_bIgnoreDbForClient && !m_bDatabaseLoaded)
    bGrabSuccess = database->GetEPGForChannel(*channel, epg, *start, *end);

  /* grab from the client or scraper if we didn't get any result from the database or if an update is scheduled */
  if (!bGrabSuccess || bUpdate)
    bGrabSuccess = GrabEPGForChannel(channel, epg, *start, *end);

  /* store the loaded EPG entries in the database */
  if (bGrabSuccess)
  {
    epg->RemoveOverlappingEvents();

    if (!m_bIgnoreDbForClient)
    {
      for (unsigned int iTagPtr = 0; iTagPtr < epg->InfoTags()->size(); iTagPtr++)
        database->UpdateEPGEntry(*epg->InfoTags()->at(iTagPtr), false, (iTagPtr==0), (iTagPtr == epg->InfoTags()->size()-1));
    }
  }

  epg->SetUpdate(false);

  channel->UpdateEpgPointers();

  return bGrabSuccess;
}

void CPVREpgs::UpdateAllChannelEPGPointers()
{
  /* include radio and tv channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;

    for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
    {
      CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
      if (channel->IsHidden() || !channel->GrabEpg())
        continue;

      const CPVREpg *epg = (CPVREpg *) channel->GetEpg();
      if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
        continue;

      channel->UpdateEpgPointers();
    }
  }
}

const CPVREpg *CPVREpgs::GetEPG(CPVRChannel *channel, bool bAddIfMissing /* = false */)
{
  if (!channel->m_Epg && bAddIfMissing)
    CreateEPG(channel);

  return channel->m_Epg;
}

const CPVREpg *CPVREpgs::GetValidEpgFromChannel(CPVRChannel *channel)
{
  /* don't include hidden channels or channels without an EPG */
  if (channel->IsHidden() || !channel->GrabEpg())
  {
    CLog::Log(LOGDEBUG, "PVREpgs - %s - channel '%s' is not marked for grabbing or is hidden (hidden=%d, grab=%d)",
        __FUNCTION__, channel->ChannelName().c_str(), channel->IsHidden() ? 1 : 0, channel->GrabEpg() ? 1 : 0);
    return NULL;
  }

  const CPVREpg *epg = (CPVREpg *) channel->GetEpg();

  /* don't include epg tables that are being updated at the moment or that aren't valid */
  if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
  {
    CLog::Log(LOGDEBUG, "PVREpgs - %s - channel '%s' does not have a valid epg (updating=%d, valid=%d)",
        __FUNCTION__, channel->ChannelName().c_str(), (epg && epg->IsUpdateRunning()) ? 1 : 0, (epg && epg->IsValid()) ? 1 : 0);
    return NULL;
  }

  return epg;
}

int CPVREpgs::GetEPGAll(CFileItemList* results, bool bRadio /* = false */)
{
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;

  int iInitialSize = results->Size();

  /* include all channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CFileItemList *channelResults = new CFileItemList();

    int iNewEntries = GetEPGForChannel(channel, channelResults);
     for (int iTagPtr = 0; iTagPtr < iNewEntries; iTagPtr++)
    {
      results->Add(channelResults->Get(iTagPtr));
    }

    delete channelResults;
  }

  SetVariableData(results);

  return results->Size() - iInitialSize;
}

CDateTime CPVREpgs::GetFirstEPGDate(bool bRadio /* = false*/)
{
  CDateTime first        = CDateTime::GetCurrentDateTime();
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;

  /* check all channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVREpg *epg = (CPVREpg *)GetValidEpgFromChannel(channels->GetByIndex(iChannelPtr));

    if (!epg)
      continue;

    /* sort the epg and check if the first entry has an earlier start time */
    epg->Sort();
    const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
    if (ch_epg->size() > 0 && ch_epg->at(0)->Start() < first)
      first = ch_epg->at(0)->Start();
  }

  return first;
}

CDateTime CPVREpgs::GetLastEPGDate(bool bRadio /* = false*/)
{
  CDateTime last         = CDateTime::GetCurrentDateTime();
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;

  /* check all channels */
  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVREpg *epg = (CPVREpg *)GetValidEpgFromChannel(channels->GetByIndex(iChannelPtr));

    if (!epg)
      continue;

    /* sort the epg and check if the last entry has a later end time */
    epg->Sort();
    const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
    if (ch_epg->size() > 0 && ch_epg->at(0)->End() > last)
      last = ch_epg->at(0)->End();
  }

  return last;
}

int CPVREpgs::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  /* include radio and tv channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;

    for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
    {
      const CPVREpg *epg = GetValidEpgFromChannel(channels->GetByIndex(iChannelPtr));

      if (!epg)
        continue;

      const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
      for (unsigned int iTagPtr = 0; iTagPtr < ch_epg->size(); iTagPtr++)
      {
        if (filter.FilterEntry(*ch_epg->at(iTagPtr)))
        {
          CFileItemPtr channel(new CFileItem(*ch_epg->at(iTagPtr)));
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

  SetVariableData(results);

  return results->Size();
}

int CPVREpgs::GetEPGForChannel(CPVRChannel *channel, CFileItemList *results)
{
  int iInitialSize   = results->Size();
  const CPVREpg *epg = GetValidEpgFromChannel(channel);

  if (!epg)
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG table",
        __FUNCTION__, channel->ChannelName().c_str());
    return 0;
  }

  const vector<CPVREpgInfoTag*> *channelEpg = epg->InfoTags();
  for (unsigned int iTagPtr = 0; iTagPtr < channelEpg->size(); iTagPtr++)
  {
    CFileItemPtr channelFile(new CFileItem(*channelEpg->at(iTagPtr)));
    channelFile->SetLabel2(channelEpg->at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
    results->Add(channelFile);
  }

  SetVariableData(results);

  return results->Size() - iInitialSize;
}

int CPVREpgs::GetEPGNow(CFileItemList* results, bool bRadio)
{
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;
  int iInitialSize       = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = (CPVREpg *)GetValidEpgFromChannel(channels->GetByIndex(iChannelPtr));

    if (!epg)
    {
      CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG table",
          __FUNCTION__, channel->ChannelName().c_str());
      continue;
    }

    const CPVREpgInfoTag *epgNow = epg->GetInfoTagNow();
    if (!epgNow)
    {
      CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG 'now' table",
          __FUNCTION__, channel->ChannelName().c_str());
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNow));
    entry->SetLabel2(epgNow->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size() - iInitialSize;
}

int CPVREpgs::GetEPGNext(CFileItemList* results, bool bRadio)
{
  CPVRChannels *channels = bRadio ? &PVRChannelsRadio : &PVRChannelsTV;
  int iInitialSize       = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = (CPVREpg *)GetValidEpgFromChannel(channels->GetByIndex(iChannelPtr));

    if (!epg)
    {
      CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG table",
          __FUNCTION__, channel->ChannelName().c_str());
      continue;
    }

    const CPVREpgInfoTag *epgNext = epg->GetInfoTagNext();
    if (!epgNext)
    {
      CLog::Log(LOGINFO, "PVREpgs - %s - channel '%s' does not have a valid EPG 'next' table",
          __FUNCTION__, channel->ChannelName().c_str());
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNext));
    entry->SetLabel2(epgNext->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size() - iInitialSize;
}

///////////////////////////////////////////////////////////////////////////////////

// XXX shouldn't this be done elsewhere and not be updated on each query??
void CPVREpgs::SetVariableData(CFileItemList* results)
{
  /* Reload Timers */
  PVRTimers.Update();

  /* Clear first all Timers set inside the EPG tags */
  for (int j = 0; j < results->Size(); j++)
  {
    CPVREpgInfoTag *epg = results->Get(j)->GetEPGInfoTag();
    if (epg)
      epg->SetTimer(NULL);
  }

  /* Now go with the timers thru the EPG and set the Timer Tag for every matching item */
  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    for (int j = 0; j < results->Size(); j++)
    {
      CPVREpgInfoTag *epg = results->Get(j)->GetEPGInfoTag();
      if (epg)
      {
        if (!PVRTimers[i].Active())
          continue;

        if (epg->ChannelTag()->ChannelNumber() != PVRTimers[i].ChannelNumber())
          continue;

        CDateTime timeAround = CDateTime(time_t((PVRTimers[i].StopTime() - PVRTimers[i].StartTime())/2 + PVRTimers[i].StartTime()));
        if ((epg->Start() <= timeAround) && (epg->End() >= timeAround))
        {
          epg->SetTimer(&PVRTimers[i]);
          break;
        }
      }
    }
  }
}

void CPVREpgs::AssignChangedChannelTags(bool bRadio /* = false*/)
{
  CSingleLock lock(m_critSection);

  CPVRChannels *ch = !bRadio ? &PVRChannelsTV : &PVRChannelsRadio;
  for (unsigned int i = 0; i < ch->size(); i++)
  {
    CPVREpg *Epg = (CPVREpg *) GetEPG(&ch->at(i), true);
    Epg->m_Channel = CPVRChannels::GetByChannelIDFromAll(Epg->ChannelTag()->ChannelID());
  }
}
