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
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "PVRChannels.h"
#include "PVRManager.h"

struct sortEPGbyDate
{
  bool operator()(CPVREpgInfoTag* strItem1, CPVREpgInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->Start() < strItem2->Start();
  }
};

CPVREpg::CPVREpg(const CPVRChannel &channel)
{
  m_Channel        = &channel;
  m_bUpdateRunning = false;
  m_bValid         = m_Channel->ChannelID() != -1;
  m_bIsSorted      = false;
}

CPVREpg::~CPVREpg()
{
  Clear();
}

bool CPVREpg::IsValid(void) const
{
  if (!m_bValid || size() == 0)
    return false;

  if (at(size()-1)->m_endTime < CDateTime::GetCurrentDateTime())
    return false;

  return true;
}

void CPVREpg::DelInfoTag(CPVREpgInfoTag *tag)
{
  if (tag->m_Epg == this)
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      CPVREpgInfoTag *entry = at(i);
      if (entry == tag)
      {
        delete entry;
        erase(begin()+i);
        m_bIsSorted = false;
        return;
      }
    }
  }
}

void CPVREpg::Sort(void)
{
  if (m_bIsSorted) return;

  sort(begin(), end(), sortEPGbyDate());

  int iTagAmount = size();

  for (int ptr = 0; ptr < iTagAmount; ptr++)
  {
    CPVREpgInfoTag *tag = at(ptr);

    if (ptr == 0)
    {
      tag->SetPreviousEvent(NULL);
    }

    if (ptr > 0)
    {
      CPVREpgInfoTag *previousTag = at(ptr-1);
      previousTag->SetNextEvent(tag);
      tag->SetPreviousEvent(previousTag);
    }

    if (ptr == iTagAmount - 1)
    {
      tag->SetNextEvent(NULL);
    }
  }
  m_bIsSorted = true;
}

void CPVREpg::Clear()
{
  m_bUpdateRunning = true;

  while (size() > 0)
  {
    CPVREpgInfoTag *tag = at(0);
    if (tag)
      delete tag;
  }

  erase(begin(), end());

  m_bUpdateRunning = false;
}

void CPVREpg::Cleanup(void)
{
  Cleanup(CDateTime::GetCurrentDateTime());
}

void CPVREpg::Cleanup(const CDateTime Time)
{
  m_bUpdateRunning = true;
  for (unsigned int i = 0; i < size(); i++)
  {
    CPVREpgInfoTag *tag = at(i);
    if (tag && !tag->HasTimer() && (tag->End()+CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60 + 1, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0) < Time)) // adding one hour for safety
    {
      DelInfoTag(tag);
    }
    else
      break;
  }
  m_bUpdateRunning = false;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagNow(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (size() == 0)
    return NULL;

  for (unsigned int i = 0; i < size(); i++)
  {
    if ((at(i)->Start() <= now) && (at(i)->End() > now))
      return at(i);
  }
  return NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagNext(void) const
{
  const CPVREpgInfoTag *nowTag = GetInfoTagNow();

  return nowTag ? nowTag->GetNextEvent() : NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTag(long uniqueID, CDateTime StartTime) const
{
  if (uniqueID > 0)
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      if (at(i)->UniqueBroadcastID() == uniqueID)
        return at(i);
    }
  }
  else
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      if (at(i)->Start() == StartTime)
        return at(i);
    }
  }

  return NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagAround(CDateTime Time) const
{
  if (size() == 0)
    return NULL;

  for (unsigned int i = 0; i < size(); i++)
  {
    if ((at(i)->Start() <= Time) && (at(i)->End() >= Time))
      return at(i);
  }
  return NULL;
}

CDateTime CPVREpg::GetLastEPGDate()
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->End() >= last)
      last = at(i)->End();
  }
  return last;
}

bool CPVREpg::Add(const PVR_PROGINFO *data, bool bUpdateDatabase /* = false */)
{
  if (data)
  {
    long uniqueBroadcastID      = data->uid;
    CPVREpgInfoTag *InfoTag     = (CPVREpgInfoTag *)GetInfoTag(uniqueBroadcastID, data->starttime);

    if (!InfoTag)
    {
      InfoTag = new CPVREpgInfoTag(uniqueBroadcastID);
      if (!InfoTag)
      {
        CLog::Log(LOGERROR, "%s - Couldn't create new infotag", __FUNCTION__);
        return false;
      }

      push_back(InfoTag);
    }

    CStdString path;
    path.Format("pvr://guide/channel-%04i/%s.epg", m_Channel->ChannelNumber(), InfoTag->Start().GetAsDBDateTime().c_str());
    InfoTag->SetPath(path);
    InfoTag->SetStart((time_t)data->starttime);
    InfoTag->SetEnd((time_t)data->endtime);
    InfoTag->SetTitle(data->title);
    InfoTag->SetPlotOutline(data->subtitle);
    InfoTag->SetPlot(data->description);
    InfoTag->SetGenre(data->genre_type, data->genre_sub_type);
    InfoTag->SetParentalRating(data->parental_rating);
    InfoTag->SetIcon(m_Channel->Icon());
    InfoTag->m_Epg = this;

    m_bIsSorted = false;

    if (bUpdateDatabase)
    {
      bool retval;
      CTVDatabase *database = g_PVRManager.GetTVDatabase();
      database->Open();
      retval = database->UpdateEPGEntry(*InfoTag);
      database->Close();
      return retval;
    }

    return true;
  }

  return false;
}

bool CPVREpg::RemoveOverlappingEvents()
{
  /// This will check all programs in the list and
  /// will remove any overlapping programs
  /// An overlapping program is a tv program which overlaps with another tv program in time
  /// for example.
  ///   program A on MTV runs from 20.00-21.00 on 1 november 2004
  ///   program B on MTV runs from 20.55-22.00 on 1 november 2004
  ///   this case, program B will be removed

  CTVDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  Sort();
  CStdString previousName = "";
  CDateTime previousStart;
  CDateTime previousEnd(1980, 1, 1, 0, 0, 0);
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (previousEnd > at(ptr)->Start())
    {
      //remove this program
      CLog::Log(LOGNOTICE, "PVR: Removing Overlapped TV Event '%s' on channel '%s' at date '%s' to '%s'",
          at(ptr)->Title().c_str(),
          at(ptr)->ChannelTag()->ChannelName().c_str(),
          at(ptr)->Start().GetAsLocalizedDateTime(false, false).c_str(),
          at(ptr)->End().GetAsLocalizedDateTime(false, false).c_str());
      CLog::Log(LOGNOTICE, "     Overlapps with '%s' at date '%s' to '%s'",
          previousName.c_str(),
          previousStart.GetAsLocalizedDateTime(false, false).c_str(),
          previousEnd.GetAsLocalizedDateTime(false, false).c_str());

      database->RemoveEPGEntry(*at(ptr));
      DelInfoTag(at(ptr));
    }
    else
    {
      previousName = at(ptr)->Title();
      previousStart = at(ptr)->Start();
      previousEnd = at(ptr)->End();
    }
  }

  return true;
}

bool CPVREpg::UpdateFromClient(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (g_PVRManager.GetClientProps(m_Channel->ClientID())->SupportEPG &&
      g_PVRManager.Clients()->find(m_Channel->ClientID())->second->ReadyToUse())
  {
    bGrabSuccess = g_PVRManager.Clients()->find(m_Channel->ClientID())->second->GetEPGForChannel(*m_Channel, this, start, end) == PVR_ERROR_NO_ERROR;
  }
  else
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - client '%s' on client '%li' does not support EPGs",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
  }

  return bGrabSuccess;
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_Channel->Grabber().IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "PVREpgs - %s - no EPG grabber defined for channel '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "PVREpgs - %s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->Grabber().c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CPVREpg::Update(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_Channel->Grabber() == "client")
  {
      bGrabSuccess = UpdateFromClient(start, end);
  }
  else
  {
      bGrabSuccess = UpdateFromScraper(start, end);
  }

  return bGrabSuccess;
}

