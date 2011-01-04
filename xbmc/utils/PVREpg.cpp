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

CPVREpg::CPVREpg(CPVRChannel *channel)
{
  m_Channel        = channel;
  m_bUpdateRunning = false;
  m_bIsSorted      = false;
}

CPVREpg::~CPVREpg(void)
{
  Clear();
}

bool CPVREpg::HasValidEntries(void) const
{
  ((CPVREpg *) this)->Sort();

  return (m_Channel &&
          m_Channel->ChannelID() > 0 && /* valid channel ID */
          size() > 0 && /* contains at least 1 tag */
          at(size()-1)->m_endTime >= CDateTime::GetCurrentDateTime()); /* the last end time hasn't passed yet */
}

bool CPVREpg::DeleteInfoTag(CPVREpgInfoTag *tag)
{
  /* check if we're the "owner" of this tag */
  if (tag->m_Epg != this)
    return false;

  /* remove the tag */
  for (unsigned int i = 0; i < size(); i++)
  {
    CPVREpgInfoTag *entry = at(i);
    if (entry == tag)
    {
      erase(begin()+i);
      m_bIsSorted = false;
      return true;
    }
  }

  return false;
}

void CPVREpg::Sort(void)
{
  /* no need to sort twice */
  if (m_bIsSorted) return;

  /* sort the EPG */
  sort(begin(), end(), sortEPGbyDate());

  /* reset the previous and next pointers on each tag */
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

void CPVREpg::Clear(void)
{
  clear();
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
    if ( tag && /* valid tag */
        !tag->HasTimer() && /* no time set */
        (tag->End() + CDateTimeSpan(0, PVREpgs.m_iLingerTime / 60 + 1, PVREpgs.m_iLingerTime % 60, 0) < Time)) /* adding one hour for safety */
    {
      DeleteInfoTag(tag);
    }
  }
  m_bUpdateRunning = false;
}

const CPVREpgInfoTag *CPVREpg::InfoTagNow(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  /* one of the first items will always match */
  for (unsigned int i = 0; i < size(); i++)
  {
    if ((at(i)->Start() <= now) && (at(i)->End() > now))
      return at(i);
  }
  return NULL;
}

const CPVREpgInfoTag *CPVREpg::InfoTagNext(void) const
{
  const CPVREpgInfoTag *nowTag = InfoTagNow();

  return nowTag ? nowTag->GetNextEvent() : NULL;
}

const CPVREpgInfoTag *CPVREpg::InfoTag(long uniqueID, CDateTime StartTime) const
{
  /* try to find the tag by UID */
  if (uniqueID > 0)
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      if (at(i)->UniqueBroadcastID() == uniqueID)
        return at(i);
    }
  }

  /* if we haven't found it, search by start time */
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->Start() == StartTime)
      return at(i);
  }

  return NULL;
}

const CPVREpgInfoTag *CPVREpg::InfoTagAround(CDateTime Time) const
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

bool CPVREpg::UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase /* = false */)
{
  if (!data)
    return false;

  long uniqueBroadcastID      = data->uid;
  CPVREpgInfoTag *InfoTag     = (CPVREpgInfoTag *) this->InfoTag(uniqueBroadcastID, data->starttime);

  /* create a new tag if no tag with this ID exists */
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

  /* update the tag's values */
  InfoTag->m_Epg = this;
  InfoTag->SetStart((time_t)data->starttime);
  InfoTag->SetEnd((time_t)data->endtime);
  InfoTag->SetTitle(data->title);
  InfoTag->SetPlotOutline(data->subtitle);
  InfoTag->SetPlot(data->description);
  InfoTag->SetGenre(data->genre_type, data->genre_sub_type);
  InfoTag->SetParentalRating(data->parental_rating);
  InfoTag->SetIcon(m_Channel->Icon());

  /* update the cached first and last date in the table */
  PVREpgs.UpdateFirstAndLastEPGDates(*InfoTag);

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

bool CPVREpg::RemoveOverlappingEvents(void)
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
      /* remove this program */
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
      DeleteInfoTag(at(ptr));
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
    CLog::Log(LOGINFO, "%s - client '%s' on client '%li' does not support EPGs",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
  }

  return bGrabSuccess;
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_Channel->Grabber().IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "%s - no EPG grabber defined for channel '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "%s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->Grabber().c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CPVREpg::Update(time_t start, time_t end, bool bLoadFromDb /* = false */, bool bStoreInDb /* = true */)
{
  if (!m_Channel)
      return false;

  bool bGrabSuccess     = true;
  CTVDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  /* check if this channel is marked for grabbing */
  if (!m_Channel->GrabEpg())
    return false;

  /* mark the EPG as being updated */
  m_bUpdateRunning = true;

  /* request the epg for this channel from the database */
  if (bLoadFromDb)
    bGrabSuccess = database->GetEPGForChannel(this, start, end);

  bGrabSuccess = (m_Channel->Grabber() == "client") ?
      UpdateFromClient(start, end) || bGrabSuccess:
      UpdateFromScraper(start, end) || bGrabSuccess;

  /* store the loaded EPG entries in the database */
  if (bGrabSuccess)
  {
    RemoveOverlappingEvents();

    if (bStoreInDb)
    {
      for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
        database->UpdateEPGEntry(*at(iTagPtr), false, (iTagPtr==0), (iTagPtr == size()-1));
    }
  }

  m_bUpdateRunning = false;

  return bGrabSuccess;
}
