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

#include "PVREpgSearchFilter.h"
#include "PVREpgs.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIDialogPVRUpdateProgressBar.h"
#include "GUIWindowManager.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SingleLock.h"

using namespace std;

CPVREpgs PVREpgs;

CPVREpgs::CPVREpgs()
{
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

  return epg;
}

bool CPVREpgs::LoadSettings()
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  m_iLingerTime        = g_guiSettings.GetInt ("pvrmenu.lingertime")*60;
  m_iDaysToDisplay     = g_guiSettings.GetInt ("pvrmenu.daystodisplay")*24*60*60;

  return true;
}

bool CPVREpgs::RemoveOldEntries()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "%s - removing old EPG entries",
      __FUNCTION__);

  CDateTime now = CDateTime::GetCurrentDateTime();

  /* call Cleanup() on all known EPG tables */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    at(ptr)->Cleanup(now);
  }

  return true;
}

bool CPVREpgs::RemoveAllEntries()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "%s - removing old EPG entries",
      __FUNCTION__);

  /* remove all the EPG pointers from timers */
  for (unsigned int ptr = 0; ptr < PVRTimers.size(); ++ptr)
    PVRTimers[ptr].SetEpg(NULL);

  /* remove all EPG entries */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVREpg *epg = at(ptr);

    CPVRChannel *channel = (CPVRChannel *) epg->ChannelTag();
    if (channel)
      channel->ClearEPG(false); /* clear the database entries afterwards */
  }

  /* clear the database entries */
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();
  database->EraseEPG();
  database->Close();

  return true;
}

bool CPVREpgs::ClearEPGForChannel(CPVRChannel *channel)
{
  CLog::Log(LOGINFO, "%s - clearing all EPG data for channel %s",
      __FUNCTION__, channel->ChannelName().c_str());

  CSingleLock lock(m_critSection);

  return channel->ClearEPG();
}

bool CPVREpgs::Update()
{
  long perfCnt          = CTimeUtils::GetTimeMS();
  int iChannelCount     = PVRChannelsTV.size() + PVRChannelsRadio.size();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  bool bAllNewChannels  = true;

  /* bail out if we're already updating */
  if (m_bInihibitUpdate)
    return false;
  else
    m_bInihibitUpdate = true;

  CLog::Log(LOGNOTICE, "%s - starting EPG update for %i channels",
      __FUNCTION__, iChannelCount);

  LoadSettings();

  /* remove old entries */
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

  /* open the database */
  database->Open();

  /* update all channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;
    for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
    {
      CPVRChannel *channel = &channels->at(ptr);
      bAllNewChannels = UpdateEPGForChannel(channel, &start, &end) && bAllNewChannels;

      /* update the progress bar */
      scanner->SetProgress(ptr, iChannelCount);
      scanner->SetTitle(channel->ChannelName());
      scanner->UpdateState();
    }
  }

  database->UpdateLastEPGScan(CDateTime::GetCurrentDateTime());

  database->Close();
  scanner->Close();
  m_bInihibitUpdate = false;

  CLog::Log(LOGINFO, "%s - finished updating the EPG after %li.%li seconds",
      __FUNCTION__, (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);

  return true;
}

void CPVREpgs::Unload()
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "%s - unloading all EPG information",
      __FUNCTION__);

  m_bInihibitUpdate = true;

  for (unsigned int epgPtr = 0; epgPtr < size(); epgPtr++)
  {
    CPVREpg *epg = at(epgPtr);
    if (epg)
    {
      for (unsigned int tagPtr = 0; tagPtr < epg->m_tags.size(); tagPtr++)
      {
        delete epg->m_tags.at(tagPtr);
      }
      epg->m_tags.clear();
    }
    delete epg;
    erase(begin()+epgPtr);
  }

  for (unsigned int ptr = 0; ptr < PVRTimers.size(); ptr++)
    PVRTimers[ptr].SetEpg(NULL);
}

bool CPVREpgs::GrabEPGForChannelFromClient(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bSaveInDatabase = false;

  if (g_PVRManager.GetClientProps(channel->ClientID())->SupportEPG &&
      g_PVRManager.Clients()->find(channel->ClientID())->second->ReadyToUse())
  {
    bSaveInDatabase = g_PVRManager.Clients()->find(channel->ClientID())->second->GetEPGForChannel(*channel, epg, start, end) == PVR_ERROR_NO_ERROR;
    if (epg->InfoTags()->size() == 0)
    {
      CLog::Log(LOGDEBUG, "%s - channel '%s' on client '%li' contains no EPG data",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
    }
  }
  else
  {
    CLog::Log(LOGINFO, "%s - client '%s' on client '%li' does not support EPGs",
        __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
  }

  return bSaveInDatabase;
}

bool CPVREpgs::GrabEPGForChannelFromScraper(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bSaveInDatabase = false;

  if (channel->Grabber().IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "%s - no EPG grabber defined for channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "%s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
        __FUNCTION__, channel->ChannelName().c_str(), channel->Grabber().c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bSaveInDatabase;
}

bool CPVREpgs::GrabEPGForChannel(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bSaveInDatabase = false;

  if (channel->Grabber() == "client")
  {
    bSaveInDatabase = GrabEPGForChannelFromClient(channel, epg, start, end);
  }
  else
  {
    bSaveInDatabase = GrabEPGForChannelFromScraper(channel, epg, start, end);
  }

  return bSaveInDatabase;
}

bool CPVREpgs::UpdateEPGForChannel(CPVRChannel *channel, time_t *start, time_t *end)
{
  bool bChannelExists = false;
  bool bSaveInDatabase = true;
  CTVDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  CSingleLock lock(m_critSection);

  /* check if this channel is marked for grabbing */
  if (!channel->GrabEpg())
    return false;

  /* get the EPG table for this channel or create it if it doesn't exist */
  CPVREpg *epg = (CPVREpg *) CreateEPG(channel);
  if (!epg)
    return false;

  /* mark the EPG as being updated */
  epg->SetUpdate(true);

  /* request the epg for this channel from the database */
  bChannelExists = database->GetEPGForChannel(*channel, epg, *start, *end);
  if (!bChannelExists)
  {
    /* If "retval" is false the database contains no EPG for this channel. Load it from the client or from
       scrapers for only one day to speed up load. The rest is done by the background update */
    CDateTime::GetUTCDateTime().GetAsTime(*start); // NOTE: XBMC stores the EPG times as local time
    CDateTime::GetUTCDateTime().GetAsTime(*end);
    *start -= m_iLingerTime;
    *end   += (m_bIgnoreDbForClient) ? m_iDaysToDisplay : 24*60*60;

    if (m_bIgnoreDbForClient)
    {
      bSaveInDatabase = false;
    }
    else
    {
      bSaveInDatabase = GrabEPGForChannel(channel, epg, *start, *end);
    }

    /* store the loaded EPG entries in the database */
    if (bSaveInDatabase)
    {
      for (unsigned int ptr = 0; ptr < epg->InfoTags()->size(); ptr++)
        database->UpdateEPGEntry(*epg->InfoTags()->at(ptr), false, (ptr==0), (ptr >= epg->InfoTags()->size()-1));
    }

    channel->ResetChannelEPGLinks();
  }

  epg->RemoveOverlappingEvents();
  epg->SetUpdate(false);

  // TODO update channel EPG pointers

  return bChannelExists ? false : bSaveInDatabase;
}


const CPVREpg *CPVREpgs::GetEPG(CPVRChannel *channel, bool bAddIfMissing /* = false */)
{
  if (!channel->m_Epg && bAddIfMissing)
    CreateEPG(channel);

  return channel->m_Epg;
}

CDateTime CPVREpgs::GetFirstEPGDate(bool radio /* = false*/)
{
  CDateTime first        = CDateTime::GetCurrentDateTime();
  CPVRChannels *channels = radio ? &PVRChannelsRadio : &PVRChannelsTV;

  /* check all channels */
  for (unsigned int channelPtr = 0; channelPtr < channels->size(); channelPtr++)
  {
    /* don't include hidden channels or channels without an EPG */
    if (channels->at(channelPtr).IsHidden() || !channels->at(channelPtr).GrabEpg())
      continue;

    CPVREpg *epg = (CPVREpg *) GetEPG(&channels->at(channelPtr), false);

    /* don't include epg tables that are being updated at the moment or that aren't valid */
    if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
      continue;

    /* sort the epg and check if the first entry has an earlier start time */
    epg->Sort();
    const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
    if (ch_epg->size() > 0 && ch_epg->at(0)->Start() < first)
      first = ch_epg->at(0)->Start();
  }

  return first;
}

CDateTime CPVREpgs::GetLastEPGDate(bool radio /* = false*/)
{
  CDateTime last         = CDateTime::GetCurrentDateTime();
  CPVRChannels *channels = radio ? &PVRChannelsRadio : &PVRChannelsTV;

  /* check all channels */
  for (unsigned int channelPtr = 0; channelPtr < channels->size(); channelPtr++)
  {
    /* don't include hidden channels or channels without an EPG */
    if (channels->at(channelPtr).IsHidden() || !channels->at(channelPtr).GrabEpg())
      continue;

    CPVREpg *epg = (CPVREpg *) GetEPG(&channels->at(channelPtr), false);

    /* don't include epg tables that are being updated at the moment or that aren't valid */
    if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
      continue;

    /* sort the epg and check if the last entry has a later end time */
    epg->Sort();
    const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
    if (ch_epg->size() > 0 && ch_epg->at(0)->End() > last)
      last = ch_epg->at(0)->End();
  }

  return last;
}

int CPVREpgs::GetEPGAll(CFileItemList* results, bool radio /* = false */)
{
  CPVRChannels *channels = radio ? &PVRChannelsRadio : &PVRChannelsTV;

  /* include all channels */
  for (unsigned int channelPtr = 0; channelPtr < channels->size(); channelPtr++)
  {
    /* don't include hidden channels or channels without an EPG */
    if (channels->at(channelPtr).IsHidden() || !channels->at(channelPtr).GrabEpg())
      continue;

    CPVREpg *epg = (CPVREpg *) GetEPG(&channels->at(channelPtr), false);

    /* don't include epg tables that are being updated at the moment or that aren't valid */
    if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
      continue;

    /* add all infotags */
    const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();
    for (unsigned int tagPtr = 0; tagPtr < ch_epg->size(); tagPtr++)
    {
      CFileItemPtr channel(new CFileItem(*ch_epg->at(tagPtr)));
      results->Add(channel);
    }
  }

  SetVariableData(results);

  return results->Size();
}

int CPVREpgs::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  /* include radio and tv channels */
  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;
    for (unsigned int channelPtr = 0; channelPtr < channels->size(); channelPtr++)
    {
      CPVRChannel *channel = &channels->at(channelPtr);

      /* don't include hidden channels or channels without an EPG */
      if (channel->IsHidden() || !channel->GrabEpg())
        continue;

      const CPVREpg *epg = GetEPG(channel, false);

      /* don't include epg tables that are being updated at the moment or that aren't valid */
      if (!epg || epg->IsUpdateRunning() || !epg->IsValid())
        continue;

      const vector<CPVREpgInfoTag*> *ch_epg = epg->InfoTags();

      for (unsigned int tagPtr = 0; tagPtr < ch_epg->size(); tagPtr++)
      {
        if (filter.FilterEntry(*ch_epg->at(tagPtr)))
        {
          CFileItemPtr channel(new CFileItem(*ch_epg->at(tagPtr)));
          results->Add(channel);
        }
      }
    }
  }

  /* filter recordings */
  if (filter.m_bIgnorePresentRecordings && PVRRecordings.size() > 0)
  {
    for (unsigned int recordingPtr = 0; recordingPtr < PVRRecordings.size(); recordingPtr++)
    {
      for (int resultPtr = 0; resultPtr < results->Size(); resultPtr++)
      {
        const CPVREpgInfoTag *epgentry = results->Get(resultPtr)->GetEPGInfoTag();
        if (epgentry)
        {
          if (epgentry->Title() != PVRRecordings[recordingPtr].Title())
            continue;
          if (epgentry->PlotOutline() != PVRRecordings[recordingPtr].PlotOutline())
            continue;
          if (epgentry->Plot() != PVRRecordings[recordingPtr].Plot())
            continue;

          results->Remove(resultPtr);
          resultPtr--;
        }
      }
    }
  }

  /* filter timers */
  if (filter.m_bIgnorePresentTimers && PVRTimers.size() > 0)
  {
    for (unsigned int timerPtr = 0; timerPtr < PVRTimers.size(); timerPtr++)
    {
      for (int resultPtr = 0; resultPtr < results->Size(); resultPtr++)
      {
        const CPVREpgInfoTag *epgentry = results->Get(resultPtr)->GetEPGInfoTag();
        if (epgentry)
        {
          if (epgentry->ChannelTag()->ChannelNumber() != PVRTimers[timerPtr].ChannelNumber())
            continue;
          if (epgentry->Start() < PVRTimers[timerPtr].Start())
            continue;
          if (epgentry->End() > PVRTimers[timerPtr].Stop())
            continue;

          results->Remove(resultPtr);
          resultPtr--;
        }
      }
    }
  }

  /* remove duplicate entries */
  if (filter.m_bPreventRepeats)
  {
    unsigned int size = results->Size();
    for (unsigned int resultPtr = 0; resultPtr < size; resultPtr++)
    {
      const CPVREpgInfoTag *epgentry_1 = results->Get(resultPtr)->GetEPGInfoTag();
      for (unsigned int tagPtr = 0; tagPtr < size; tagPtr++)
      {
        const CPVREpgInfoTag *epgentry_2 = results->Get(tagPtr)->GetEPGInfoTag();
        if (resultPtr == tagPtr)
          continue;

        if (epgentry_1->Title() != epgentry_2->Title())
          continue;

        if (epgentry_1->Plot() != epgentry_2->Plot())
          continue;

        if (epgentry_1->PlotOutline() != epgentry_2->PlotOutline())
          continue;

        results->Remove(tagPtr);
        size = results->Size();
        resultPtr--;
        tagPtr--;
      }
    }
  }

  SetVariableData(results);

  return results->Size();
}

///////////////////////////////////////////////////////////////////////////////////

int CPVREpgs::GetEPGChannel(unsigned int number, CFileItemList* results, bool radio)
{
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  if (!ch->at(number-1).m_bGrabEpg)
    return 0;

  const CPVREpg *Epg = GetEPG(&ch->at(number-1), true);
  if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
  {
    const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();
    for (unsigned int i = 0; i < ch_epg->size(); i++)
    {
      CFileItemPtr channel(new CFileItem(*ch_epg->at(i)));
      channel->SetLabel2(ch_epg->at(i)->Start().GetAsLocalizedDateTime(false, false));
      results->Add(channel);
    }
    SetVariableData(results);
  }

  return results->Size();
}

int CPVREpgs::GetEPGNow(CFileItemList* results, bool radio)
{
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_bIsHidden || !ch->at(i).m_bGrabEpg)
      continue;

    const CPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (!Epg || Epg->IsUpdateRunning() || !Epg->IsValid())
      continue;

    const CPVREpgInfoTag *epgnow = Epg->GetInfoTagNow();
    if (!epgnow)
      continue;

    CFileItemPtr entry(new CFileItem(*epgnow));
    entry->SetLabel2(epgnow->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = ch->at(i).ChannelName();
    entry->SetThumbnailImage(ch->at(i).Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size();
}

int CPVREpgs::GetEPGNext(CFileItemList* results, bool radio)
{
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_bIsHidden || !ch->at(i).GrabEpg())
      continue;

    const CPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (!Epg || Epg->IsUpdateRunning() || !Epg->IsValid())
      continue;

    const CPVREpgInfoTag *epgnext = Epg->GetInfoTagNext();
    if (!epgnext)
      continue;

    CFileItemPtr entry(new CFileItem(*epgnext));
    entry->SetLabel2(epgnext->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = ch->at(i).ChannelName();
    entry->SetThumbnailImage(ch->at(i).Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size();
}

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

void CPVREpgs::AssignChangedChannelTags(bool radio/* = false*/)
{
  CSingleLock lock(m_critSection);

  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  for (unsigned int i = 0; i < ch->size(); i++)
  {
    CPVREpg *Epg = (CPVREpg *) GetEPG(&ch->at(i), true);
    Epg->m_Channel = CPVRChannels::GetByChannelIDFromAll(Epg->ChannelID());
  }
}
