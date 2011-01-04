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

void CPVREpgs::Cleanup()
{
  CLog::Log(LOGINFO, "PVR: cleaning up epg data");

  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    CPVREpg *Epg = (CPVREpg *) GetEPG(&PVRChannelsTV[i], true);
    Epg->Cleanup(CDateTime::GetCurrentDateTime());
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    CPVREpg *Epg = (CPVREpg *) GetEPG(&PVRChannelsRadio[i], true);
    Epg->Cleanup(CDateTime::GetCurrentDateTime());
  }
}

bool CPVREpgs::ClearAll(void)
{
  CSingleLock lock(m_critSection);

  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
    PVRTimers[i].SetEpg(NULL);
  for (unsigned int i = 0; i < size(); i++)
    at(i)->Cleanup(-1);

  return true;
}

bool CPVREpgs::ClearChannel(long ChannelID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (ChannelID == at(i)->ChannelID())
    {
      CSingleLock lock(m_critSection);

      at(i)->Cleanup(-1);
      CTVDatabase *database = g_PVRManager.GetTVDatabase();
      database->Open();
      database->EraseChannelEPG(ChannelID);
      database->Close();
      return true;
    }
  }

  return false;
}

void CPVREpgs::Load()
{
  long perfCnt          = CTimeUtils::GetTimeMS();
  int channelcount      = PVRChannelsTV.size() + PVRChannelsRadio.size();
  bool ignoreDB         = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  int lingertime        = g_guiSettings.GetInt("pvrmenu.lingertime")*60;
  int daysToLoad        = g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;
  CLIENTMAP   *clients  = g_PVRManager.Clients();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  bool completeNew      = true;
  m_bInihibitUpdate     = true;

  CLog::Log(LOGINFO, "PVR: Starting EPG load for %i channels", channelcount);

  CGUIDialogPVRUpdateProgressBar *scanner = (CGUIDialogPVRUpdateProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
  scanner->Show();
  scanner->SetHeader(g_localizeStrings.Get(19004));

  database->Open();

  time_t start;
  time_t end;
  CDateTime::GetCurrentDateTime().GetAsTime(start); // NOTE: XBMC stores the EPG times as local time
  CDateTime::GetCurrentDateTime().GetAsTime(end);
  start -= lingertime;
  end   += daysToLoad;

  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    CSingleLock lock(m_critSection);

    if (PVRChannelsTV[i].GrabEpg())
    {
      scanner->SetProgress(i, channelcount);
      scanner->SetTitle(PVRChannelsTV[i].ChannelName());
      scanner->UpdateState();
      CPVREpg *p = AddEPG(PVRChannelsTV[i].ChannelID());
      if (p)
      {
        bool ret = database->GetEPGForChannel(PVRChannelsTV[i], p, start, end);
        if (ret)
        {
          completeNew = false;
        }
        else
        {
          // If "ret" is false the database contains no EPG for this channel, load it now from client or over
          // scrapers for only one day to speed up load, rest is done by Background Update
          CDateTime::GetUTCDateTime().GetAsTime(start); // NOTE: Client EPG times are must be UTC based
          CDateTime::GetUTCDateTime().GetAsTime(end);
          start -= lingertime;
          if (ignoreDB)
            end += daysToLoad;
          else
            end += 24*60*60; // One day

          if (PVRChannelsTV[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsTV[i].ClientID())->SupportEPG && clients->find(PVRChannelsTV[i].ClientID())->second->ReadyToUse())
          {
            ret = clients->find(PVRChannelsTV[i].ClientID())->second->GetEPGForChannel(PVRChannelsTV[i], p, start, end) == PVR_ERROR_NO_ERROR ? true : false;
            if (ignoreDB)
              ret = false;  // This prevent the save of the loaded data inside the Database
            else if (p->InfoTags()->size() > 0)
              CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for TV Channel '%s', loaded from client '%li'", PVRChannelsTV[i].ChannelName().c_str(), PVRChannelsTV[i].ClientID());
            else
              CLog::Log(LOGDEBUG, "PVR: TV Channel '%s', on client '%li' contains no EPG data", PVRChannelsTV[i].ChannelName().c_str(), PVRChannelsTV[i].ClientID());
          }
          else
          {
            if (PVRChannelsTV[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].ChannelName().c_str());
              return;
            }
            CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for TV Channel '%s', loading with scraper '%s'", PVRChannelsTV[i].ChannelName().c_str(), PVRChannelsTV[i].Grabber().c_str());
            /// TODO: Add Support for Web EPG Scrapers here
          }

          // Store the loaded EPG entries inside Database
          if (ret)
          {
            for (unsigned int i = 0; i < p->InfoTags()->size(); i++)
              database->AddEPGEntry(*p->InfoTags()->at(i), false, (i==0), (i >= p->InfoTags()->size()-1));
          }
        }

        // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
        GetEPG(&PVRChannelsTV[i]);
      }
    }
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    CSingleLock lock(m_critSection);

    if (PVRChannelsRadio[i].GrabEpg())
    {
      scanner->SetProgress(PVRChannelsTV.size()+i, channelcount);
      scanner->SetTitle(PVRChannelsRadio[i].ChannelName());
      scanner->UpdateState();
      CPVREpg *p = AddEPG(PVRChannelsRadio[i].ChannelID());
      if (p)
      {
        bool ret = database->GetEPGForChannel(PVRChannelsRadio[i], p, start, end);
        if (ret)
        {
          completeNew = false;
        }
        else
        {
          // If "ret" is false the database contains no EPG for this channel, load it now from client or over
          // scrapers for only one day to speed up load
          CDateTime::GetUTCDateTime().GetAsTime(start); // NOTE: Client EPG times are must be UTC based
          CDateTime::GetUTCDateTime().GetAsTime(end);
          start -= lingertime;
          if (ignoreDB)
            end += daysToLoad;
          else
            end += 24*60*60; // One day

          if (PVRChannelsRadio[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsRadio[i].ClientID())->SupportEPG && clients->find(PVRChannelsRadio[i].ClientID())->second->ReadyToUse())
          {
            ret = clients->find(PVRChannelsRadio[i].ClientID())->second->GetEPGForChannel(PVRChannelsRadio[i], p, start, end) == PVR_ERROR_NO_ERROR ? true : false;
            if (ignoreDB)
              ret = false;  // This prevent the save of the loaded data inside the Database
            else if (p->InfoTags()->size() > 0)
              CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for Radio Channel '%s', loaded from client '%li'", PVRChannelsRadio[i].ChannelName().c_str(), PVRChannelsRadio[i].ClientID());
            else
              CLog::Log(LOGDEBUG, "PVR: Radio Channel '%s', on client '%li' contains no EPG data", PVRChannelsRadio[i].ChannelName().c_str(), PVRChannelsRadio[i].ClientID());
          }
          else
          {
            if (PVRChannelsRadio[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].ChannelName().c_str());
              return;
            }
            CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for Radio Channel '%s', loading with scraper '%s'", PVRChannelsRadio[i].ChannelName().c_str(), PVRChannelsRadio[i].Grabber().c_str());
            /// TODO: Add Support for Web EPG Scrapers here
          }

          // Store the loaded EPG entries inside Database
          if (ret)
          {
            for (unsigned int i = 0; i < p->InfoTags()->size(); i++)
              database->AddEPGEntry(*p->InfoTags()->at(i), false, (i==0), (i >= p->InfoTags()->size()-1));
          }
        }

        // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
        GetEPG(&PVRChannelsRadio[i]);
      }
    }
  }

  if (completeNew)
    database->UpdateLastEPGScan(CDateTime::GetCurrentDateTime());

  database->Close();
  scanner->Close();
  m_bInihibitUpdate = false;

  CLog::Log(LOGINFO, "PVR: EPG load finished after %li.%li seconds", (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);

  return;
}

void CPVREpgs::Unload()
{
  CSingleLock lock(m_critSection);

  CLog::Log(LOGINFO, "PVR: Unloading EPG information");

  m_bInihibitUpdate = true;

  for (unsigned int i = 0; i < size(); i++)
  {
    CPVREpg *p = at(i);
    if (p)
    {
      for (unsigned int j = 0; j < p->m_tags.size(); j++)
      {
        delete p->m_tags.at(j);
      }
      p->m_tags.clear();
    }
    delete p;
    erase(begin()+i);
  }

  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
    PVRTimers[i].SetEpg(NULL);
}

void CPVREpgs::Update(bool Scan)
{
  long perfCnt      = CTimeUtils::GetTimeMS();
  int channelcount  = PVRChannelsTV.size() + PVRChannelsRadio.size();
  bool ignoreDB     = g_guiSettings.GetBool("pvrepg.ignoredbforclient");
  int lingertime    = g_guiSettings.GetInt("pvrmenu.lingertime")*60;
  int daysToLoad    = g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

  if (m_bInihibitUpdate)
    return;

  if (Scan)
    CLog::Log(LOGNOTICE, "PVR: Starting EPG scan for %i channels", channelcount);
  else
    CLog::Log(LOGNOTICE, "PVR: Starting EPG update for %i channels", channelcount);

  time_t endLoad;
  CDateTime::GetCurrentDateTime().GetAsTime(endLoad);
  endLoad += daysToLoad;

  CLIENTMAP   *clients  = g_PVRManager.Clients();
  CTVDatabase *database = g_PVRManager.GetTVDatabase();

  database->Open();
  database->EraseOldEPG();

  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    if (m_bInihibitUpdate)
      return;

    CSingleLock lock(m_critSection);

    if (PVRChannelsTV[i].GrabEpg())
    {
      CPVREpg *p = AddEPG(PVRChannelsTV[i].ChannelID());
      if (p)
      {
        bool ret = false;

        p->SetUpdate(true);

        if (Scan)
        {
          time_t start;
          time_t end;
          CDateTime::GetUTCDateTime().GetAsTime(start); // NOTE: Client EPG times must be UTC based
          CDateTime::GetUTCDateTime().GetAsTime(end);

          start -= lingertime;
          end   += daysToLoad;

          if (PVRChannelsTV[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsTV[i].ClientID())->SupportEPG && clients->find(PVRChannelsTV[i].ClientID())->second->ReadyToUse())
          {
            ret = clients->find(PVRChannelsTV[i].ClientID())->second->GetEPGForChannel(PVRChannelsTV[i], p, start, end) == PVR_ERROR_NO_ERROR ? true : false;
            if (ignoreDB)
              ret = false;  // This prevent the save of the loaded data inside the Database
          }
          else
          {
            if (PVRChannelsTV[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].ChannelName().c_str());
              return;
            }
            /// TODO: Add Support for Web EPG Scrapers here
            ret = false;
          }

          if (ret)
          {
            for (unsigned int j = 0; j < p->InfoTags()->size(); j++)
              database->UpdateEPGEntry(*p->InfoTags()->at(j), false, (j==0), (j >= p->InfoTags()->size()-1));

            PVRChannelsTV[i].ResetChannelEPGLinks();
          }
          if (p->InfoTags()->size() > 0)
            CLog::Log(LOGINFO, "PVR: Refreshed '%i' EPG events for TV Channel '%s' during Scan", (int)p->InfoTags()->size(), PVRChannelsTV[i].ChannelName().c_str());
        }
        else
        {
          time_t DataLastTime;
          unsigned int cntEntries = p->InfoTags()->size();

          if (PVRChannelsTV[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsTV[i].ClientID())->SupportEPG && clients->find(PVRChannelsTV[i].ClientID())->second->ReadyToUse())
          {
            if (ignoreDB)
            {
              p->GetLastEPGDate().GetAsTime(DataLastTime);
              clients->find(PVRChannelsTV[i].ClientID())->second->GetEPGForChannel(PVRChannelsTV[i], p, DataLastTime, endLoad);
              ret = false;  // This prevent the save of the loaded data inside the Database
            }
            else
            {
              database->GetEPGDataEnd(PVRChannelsTV[i].ChannelID()).GetAsTime(DataLastTime);
              ret = clients->find(PVRChannelsTV[i].ClientID())->second->GetEPGForChannel(PVRChannelsTV[i], p, DataLastTime, 0, true) == PVR_ERROR_NO_ERROR ? true : false;
            }
          }
          else
          {
            if (PVRChannelsTV[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].ChannelName().c_str());
              return;
            }
            /// TODO: Add Support for Web EPG Scrapers here
            ret = false;
          }

          if (ret)
          {
            p->GetLastEPGDate().GetAsTime(DataLastTime);
            database->GetEPGForChannel(PVRChannelsTV[i], p, DataLastTime, endLoad);
            PVRChannelsTV[i].ResetChannelEPGLinks();
          }
          if (p->InfoTags()->size()-cntEntries > 0)
            CLog::Log(LOGINFO, "PVR: Added '%u' EPG events for TV Channel '%s' during Update", p->InfoTags()->size()-cntEntries, PVRChannelsTV[i].ChannelName().c_str());
        }

        /// This will check all programs in the list and
        /// will remove any overlapping programs
        /// An overlapping program is a tv program which overlaps with another tv program in time
        /// for example.
        ///   program A on MTV runs from 20.00-21.00 on 1 november 2004
        ///   program B on MTV runs from 20.55-22.00 on 1 november 2004
        ///   this case, program B will be removed
        p->Sort();
        CStdString previousName = "";
        CDateTime previousStart;
        CDateTime previousEnd(1980, 1, 1, 0, 0, 0);
        for (unsigned int j = 0; j < p->InfoTags()->size(); j++)
        {
          if (previousEnd > p->InfoTags()->at(j)->Start())
          {
            //remove this program
            CLog::Log(LOGNOTICE, "PVR: Removing Overlapped TV Event '%s' on channel '%s' at date '%s' to '%s'",
                                 p->InfoTags()->at(j)->Title().c_str(),
                                 p->InfoTags()->at(j)->ChannelTag()->ChannelName().c_str(),
                                 p->InfoTags()->at(j)->Start().GetAsLocalizedDateTime(false, false).c_str(),
                                 p->InfoTags()->at(j)->End().GetAsLocalizedDateTime(false, false).c_str());
            CLog::Log(LOGNOTICE, "     Overlapps with '%s' at date '%s' to '%s'",
                                 previousName.c_str(),
                                 previousStart.GetAsLocalizedDateTime(false, false).c_str(),
                                 previousEnd.GetAsLocalizedDateTime(false, false).c_str());
            database->RemoveEPGEntry(*p->InfoTags()->at(j));
            p->DelInfoTag(p->InfoTags()->at(j));
          }
          else
          {
            previousName = p->InfoTags()->at(j)->Title();
            previousStart = p->InfoTags()->at(j)->Start();
            previousEnd = p->InfoTags()->at(j)->End();
          }
        }
      }
      p->SetUpdate(false);
    }
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    if (m_bInihibitUpdate)
      return;

    CSingleLock lock(m_critSection);

    if (PVRChannelsRadio[i].GrabEpg())
    {
      CPVREpg *p = AddEPG(PVRChannelsRadio[i].ChannelID());
      if (p)
      {
        bool ret = false;

        p->SetUpdate(true);

        if (Scan)
        {
          time_t start;
          time_t end;
          CDateTime::GetUTCDateTime().GetAsTime(start); // NOTE: Client EPG times must be UTC based
          CDateTime::GetUTCDateTime().GetAsTime(end);
          start -= lingertime;
          end   += daysToLoad;

          if (PVRChannelsRadio[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsRadio[i].ClientID())->SupportEPG && clients->find(PVRChannelsRadio[i].ClientID())->second->ReadyToUse())
          {
            ret = clients->find(PVRChannelsRadio[i].ClientID())->second->GetEPGForChannel(PVRChannelsRadio[i], p, start, end) == PVR_ERROR_NO_ERROR ? true : false;
            if (ignoreDB)
              ret = false;  // This prevent the save of the loaded data inside the Database
          }
          else
          {
            if (PVRChannelsRadio[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].ChannelName().c_str());
              return;
            }
            /// TODO: Add Support for Web EPG Scrapers here
            ret = false;
          }

          if (ret)
          {
            for (unsigned int j = 0; j < p->InfoTags()->size(); j++)
              database->UpdateEPGEntry(*p->InfoTags()->at(j), false, (j==0), (j >= p->InfoTags()->size()-1));

            PVRChannelsRadio[i].ResetChannelEPGLinks();
          }
          if (p->InfoTags()->size() > 0)
            CLog::Log(LOGINFO, "PVR: Refreshed '%i' EPG events for Radio Channel '%s' during Scan", (int)p->InfoTags()->size(), PVRChannelsRadio[i].ChannelName().c_str());
        }
        else
        {
          time_t DataLastTime;
          unsigned int cntEntries = p->InfoTags()->size();

          if (PVRChannelsRadio[i].Grabber() == "client" && g_PVRManager.GetClientProps(PVRChannelsRadio[i].ClientID())->SupportEPG && clients->find(PVRChannelsRadio[i].ClientID())->second->ReadyToUse())
          {
            if (ignoreDB)
            {
              p->GetLastEPGDate().GetAsTime(DataLastTime);
              clients->find(PVRChannelsRadio[i].ClientID())->second->GetEPGForChannel(PVRChannelsRadio[i], p, DataLastTime, endLoad);
              ret = false;  // This prevent the save of the loaded data inside the Database
            }
            else
            {
              database->GetEPGDataEnd(PVRChannelsRadio[i].ChannelID()).GetAsTime(DataLastTime);
              ret = clients->find(PVRChannelsRadio[i].ClientID())->second->GetEPGForChannel(PVRChannelsRadio[i], p, DataLastTime, 0, true) == PVR_ERROR_NO_ERROR ? true : false;
            }
          }
          else
          {
            if (PVRChannelsRadio[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].ChannelName().c_str());
              return;
            }
            /// TODO: Add Support for Web EPG Scrapers here
            ret = false;
          }

          if (ret)
          {
            p->GetLastEPGDate().GetAsTime(DataLastTime);
            database->GetEPGForChannel(PVRChannelsRadio[i], p, DataLastTime, endLoad);
            PVRChannelsRadio[i].ResetChannelEPGLinks();
          }

          if (p->InfoTags()->size()-cntEntries > 0)
            CLog::Log(LOGINFO, "PVR: Added '%d' EPG events for Radio Channel '%s' during Update", p->InfoTags()->size()-cntEntries, PVRChannelsRadio[i].ChannelName().c_str());
        }

        /// This will check all programs in the list and
        /// will remove any overlapping programs
        /// An overlapping program is a tv program which overlaps with another tv program in time
        /// for example.
        ///   program A on MTV runs from 20.00-21.00 on 1 november 2004
        ///   program B on MTV runs from 20.55-22.00 on 1 november 2004
        ///   this case, program B will be removed
        p->Sort();
        CStdString previousName = "";
        CDateTime previousStart;
        CDateTime previousEnd(1980, 1, 1, 0, 0, 0);
        for (unsigned int j = 0; j < p->InfoTags()->size(); j++)
        {
          if (previousEnd > p->InfoTags()->at(j)->Start())
          {
            //remove this program
            CLog::Log(LOGNOTICE, "PVR: Removing Overlapped Radio Event '%s' on channel '%s' at date '%s' to '%s'",
                                 p->InfoTags()->at(j)->Title().c_str(),
                                 p->InfoTags()->at(j)->ChannelTag()->ChannelName().c_str(),
                                 p->InfoTags()->at(j)->Start().GetAsLocalizedDateTime(false, false).c_str(),
                                 p->InfoTags()->at(j)->End().GetAsLocalizedDateTime(false, false).c_str());
            CLog::Log(LOGNOTICE, "     Overlapps with '%s' at date '%s' to '%s'",
                                 previousName.c_str(),
                                 previousStart.GetAsLocalizedDateTime(false, false).c_str(),
                                 previousEnd.GetAsLocalizedDateTime(false, false).c_str());
            database->RemoveEPGEntry(*p->InfoTags()->at(j));
            p->DelInfoTag(p->InfoTags()->at(j));
          }
          else
          {
            previousName = p->InfoTags()->at(j)->Title();
            previousStart = p->InfoTags()->at(j)->Start();
            previousEnd = p->InfoTags()->at(j)->End();
          }
        }
      }
      p->SetUpdate(false);
    }
  }

  if (Scan)
    database->UpdateLastEPGScan(CDateTime::GetCurrentDateTime());
  database->Close();
  CLog::Log(LOGNOTICE, "PVR: EPG update finished after %li.%li seconds", (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);

  return;
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
    if (!Epg || Epg->IsUpdateRunning() && Epg->IsValid())
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
    if (!Epg || Epg->IsUpdateRunning() && Epg->IsValid())
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

CPVREpg *CPVREpgs::AddEPG(long ChannelID)
{
  CPVREpg *p = (CPVREpg *)GetEPG(ChannelID);
  if (!p)
  {
    p = new CPVREpg(ChannelID);
    Add(p);
    CPVRChannel *channel = CPVRChannels::GetByChannelIDFromAll(ChannelID);
    if (channel)
      channel->m_Epg = p;
  }
  return p;
}

const CPVREpg *CPVREpgs::GetEPG(long ChannelID) const
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->ChannelID() == ChannelID)
      return at(i);
  }
  return NULL;
}

const CPVREpg *CPVREpgs::GetEPG(const CPVRChannel *Channel, bool AddIfMissing) const
{
  // This is not very beautiful, but it dramatically speeds up the
  // "What's on now/next?" menus.
  static CPVREpg DummyEPG(-1);

  if (!Channel->m_Epg)
     Channel->m_Epg = GetEPG(Channel->ChannelID());

  if (!Channel->m_Epg)
     Channel->m_Epg = &DummyEPG;

  if (Channel->m_Epg == &DummyEPG && AddIfMissing)
  {
    CPVREpg *Epg = new CPVREpg(Channel->ChannelID());
    ((CPVREpgs *)this)->Add(Epg);
    Channel->m_Epg = Epg;
  }
  return Channel->m_Epg != &DummyEPG ? Channel->m_Epg: NULL;
}

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

void CPVREpgs::Add(CPVREpg *entry)
{
  push_back(entry);
}

