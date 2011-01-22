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
#include "PVRChannelGroup.h"
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
  for (int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    delete at(iTagPtr);
  }
  erase(begin(), end());
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
        (tag->End() + CDateTimeSpan(0, g_PVREpgs.m_iLingerTime / 60 + 1, g_PVREpgs.m_iLingerTime % 60, 0) < Time)) /* adding one hour for safety */
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

const CPVREpgInfoTag *CPVREpg::InfoTagBetween(CDateTime BeginTime, CDateTime EndTime) const
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVREpgInfoTag *tag = at(ptr);
    if (tag->Start() >= BeginTime && tag->End() <= EndTime)
      return tag;
  }
  return NULL;
}

const CPVREpgInfoTag *CPVREpg::InfoTagAround(CDateTime Time) const
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVREpgInfoTag *tag = at(ptr);
    if ((tag->Start() <= Time) && (tag->End() >= Time))
      return tag;
  }
  return NULL;
}

bool CPVREpg::UpdateEntry(const CPVREpgInfoTag &tag, bool bUpdateDatabase /* = false */)
{
  CPVREpgInfoTag *InfoTag = (CPVREpgInfoTag *) this->InfoTag(tag.UniqueBroadcastID(), tag.Start());
  /* create a new tag if no tag with this ID exists */
  if (!InfoTag)
  {
    InfoTag = new CPVREpgInfoTag(tag.UniqueBroadcastID());
    if (!InfoTag)
    {
      CLog::Log(LOGERROR, "%s - Couldn't create new infotag", __FUNCTION__);
      return false;
    }
    push_back(InfoTag);
  }

  InfoTag->m_Epg = this;
  InfoTag->Update(tag);

  /* update the cached first and last date in the table */
  g_PVREpgs.UpdateFirstAndLastEPGDates(*InfoTag);

  m_bIsSorted = false;

  if (bUpdateDatabase)
  {
    bool retval;
    CPVRDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    retval = database->UpdateEpgEntry(*InfoTag);
    database->Close();
    return retval;
  }

  return true;
}

bool CPVREpg::UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase /* = false */)
{
  if (!data)
    return false;

  CPVREpgInfoTag *InfoTag = (CPVREpgInfoTag *) this->InfoTag(data->uid, data->starttime);

  /* create a new tag if no tag with this ID exists */
  if (!InfoTag)
  {
    InfoTag = new CPVREpgInfoTag(data->uid);
    if (!InfoTag)
    {
      CLog::Log(LOGERROR, "%s - Couldn't create new infotag", __FUNCTION__);
      return false;
    }
    push_back(InfoTag);
  }

  /* update the tag's values */
  InfoTag->m_Epg = this;
  InfoTag->Update(data);

  /* update the cached first and last date in the table */
  g_PVREpgs.UpdateFirstAndLastEPGDates(*InfoTag);

  m_bIsSorted = false;

  if (bUpdateDatabase)
  {
    bool retval;
    CPVRDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    retval = database->UpdateEpgEntry(*InfoTag);
    database->Close();
    return retval;
  }

  return true;
}

bool CPVREpg::FixOverlappingEvents(bool bStore /* = true */)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  Sort();

  CPVREpgInfoTag *previousTag = NULL;
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (previousTag == NULL)
    {
      previousTag = at(ptr);
      continue;
    }

    CPVREpgInfoTag *currentTag = at(ptr);

    if (previousTag->End() > currentTag->End())
    {
      /* previous tag completely overlaps current tag; delete the current tag */
      CLog::Log(LOGNOTICE, "%s - Removing EPG event '%s' on channel '%s' at '%s' to '%s': overlaps with '%s' at '%s' to '%s'",
          __FUNCTION__, currentTag->Title().c_str(), currentTag->ChannelTag()->ChannelName().c_str(),
          currentTag->Start().GetAsLocalizedDateTime(false, false).c_str(),
          currentTag->End().GetAsLocalizedDateTime(false, false).c_str(),
          previousTag->Title().c_str(),
          previousTag->Start().GetAsLocalizedDateTime(false, false).c_str(),
          previousTag->End().GetAsLocalizedDateTime(false, false).c_str());

      database->RemoveEpgEntry(*currentTag);
      DeleteInfoTag(currentTag);
      ptr--;
    }
    else if (previousTag->End() > currentTag->Start())
    {
      /* previous tag ends after the current tag starts; mediate */
      CDateTimeSpan diff = previousTag->End() - currentTag->Start();
      int iDiffSeconds = diff.GetSeconds() + diff.GetMinutes() * 60 + diff.GetHours() * 3600 + diff.GetDays() * 86400;
      CDateTime newTime = previousTag->End() - CDateTimeSpan(0, 0, 0, (int) (iDiffSeconds / 2));

      CLog::Log(LOGNOTICE, "%s - Mediating start and end times of EPG events '%s' on channel '%s' at '%s' to '%s' and '%s' at '%s' to '%s': using '%s'",
          __FUNCTION__, currentTag->Title().c_str(), currentTag->ChannelTag()->ChannelName().c_str(),
          currentTag->Start().GetAsLocalizedDateTime(false, false).c_str(),
          currentTag->End().GetAsLocalizedDateTime(false, false).c_str(),
          previousTag->Title().c_str(),
          previousTag->Start().GetAsLocalizedDateTime(false, false).c_str(),
          previousTag->End().GetAsLocalizedDateTime(false, false).c_str(),
          newTime.GetAsLocalizedDateTime(false, false).c_str());

      previousTag->SetEnd(newTime);
      currentTag->SetStart(newTime);

      if (bStore)
      {
        database->UpdateEpgEntry(*previousTag, false, false);
        database->UpdateEpgEntry(*currentTag, false, true);
      }
    }

    previousTag = at(ptr);
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
    CLog::Log(LOGINFO, "%s - client '%s' on client '%i' does not support EPGs",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
  }

  return bGrabSuccess;
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_Channel->EPGScraper().IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "%s - no EPG grabber defined for channel '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "%s - the database contains no EPG data for channel '%s', loading with scraper '%s'",
        __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->EPGScraper().c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CPVREpg::LoadFromDb()
{
  bool bReturn = false;

  if (!m_Channel)
      return bReturn;

  CPVRDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  /* check if this channel is marked for grabbing */
  if (!m_Channel->EPGEnabled())
    return bReturn;

  /* request the epg for this channel from the database */
  bReturn = (database->GetEpgForChannel(this, NULL, NULL) > 0);

  return bReturn;
}

bool CPVREpg::Update(time_t start, time_t end, bool bStoreInDb /* = true */) // XXX add locking
{
  if (!m_Channel)
      return false;

  bool bGrabSuccess     = true;
  CPVRDatabase *database = g_PVRManager.GetTVDatabase(); /* the database has already been opened */

  /* check if this channel is marked for grabbing */
  if (!m_Channel->EPGEnabled())
    return false;

  /* mark the EPG as being updated */
  m_bUpdateRunning = true;

  bGrabSuccess = (m_Channel->EPGScraper() == "client") ?
      UpdateFromClient(start, end) || bGrabSuccess:
      UpdateFromScraper(start, end) || bGrabSuccess;

  /* store the loaded EPG entries in the database */
  if (bGrabSuccess)
  {
    FixOverlappingEvents(bStoreInDb);

    if (bStoreInDb)
    {
      for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
        database->UpdateEpgEntry(*at(iTagPtr), false, (iTagPtr == size() - 1));
    }
  }

  m_bUpdateRunning = false;

  return bGrabSuccess;
}

int CPVREpg::Get(CFileItemList *results)
{
  int iInitialSize = results->Size();

  if (!HasValidEntries() || IsUpdateRunning())
    return -1;

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CFileItemPtr channel(new CFileItem(*at(iTagPtr)));
    channel->SetLabel2(at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
    results->Add(channel);
  }

  return size() - iInitialSize;
}

int CPVREpg::Get(CFileItemList *results, const PVREpgSearchFilter &filter)
{
  int iInitialSize = results->Size();

  if (!HasValidEntries() || IsUpdateRunning())
    return -1;

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    if (filter.FilterEntry(*at(iTagPtr)))
    {
      CFileItemPtr channel(new CFileItem(*at(iTagPtr)));
      channel->SetLabel2(at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
      results->Add(channel);
    }
  }

  return size() - iInitialSize;
}
