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
#include "PVREpg.h"
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

/// ----- EPGSearchFilter -------------------------------------------------------------------------

void EPGSearchFilter::SetDefaults()
{
  m_SearchString      = "";
  m_CaseSensitive     = false;
  m_SearchDescription = false;
  m_GenreType         = -1;
  m_GenreSubType      = -1;
  m_minDuration       = -1;
  m_maxDuration       = -1;
  PVREpgs.GetFirstEPGDate().GetAsSystemTime(m_startDate);
  m_startDate.wHour   = 0;
  m_startDate.wMinute = 0;
  PVREpgs.GetLastEPGDate().GetAsSystemTime(m_endDate);
  m_endDate.wHour     = 23;
  m_endDate.wMinute   = 59;
  m_startTime         = m_startDate;
  m_startTime.wHour   = 0;
  m_startTime.wMinute = 0;
  m_endTime           = m_endDate;
  m_endTime.wHour     = 23;
  m_endTime.wMinute   = 59;
  m_ChannelNumber     = -1;
  m_FTAOnly           = false;
  m_IncUnknGenres     = true;
  m_Group             = -1;
  m_IgnPresentTimers  = true;
  m_IgnPresentRecords = true;
  m_PreventRepeats    = false;
}

bool EPGSearchFilter::FilterEntry(const cPVREPGInfoTag &tag) const
{
  if (m_GenreType != -1)
  {
    if (tag.GenreType() != m_GenreType &&
        (!m_IncUnknGenres &&
        ((tag.GenreType() < EVCONTENTMASK_USERDEFINED || tag.GenreType() >= EVCONTENTMASK_MOVIEDRAMA))))
    {
      return false;
    }
  }
  if (m_minDuration != -1)
  {
    if (tag.GetDuration() < (m_minDuration*60))
      return false;
  }
  if (m_maxDuration != -1)
  {
    if (tag.GetDuration() > (m_maxDuration*60))
      return false;
  }
  if (m_ChannelNumber != -1)
  {
    if (m_ChannelNumber == -2)
    {
      if (tag.IsRadio())
        return false;
    }
    else if (m_ChannelNumber == -3)
    {
      if (!tag.IsRadio())
        return false;
    }
    else if (tag.ChannelNumber() != m_ChannelNumber)
      return false;
  }
  if (m_FTAOnly && tag.IsEncrypted())
  {
    return false;
  }
  if (m_Group != -1)
  {
    if (tag.GroupID() != m_Group)
      return false;
  }

  int timeTag = (tag.Start().GetHour()*60 + tag.Start().GetMinute());

  if (timeTag < (m_startTime.wHour*60 + m_startTime.wMinute))
    return false;

  if (timeTag > (m_endTime.wHour*60 + m_endTime.wMinute))
    return false;

  if (tag.Start() < m_startDate)
    return false;

  if (tag.Start() > m_endDate)
    return false;

  if (m_SearchString != "")
  {
    cTextSearch search(tag.Title(), m_SearchString, m_CaseSensitive);
    if (!search.DoSearch())
    {
      if (m_SearchDescription)
      {
        search.SetText(tag.PlotOutline(), m_SearchString, m_CaseSensitive);
        if (!search.DoSearch())
        {
          search.SetText(tag.Plot(), m_SearchString, m_CaseSensitive);
          if (!search.DoSearch())
            return false;
        }
      }
      else
        return false;
    }
  }
  return true;
}


/// ----- cPVREPGInfoTag --------------------------------------------------------------------------

cPVREPGInfoTag::cPVREPGInfoTag(long uniqueBroadcastID)
{
  Reset();
  m_uniqueBroadcastID = uniqueBroadcastID;
}

void cPVREPGInfoTag::Reset()
{
  m_strTitle            = g_localizeStrings.Get(19055);
  m_strGenre            = "";
  m_strPlotOutline      = "";
  m_strPlot             = "";
  m_GenreType           = 0;
  m_GenreSubType        = 0;
  m_strFileNameAndPath  = "";
  m_IconPath            = "";
  m_isRecording         = false;
  m_Timer               = NULL;
  m_Epg                 = NULL;
  m_parentalRating      = 0;
  m_starRating          = 0;
  m_notify              = false;
  m_seriesNum           = "";
  m_episodeNum          = "";
  m_episodePart         = "";
  m_episodeName         = "";
}

bool cPVREPGInfoTag::HasTimer(void) const
{
  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
  {
    if (PVRTimers[i].Epg() == this)
      return true;
  }
  return false;
}

int cPVREPGInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start;
}

void cPVREPGInfoTag::SetGenre(int ID, int subID)
{
  m_GenreType    = ID;
  m_GenreSubType = subID;
  m_strGenre     = ConvertGenreIdToString(ID, subID);
}

CStdString cPVREPGInfoTag::ConvertGenreIdToString(int ID, int subID) const
{
  CStdString str = g_localizeStrings.Get(19499);
  switch (ID)
  {
    case EVCONTENTMASK_MOVIEDRAMA:
      if (subID <= 8)
        str = g_localizeStrings.Get(19500 + subID);
      else
        str = g_localizeStrings.Get(19500) + " (undefined)";
      break;
    case EVCONTENTMASK_NEWSCURRENTAFFAIRS:
      if (subID <= 4)
        str = g_localizeStrings.Get(19516 + subID);
      else
        str = g_localizeStrings.Get(19516) + " (undefined)";
      break;
    case EVCONTENTMASK_SHOW:
      if (subID <= 3)
        str = g_localizeStrings.Get(19532 + subID);
      else
        str = g_localizeStrings.Get(19532) + " (undefined)";
      break;
    case EVCONTENTMASK_SPORTS:
      if (subID <= 0x0B)
        str = g_localizeStrings.Get(19548 + subID);
      else
        str = g_localizeStrings.Get(19548) + " (undefined)";
      break;
    case EVCONTENTMASK_CHILDRENYOUTH:
      if (subID <= 5)
        str = g_localizeStrings.Get(19564 + subID);
      else
        str = g_localizeStrings.Get(19564) + " (undefined)";
      break;
    case EVCONTENTMASK_MUSICBALLETDANCE:
      if (subID <= 6)
        str = g_localizeStrings.Get(19580 + subID);
      else
        str = g_localizeStrings.Get(19580) + " (undefined)";
      break;
    case EVCONTENTMASK_ARTSCULTURE:
      if (subID <= 0x0B)
        str = g_localizeStrings.Get(19596 + subID);
      else
        str = g_localizeStrings.Get(19596) + " (undefined)";
      break;
    case EVCONTENTMASK_SOCIALPOLITICALECONOMICS:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19612 + subID);
      else
        str = g_localizeStrings.Get(19612) + " (undefined)";
      break;
    case EVCONTENTMASK_EDUCATIONALSCIENCE:
      if (subID <= 0x07)
        str = g_localizeStrings.Get(19628 + subID);
      else
        str = g_localizeStrings.Get(19628) + " (undefined)";
      break;
    case EVCONTENTMASK_LEISUREHOBBIES:
      if (subID <= 0x07)
        str = g_localizeStrings.Get(19644 + subID);
      else
        str = g_localizeStrings.Get(19644) + " (undefined)";
      break;
    case EVCONTENTMASK_SPECIAL:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19660 + subID);
      else
        str = g_localizeStrings.Get(19660) + " (undefined)";
      break;
    case EVCONTENTMASK_USERDEFINED:
      if (subID <= 0x03)
        str = g_localizeStrings.Get(19676 + subID);
      else
        str = g_localizeStrings.Get(19676) + " (undefined)";
      break;
    default:
      break;
  }
  return str;
}


/// ----- cPVREpg ---------------------------------------------------------------------------------

struct sortEPGbyDate
{
  bool operator()(cPVREPGInfoTag* strItem1, cPVREPGInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->Start() < strItem2->Start();
  }
};

cPVREpg::cPVREpg(long ChannelID)
{
  m_channelID       = ChannelID;
  m_Channel         = cPVRChannels::GetByChannelIDFromAll(ChannelID);
  m_bUpdateRunning  = false;
  m_bValid          = ChannelID == -1 ? false : true;
}

bool cPVREpg::IsValid(void) const
{
  if (!m_bValid || m_tags.size() == 0)
    return false;

  if (m_tags[m_tags.size()-1]->m_endTime < CDateTime::GetCurrentDateTime())
    return false;

  return true;
}

cPVREPGInfoTag *cPVREpg::AddInfoTag(cPVREPGInfoTag *Tag)
{
  m_tags.push_back(Tag);
  return Tag;
}

void cPVREpg::DelInfoTag(cPVREPGInfoTag *tag)
{
  if (tag->m_Epg == this)
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      cPVREPGInfoTag *entry = m_tags[i];
      if (entry == tag)
      {
        delete entry;
        m_tags.erase(m_tags.begin()+i);
        return;
      }
    }
  }
}

void cPVREpg::Sort(void)
{
  sort(m_tags.begin(), m_tags.end(), sortEPGbyDate());
}

void cPVREpg::Cleanup(void)
{
  Cleanup(CDateTime::GetCurrentDateTime());
}

void cPVREpg::Cleanup(CDateTime Time)
{
  m_bUpdateRunning = true;
  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    cPVREPGInfoTag *tag = m_tags[i];
    if (!tag->HasTimer() && (tag->End()+CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60 + 1, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0) < Time)) // adding one hour for safety
    {
      DelInfoTag(tag);
    }
    else
      break;
  }
  m_bUpdateRunning = false;
}

const cPVREPGInfoTag *cPVREpg::GetInfoTagNow(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (m_tags.size() == 0)
    return NULL;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= now) && (m_tags[i]->End() > now))
      return m_tags[i];
  }
  return NULL;
}

const cPVREPGInfoTag *cPVREpg::GetInfoTagNext(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (m_tags.size() == 0)
    return false;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= now) && (m_tags[i]->End() > now))
    {
      CDateTime next = m_tags[i]->End();

      for (unsigned int j = 0; j < m_tags.size(); j++)
      {
        if (m_tags[j]->Start() >= next)
        {
          return m_tags[j];
        }
      }
    }
  }

  return NULL;
}

const cPVREPGInfoTag *cPVREpg::GetInfoTag(long uniqueID, CDateTime StartTime) const
{
  if (uniqueID > 0)
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      if (m_tags[i]->GetUniqueBroadcastID() == uniqueID)
        return m_tags[i];
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      if (m_tags[i]->Start() == StartTime)
        return m_tags[i];
    }
  }

  return NULL;
}

const cPVREPGInfoTag *cPVREpg::GetInfoTagAround(CDateTime Time) const
{
  if (m_tags.size() == 0)
    return NULL;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= Time) && (m_tags[i]->End() >= Time))
      return m_tags[i];
  }
  return NULL;
}

CDateTime cPVREpg::GetLastEPGDate()
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if (m_tags[i]->End() >= last)
      last = m_tags[i]->End();
  }
  return last;
}

bool cPVREpg::Add(const PVR_PROGINFO *data, cPVREpg *Epg)
{
  if (Epg && data)
  {
    cPVREPGInfoTag *InfoTag     = NULL;
    cPVREPGInfoTag *newInfoTag  = NULL;
    long uniqueBroadcastID      = data->uid;

    InfoTag = (cPVREPGInfoTag *)Epg->GetInfoTag(uniqueBroadcastID, data->starttime);
    if (!InfoTag)
      InfoTag = newInfoTag = new cPVREPGInfoTag(uniqueBroadcastID);

    if (InfoTag)
    {
      CStdString path;
      path.Format("pvr://guide/channel-%04i/%s.epg", Epg->m_Channel->Number(), InfoTag->Start().GetAsDBDateTime().c_str());
      InfoTag->SetPath(path);
      InfoTag->SetStart((time_t)data->starttime);
      InfoTag->SetEnd((time_t)data->endtime);
      InfoTag->SetTitle(data->title);
      InfoTag->SetPlotOutline(data->subtitle);
      InfoTag->SetPlot(data->description);
      InfoTag->SetGenre(data->genre_type, data->genre_sub_type);
      InfoTag->SetParentalRating(data->parental_rating);
      InfoTag->SetIcon(Epg->m_Channel->Icon());
      InfoTag->m_Epg = Epg;

      if (newInfoTag)
        Epg->AddInfoTag(newInfoTag);

      return true;
    }
  }
  return false;
}

bool cPVREpg::AddDB(const PVR_PROGINFO *data, cPVREpg *Epg)
{
  if (Epg && data)
  {
    CTVDatabase *database = g_PVRManager.GetTVDatabase();
    cPVREPGInfoTag InfoTag(data->uid);

    /// NOTE: Database is already opened by the EPG Update function
    CStdString path;
    path.Format("pvr://guide/channel-%04i/%s.epg", Epg->m_Channel->Number(), InfoTag.Start().GetAsDBDateTime().c_str());
    InfoTag.SetPath(path);
    InfoTag.SetStart(CDateTime((time_t)data->starttime));
    InfoTag.SetEnd(CDateTime((time_t)data->endtime));
    InfoTag.SetTitle(data->title);
    InfoTag.SetPlotOutline(data->subtitle);
    InfoTag.SetPlot(data->description);
    InfoTag.SetGenre(data->genre_type, data->genre_sub_type);
    InfoTag.SetParentalRating(data->parental_rating);
    InfoTag.SetIcon(Epg->m_Channel->Icon());
    InfoTag.m_Epg = Epg;

    return database->UpdateEPGEntry(InfoTag);
  }
  return false;
}


/// ----- cPVREpgs --------------------------------------------------------------------------------

cPVREpgs PVREpgs;

cPVREpgs::cPVREpgs(void)
{
}

void cPVREpgs::Cleanup()
{
  CLog::Log(LOGINFO, "PVR: cleaning up epg data");

  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    cPVREpg *Epg = (cPVREpg *) GetEPG(&PVRChannelsTV[i], true);
    Epg->Cleanup(CDateTime::GetCurrentDateTime());
  }

  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    cPVREpg *Epg = (cPVREpg *) GetEPG(&PVRChannelsRadio[i], true);
    Epg->Cleanup(CDateTime::GetCurrentDateTime());
  }
}

bool cPVREpgs::ClearAll(void)
{
  CSingleLock lock(m_critSection);

  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
    PVRTimers[i].SetEpg(NULL);
  for (unsigned int i = 0; i < size(); i++)
    at(i)->Cleanup(-1);

  return true;
}

bool cPVREpgs::ClearChannel(long ChannelID)
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

void cPVREpgs::Load()
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
      scanner->SetTitle(PVRChannelsTV[i].Name());
      scanner->UpdateState();
      cPVREpg *p = AddEPG(PVRChannelsTV[i].ChannelID());
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
              CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for TV Channel '%s', loaded from client '%li'", PVRChannelsTV[i].Name().c_str(), PVRChannelsTV[i].ClientID());
            else
              CLog::Log(LOGDEBUG, "PVR: TV Channel '%s', on client '%li' contains no EPG data", PVRChannelsTV[i].Name().c_str(), PVRChannelsTV[i].ClientID());
          }
          else
          {
            if (PVRChannelsTV[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].Name().c_str());
              return;
            }
            CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for TV Channel '%s', loading with scraper '%s'", PVRChannelsTV[i].Name().c_str(), PVRChannelsTV[i].Grabber().c_str());
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
      scanner->SetTitle(PVRChannelsRadio[i].Name());
      scanner->UpdateState();
      cPVREpg *p = AddEPG(PVRChannelsRadio[i].ChannelID());
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
              CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for Radio Channel '%s', loaded from client '%li'", PVRChannelsRadio[i].Name().c_str(), PVRChannelsRadio[i].ClientID());
            else
              CLog::Log(LOGDEBUG, "PVR: Radio Channel '%s', on client '%li' contains no EPG data", PVRChannelsRadio[i].Name().c_str(), PVRChannelsRadio[i].ClientID());
          }
          else
          {
            if (PVRChannelsRadio[i].Grabber().IsEmpty())
            {
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].Name().c_str());
              return;
            }
            CLog::Log(LOGINFO, "PVR: TV Database contains no EPG data for Radio Channel '%s', loading with scraper '%s'", PVRChannelsRadio[i].Name().c_str(), PVRChannelsRadio[i].Grabber().c_str());
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

void cPVREpgs::Unload()
{
  CSingleLock lock(m_critSection);

  CLog::Log(LOGINFO, "PVR: Unloading EPG information");

  m_bInihibitUpdate = true;

  for (unsigned int i = 0; i < size(); i++)
  {
    cPVREpg *p = at(i);
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

void cPVREpgs::Update(bool Scan)
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
      cPVREpg *p = AddEPG(PVRChannelsTV[i].ChannelID());
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
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].Name().c_str());
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
            CLog::Log(LOGINFO, "PVR: Refreshed '%i' EPG events for TV Channel '%s' during Scan", (int)p->InfoTags()->size(), PVRChannelsTV[i].Name().c_str());
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
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for TV Channel '%s'", PVRChannelsTV[i].Name().c_str());
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
            CLog::Log(LOGINFO, "PVR: Added '%lu' EPG events for TV Channel '%s' during Update", p->InfoTags()->size()-cntEntries, PVRChannelsTV[i].Name().c_str());
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
                                 p->InfoTags()->at(j)->ChannelName().c_str(),
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
      cPVREpg *p = AddEPG(PVRChannelsRadio[i].ChannelID());
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
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].Name().c_str());
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
            CLog::Log(LOGINFO, "PVR: Refreshed '%i' EPG events for Radio Channel '%s' during Scan", (int)p->InfoTags()->size(), PVRChannelsRadio[i].Name().c_str());
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
              CLog::Log(LOGERROR, "PVR: No EPG grabber defined for Radio Channel '%s'", PVRChannelsRadio[i].Name().c_str());
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
            CLog::Log(LOGINFO, "PVR: Added '%lu' EPG events for Radio Channel '%s' during Update", p->InfoTags()->size()-cntEntries, PVRChannelsRadio[i].Name().c_str());
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
                                 p->InfoTags()->at(j)->ChannelName().c_str(),
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

int cPVREpgs::GetEPGAll(CFileItemList* results, bool radio)
{
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_hide || !ch->at(i).m_grabEpg)
      continue;

    const cPVREpg *Epg = GetEPG(&ch->at(i), false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();
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

int cPVREpgs::GetEPGSearch(CFileItemList* results, const EPGSearchFilter &filter)
{
  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    if (PVRChannelsTV[i].m_hide || !PVRChannelsTV[i].m_grabEpg)
      continue;

    const cPVREpg *Epg = GetEPG(&PVRChannelsTV[i], false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();

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
    if (PVRChannelsRadio[i].m_hide || !PVRChannelsRadio[i].m_grabEpg)
      continue;

    const cPVREpg *Epg = GetEPG(&PVRChannelsRadio[i], false);
    if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
    {
      const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();

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

  if (filter.m_IgnPresentRecords && PVRRecordings.size() > 0)
  {
    for (unsigned int i = 0; i < PVRRecordings.size(); i++)
    {
      for (int j = 0; j < results->Size(); j++)
      {
        const cPVREPGInfoTag *epgentry = results->Get(j)->GetEPGInfoTag();
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

  if (filter.m_IgnPresentTimers && PVRTimers.size() > 0)
  {
    for (unsigned int i = 0; i < PVRTimers.size(); i++)
    {
      for (int j = 0; j < results->Size(); j++)
      {
        const cPVREPGInfoTag *epgentry = results->Get(j)->GetEPGInfoTag();
        if (epgentry)
        {
          if (epgentry->ChannelNumber() != PVRTimers[i].Number())
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

  if (filter.m_PreventRepeats)
  {
    unsigned int size = results->Size();
    for (unsigned int i = 0; i < size; i++)
    {
      const cPVREPGInfoTag *epgentry_1 = results->Get(i)->GetEPGInfoTag();
      for (unsigned int j = 0; j < size; j++)
      {
        const cPVREPGInfoTag *epgentry_2 = results->Get(j)->GetEPGInfoTag();
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

int cPVREpgs::GetEPGChannel(unsigned int number, CFileItemList* results, bool radio)
{
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  if (!ch->at(number-1).m_grabEpg)
    return 0;

  const cPVREpg *Epg = GetEPG(&ch->at(number-1), true);
  if (Epg && !Epg->IsUpdateRunning() && Epg->IsValid())
  {
    const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();
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

int cPVREpgs::GetEPGNow(CFileItemList* results, bool radio)
{
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_hide || !ch->at(i).m_grabEpg)
      continue;

    const cPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (!Epg || Epg->IsUpdateRunning() && Epg->IsValid())
      continue;

    const cPVREPGInfoTag *epgnow = Epg->GetInfoTagNow();
    if (!epgnow)
      continue;

    CFileItemPtr entry(new CFileItem(*epgnow));
    entry->SetLabel2(epgnow->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = ch->at(i).Name();
    entry->SetThumbnailImage(ch->at(i).Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size();
}

int cPVREpgs::GetEPGNext(CFileItemList* results, bool radio)
{
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_hide || !ch->at(i).GrabEpg())
      continue;

    const cPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (!Epg || Epg->IsUpdateRunning() && Epg->IsValid())
      continue;

    const cPVREPGInfoTag *epgnext = Epg->GetInfoTagNext();
    if (!epgnext)
      continue;

    CFileItemPtr entry(new CFileItem(*epgnext));
    entry->SetLabel2(epgnext->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = ch->at(i).Name();
    entry->SetThumbnailImage(ch->at(i).Icon());
    results->Add(entry);
  }
  SetVariableData(results);

  return results->Size();
}

cPVREpg *cPVREpgs::AddEPG(long ChannelID)
{
  cPVREpg *p = (cPVREpg *)GetEPG(ChannelID);
  if (!p)
  {
    p = new cPVREpg(ChannelID);
    Add(p);
    cPVRChannelInfoTag *channel = cPVRChannels::GetByChannelIDFromAll(ChannelID);
    if (channel)
      channel->m_Epg = p;
  }
  return p;
}

const cPVREpg *cPVREpgs::GetEPG(long ChannelID) const
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->ChannelID() == ChannelID)
      return at(i);
  }
  return NULL;
}

const cPVREpg *cPVREpgs::GetEPG(const cPVRChannelInfoTag *Channel, bool AddIfMissing) const
{
  // This is not very beautiful, but it dramatically speeds up the
  // "What's on now/next?" menus.
  static cPVREpg DummyEPG(-1);

  if (!Channel->m_Epg)
     Channel->m_Epg = GetEPG(Channel->ChannelID());

  if (!Channel->m_Epg)
     Channel->m_Epg = &DummyEPG;

  if (Channel->m_Epg == &DummyEPG && AddIfMissing)
  {
    cPVREpg *Epg = new cPVREpg(Channel->ChannelID());
    ((cPVREpgs *)this)->Add(Epg);
    Channel->m_Epg = Epg;
  }
  return Channel->m_Epg != &DummyEPG ? Channel->m_Epg: NULL;
}

CDateTime cPVREpgs::GetFirstEPGDate(bool radio/* = false*/)
{
  CDateTime first = CDateTime::GetCurrentDateTime();
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_hide || !ch->at(i).GrabEpg())
      continue;

    const cPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (Epg->IsUpdateRunning())
      continue;

    const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();

    for (unsigned int j = 0; j < ch_epg->size(); j++)
    {
      if (ch_epg->at(j)->Start() < first)
        first = ch_epg->at(j)->Start();
    }
  }

  return first;
}

CDateTime cPVREpgs::GetLastEPGDate(bool radio/* = false*/)
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;

  for (unsigned int i = 0; i < ch->size(); i++)
  {
    if (ch->at(i).m_hide)
      continue;

    const cPVREpg *Epg = GetEPG(&ch->at(i), true);
    if (Epg->IsUpdateRunning())
      continue;

    const vector<cPVREPGInfoTag*> *ch_epg = Epg->InfoTags();

    for (unsigned int j = 0; j < ch_epg->size(); j++)
    {
      if (ch_epg->at(j)->End() >= last)
        last = ch_epg->at(j)->End();
    }
  }
  return last;
}

void cPVREpgs::SetVariableData(CFileItemList* results)
{
  /* Reload Timers */
  PVRTimers.Update();

  /* Clear first all Timers set inside the EPG tags */
  for (int j = 0; j < results->Size(); j++)
  {
    cPVREPGInfoTag *epg = results->Get(j)->GetEPGInfoTag();
    if (epg)
      epg->SetTimer(NULL);
  }

  /* Now go with the timers thru the EPG and set the Timer Tag for every matching item */
  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    for (int j = 0; j < results->Size(); j++)
    {
      cPVREPGInfoTag *epg = results->Get(j)->GetEPGInfoTag();
      if (epg)
      {
        if (!PVRTimers[i].Active())
          continue;

        if (epg->ChannelNumber() != PVRTimers[i].Number())
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

void cPVREpgs::AssignChangedChannelTags(bool radio/* = false*/)
{
  CSingleLock lock(m_critSection);

  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  for (unsigned int i = 0; i < ch->size(); i++)
  {
    cPVREpg *Epg = (cPVREpg *) GetEPG(&ch->at(i), true);
    Epg->m_Channel = cPVRChannels::GetByChannelIDFromAll(Epg->ChannelID());
  }
}

void cPVREpgs::Add(cPVREpg *entry)
{
  push_back(entry);
}

