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

#include "FileItem.h"
#include "Application.h"
#include "PVREpgSearchFilter.h"
#include "PVREpgs.h"
#include "PVRChannels.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIDialogPVRUpdateProgressBar.h"
#include "GUIWindowManager.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SingleLock.h"
#include "LocalizeStrings.h"
#include "TextSearch.h"

using namespace std;

CPVREpgs PVREpgs;

CPVREpgs::CPVREpgs(void)
{
}

bool CPVREpgs::LoadSettings()
{
  m_bIgnoreDbForClient = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  m_iLingerTime        = g_guiSettings.GetInt("pvrmenu.lingertime")*60;
  m_iDaysToDisplay     = g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

  return true;
}

bool CPVREpgs::Cleanup()
{
  CSingleLock lock(m_critSection);
  CDateTime now = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGINFO, "%s - cleaning up EPG data", __FUNCTION__);

  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;
    for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
    {
      CPVREpg *epg = (CPVREpg *) GetEPG(&channels->at(ptr), true);
      epg->Cleanup(now);
    }
  }

  return true;
}

bool CPVREpgs::ClearAll(void)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGINFO, "%s - clearing all EPG data",
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
      channel->ClearEPG(false);
  }

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();
  database->EraseEPG();
  database->Close();

  return true;
}

bool CPVREpgs::ClearChannel(long ChannelID)
{
  CLog::Log(LOGINFO, "%s - clearing all EPG data from channel %li",
      __FUNCTION__, ChannelID);

  for (unsigned int radio = 0; radio <= 1; radio++)
  {
    CPVRChannels *channels = (radio == 0) ? &PVRChannelsTV : &PVRChannelsRadio;
    for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
    {
      CPVRChannel *channel = &channels->at(ptr);
      if (channel->ChannelID() == ChannelID)
      {
        CSingleLock lock(m_critSection);
        return channel->ClearEPG();
      }
    }
  }

  CLog::Log(LOGINFO, "%s - channel %ld wasn't found",
      __FUNCTION__, ChannelID);
  return false;
}

bool CPVREpgs::RemoveOverlappingEvents(CTVDatabase *database,CPVREpg *epg)
{
  /// This will check all programs in the list and
  /// will remove any overlapping programs
  /// An overlapping program is a tv program which overlaps with another tv program in time
  /// for example.
  ///   program A on MTV runs from 20.00-21.00 on 1 november 2004
  ///   program B on MTV runs from 20.55-22.00 on 1 november 2004
  ///   this case, program B will be removed
  epg->Sort();
  CStdString previousName = "";
  CDateTime previousStart;
  CDateTime previousEnd(1980, 1, 1, 0, 0, 0);
  for (unsigned int ptr = 0; ptr < epg->InfoTags()->size(); ptr++)
  {
    if (previousEnd > epg->InfoTags()->at(ptr)->Start())
    {
      //remove this program
      CLog::Log(LOGNOTICE, "PVR: Removing Overlapped TV Event '%s' on channel '%s' at date '%s' to '%s'",
          epg->InfoTags()->at(ptr)->Title().c_str(),
          epg->InfoTags()->at(ptr)->ChannelTag()->ChannelName().c_str(),
          epg->InfoTags()->at(ptr)->Start().GetAsLocalizedDateTime(false, false).c_str(),
          epg->InfoTags()->at(ptr)->End().GetAsLocalizedDateTime(false, false).c_str());
      CLog::Log(LOGNOTICE, "     Overlapps with '%s' at date '%s' to '%s'",
          previousName.c_str(),
          previousStart.GetAsLocalizedDateTime(false, false).c_str(),
          previousEnd.GetAsLocalizedDateTime(false, false).c_str());

      database->RemoveEPGEntry(*epg->InfoTags()->at(ptr));
      epg->DelInfoTag(epg->InfoTags()->at(ptr));
    }
    else
    {
      previousName = epg->InfoTags()->at(ptr)->Title();
      previousStart = epg->InfoTags()->at(ptr)->Start();
      previousEnd = epg->InfoTags()->at(ptr)->End();
    }
  }

  return true;
}

bool CPVREpgs::Update()
{
  long perfCnt          = CTimeUtils::GetTimeMS();
  int iChannelCount     = PVRChannelsTV.size() + PVRChannelsRadio.size();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  bool bAllNewChannels  = true;

  if (m_bInihibitUpdate)
    return false;
  else
    m_bInihibitUpdate = true;

  LoadSettings();

  CLog::Log(LOGNOTICE, "%s - starting EPG update for %i channels",
      __FUNCTION__, iChannelCount);

  /* show the progress bar */
  CGUIDialogPVRUpdateProgressBar *scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
  scanner->Show();
  scanner->SetHeader(g_localizeStrings.Get(19004));

  /* open the database */
  database->Open();

  /* remove old entries */
  database->EraseOldEPG();

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
      CPVRChannel *channel = &channels->at(ptr);
      bAllNewChannels = UpdateEPGForChannel(database, channel, &start, &end) && bAllNewChannels;

      /* update the progress bar */
      scanner->SetProgress(ptr, iChannelCount);
      scanner->SetTitle(channel->ChannelName());
      scanner->UpdateState();
    }
  }

  database->UpdateLastEPGScan(CDateTime::GetCurrentDateTime());

  /* close up */
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

bool CPVREpgs::GrabEPGForChannel(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end)
{
  bool bSaveInDatabase = false;

  /* load the EPG from the client */
  if (channel->Grabber() == "client" &&
      g_PVRManager.GetClientProps(channel->ClientID())->SupportEPG &&
      g_PVRManager.Clients()->find(channel->ClientID())->second->ReadyToUse())
  {
    bSaveInDatabase = g_PVRManager.Clients()->find(channel->ClientID())->second->GetEPGForChannel(*channel, epg, start, end) == PVR_ERROR_NO_ERROR;
    if (epg->InfoTags()->size() > 0)
    {
      CLog::Log(LOGINFO, "%s - the database contains no EPG data for channel '%s' loaded from client '%li'",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - channel '%s' on client '%li' contains no EPG data",
          __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
    }
  }
  else
  {
    if (channel->Grabber().IsEmpty()) /* no grabber defined */
    {
      CLog::Log(LOGERROR, "%s - no EPG grabber defined for channel '%s'",
          __FUNCTION__, channel->ChannelName().c_str());
      bSaveInDatabase = false;
    }
    else
    {
      CLog::Log(LOGINFO, "%s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
          __FUNCTION__, channel->ChannelName().c_str(), channel->Grabber().c_str());
      CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
      // TODO: Add Support for Web EPG Scrapers here
    }
  }

  return bSaveInDatabase;
}

bool CPVREpgs::UpdateEPGForChannel(CTVDatabase *database, CPVRChannel *channel, time_t *start, time_t *end)
{
  bool bChannelExists = false;
  bool bSaveInDatabase = true;

  CSingleLock lock(m_critSection);

  /* check if this channel is marked for grabbing */
  if (!channel->GrabEpg())
    return false;

  /* get the EPG table for this channel or create it if it doesn't exist */
  CPVREpg *epg = (CPVREpg *) AddEPG(channel->ChannelID());
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

  RemoveOverlappingEvents(database, epg);
  epg->SetUpdate(false);

  // XXX Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster
  GetEPG(channel);

  return bChannelExists ? false : bSaveInDatabase;
}

int CPVREpgs::GetEPGAll(CFileItemList* results, bool radio)
{
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_bIsHidden || !ch->at(i).m_bGrabEpg)
      continue;

    const CPVREpg *Epg = GetEPG(&ch->at(i), false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();
      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        CFileItemPtr channel(new CFileItem(*ch_epg->at(i)));
        results->Add(channel);
      }
    }
  }
  SetVariableData(results);

  return results->Size();
}

int CPVREpgs::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    if (PVRChannelsTV[i].m_bIsHidden || !PVRChannelsTV[i].m_bGrabEpg)
      continue;

    const CPVREpg *Epg = GetEPG(&PVRChannelsTV[i], false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();

      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        if (filter.FilterEntry(*ch_epg->at(i)))
        {
          CFileItemPtr channel(new CFileItem(*ch_epg->at(i)));
          results->Add(channel);
        }
      }
    }
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    if (PVRChannelsRadio[i].m_bIsHidden || !PVRChannelsRadio[i].m_bGrabEpg)
      continue;

    const CPVREpg *Epg = GetEPG(&PVRChannelsRadio[i], false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();

      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        if (filter.FilterEntry(*ch_epg->at(i)))
        {
          CFileItemPtr channel(new CFileItem(*ch_epg->at(i)));
          results->Add(channel);
        }
      }
    }
  }

  if (filter.m_bIgnorePresentRecordings && PVRRecordings.size() > 0)
  {
    for (unsigned int i = 0; i < PVRRecordings.size(); i++)
    {
      for (int j = 0; j < results->Size(); j++)
      {
        const CPVREpgInfoTag *epgentry = results->Get(j)->GetEPGInfoTag();
        if (epgentry)
        {
          if (epgentry->Title() != PVRRecordings[i].Title())
            continue;
          if (epgentry->PlotOutline() != PVRRecordings[i].PlotOutline())
            continue;
          if (epgentry->Plot() != PVRRecordings[i].Plot())
            continue;

          results->Remove(j);
          j--;
        }
      }
    }
  }

  if (filter.m_bIgnorePresentTimers && PVRTimers.size() > 0)
  {
    for (unsigned int i = 0; i < PVRTimers.size(); i++)
    {
      for (int j = 0; j < results->Size(); j++)
      {
        const CPVREpgInfoTag *epgentry = results->Get(j)->GetEPGInfoTag();
        if (epgentry)
        {
          if (epgentry->ChannelTag()->ChannelNumber() != PVRTimers[i].ChannelNumber())
            continue;
          if (epgentry->Start() < PVRTimers[i].Start())
            continue;
          if (epgentry->End() > PVRTimers[i].Stop())
            continue;

          results->Remove(j);
          j--;
        }
      }
    }
  }

  if (filter.m_bPreventRepeats)
  {
    unsigned int size = results->Size();
    for (unsigned int i = 0; i < size; i++)
    {
      const CPVREpgInfoTag *epgentry_1 = results->Get(i)->GetEPGInfoTag();
      for (unsigned int j = 0; j < size; j++)
      {
        const CPVREpgInfoTag *epgentry_2 = results->Get(j)->GetEPGInfoTag();
        if (i == j)
          continue;

        if (epgentry_1->Title() != epgentry_2->Title())
          continue;

        if (epgentry_1->Plot() != epgentry_2->Plot())
          continue;

        if (epgentry_1->PlotOutline() != epgentry_2->PlotOutline())
          continue;

        results->Remove(j);
        size = results->Size();
        i--;
        j--;
      }
    }
  }

  SetVariableData(results);

  return results->Size();
}

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

const CPVREpg *CPVREpgs::AddEPG(long iChannelID)
{
  const CPVREpg *epg = new CPVREpg(iChannelID);
  push_back((CPVREpg *) epg);
  CPVRChannel *channel = CPVRChannels::GetByChannelIDFromAll(iChannelID);
  if (channel)
    channel->m_Epg = epg;

  return epg;
}

const CPVREpg *CPVREpgs::GetEPG(long iChannelID, bool bAddIfMissing)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->ChannelID() == iChannelID)
      return at(i);
  }

  if (bAddIfMissing)
    return AddEPG(iChannelID);

  return NULL;
}

const CPVREpg *CPVREpgs::GetEPG(CPVRChannel *Channel, bool bAddIfMissing)
{
  if (!Channel->m_Epg && bAddIfMissing)
    AddEPG(Channel->ChannelID());

  return Channel->m_Epg;
}

/////////////////////////////////////////////////////////////

CDateTime CPVREpgs::GetFirstEPGDate(bool radio/* = false*/)
{
  CDateTime first = CDateTime::GetCurrentDateTime();
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_bIsHidden || !ch->at(i).GrabEpg())
      continue;

    const CPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (Epg->IsUpdateRunning())
      continue;

    const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();

    for (unsigned int j = 0; j < ch_epg->size(); j++)
    {
      if (ch_epg->at(j)->Start() < first)
        first = ch_epg->at(j)->Start();
    }
  }

  return first;
}

CDateTime CPVREpgs::GetLastEPGDate(bool radio/* = false*/)
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  CPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_bIsHidden)
      continue;

    const CPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (Epg->IsUpdateRunning())
      continue;

    const vector<CPVREpgInfoTag*> *ch_epg = Epg->InfoTags();

    for (unsigned int j = 0; j < ch_epg->size(); j++)
    {
      if (ch_epg->at(j)->End() >= last)
        last = ch_epg->at(j)->End();
    }
  }
  return last;
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

