/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include "EpgDatabase.h"
#include "EpgContainer.h"

struct sortEPGbyDate
{
  bool operator()(CEpgInfoTag* strItem1, CEpgInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->StartAsUTC() < strItem2->StartAsUTC();
  }
};

CEpg::CEpg(int iEpgID, const CStdString &strName /* = "" */, const CStdString &strScraperName /* = "" */)
{
  m_iEpgID          = iEpgID;
  m_strName         = strName;
  m_strScraperName  = strScraperName;
  m_nowActive       = NULL;
  m_Channel         = NULL;
  m_bInhibitSorting = false;
  m_bHasChannel     = false;
  m_lastScanTime.SetValid(false);
  m_firstDate.SetValid(false);
  m_lastDate.SetValid(false);
}

CEpg::~CEpg(void)
{
  Clear();
}

/** @name Public methods */
//@{

bool CEpg::HasValidEntries(void) const
{
  CSingleLock lock(m_critSection);

  return (m_iEpgID > 0 && /* valid EPG ID */
          size() > 0 && /* contains at least 1 tag */
          at(size()-1)->m_endTime >= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()); /* the last end time hasn't passed yet */
}

bool CEpg::DeleteInfoTag(CEpgInfoTag *tag)
{
  bool bReturn = false;

  /* check if we're the "owner" of this tag */
  if (tag->m_Epg != this)
    return bReturn;

  CSingleLock lock(m_critSection);

  /* remove the tag */
  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CEpgInfoTag *entry = at(iTagPtr);
    if (entry == tag)
    {
      /* fix previous and next pointers */
      const CEpgInfoTag *previousTag = (iTagPtr > 0) ? at(iTagPtr - 1) : NULL;
      const CEpgInfoTag *nextTag = (iTagPtr < size() - 1) ? at(iTagPtr + 1) : NULL;
      if (previousTag)
        previousTag->m_nextEvent = nextTag;
      if (nextTag)
        nextTag->m_previousEvent = previousTag;

      erase(begin() + iTagPtr);
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

void CEpg::Sort(void)
{
  CSingleLock lock(m_critSection);

  if (m_bInhibitSorting)
    return;

  /* sort the EPG */
  sort(begin(), end(), sortEPGbyDate());

  /* reset the previous and next pointers on each tag */
  int iTagAmount = size();
  for (int ptr = 0; ptr < iTagAmount; ptr++)
  {
    CEpgInfoTag *tag = at(ptr);

    if (ptr == 0)
    {
      tag->SetPreviousEvent(NULL);
    }

    if (ptr > 0)
    {
      CEpgInfoTag *previousTag = at(ptr-1);
      previousTag->SetNextEvent(tag);
      tag->SetPreviousEvent(previousTag);
    }

    if (ptr == iTagAmount - 1)
    {
      tag->SetNextEvent(NULL);
    }
  }
}

void CEpg::Clear(void)
{
  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    delete at(iTagPtr);
  }
  erase(begin(), end());
}

void CEpg::Cleanup(void)
{
  Cleanup(CDateTime::GetCurrentDateTime().GetAsUTCDateTime());
}

void CEpg::Cleanup(const CDateTime &Time)
{
  CSingleLock lock(m_critSection);

  CDateTime firstDate = Time - CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);
  CDateTime dummy; dummy.SetValid(false);

  RemoveTagsBetween(dummy, firstDate);
}

void CEpg::RemoveTagsBetween(const CDateTime &begin, const CDateTime &end, bool bRemoveFromDb /* = false */)
{
  time_t beginT, endT;

  if (begin.IsValid())
    begin.GetAsTime(beginT);
  else
    beginT = 0;

  if (end.IsValid())
    end.GetAsTime(endT);
  else
    endT = 0;

  RemoveTagsBetween(beginT, endT, bRemoveFromDb);
}

void CEpg::RemoveTagsBetween(time_t start, time_t end, bool bRemoveFromDb /* = false */)
{
  CSingleLock lock(m_critSection);

  unsigned int iSize = size();
  for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    time_t tagBegin, tagEnd;
    if (tag)
    {
      bool bMatch(true);
      tag->StartAsLocalTime().GetAsTime(tagBegin);
      tag->EndAsLocalTime().GetAsTime(tagEnd);

      if (start > 0 && tagBegin < start)
        bMatch = false;
      if (end > 0 && tagEnd > end)
        bMatch = false;

      if (bMatch)
      {
        erase(begin() + iTagPtr);
        --iSize;
        --iTagPtr;
      }
    }
  }

  if (bRemoveFromDb)
  {
    CEpgDatabase *database = g_EpgContainer.GetDatabase();
    if (database && database->Open())
    {
      database->Delete(*this, start, end);
      database->Close();
    }
  }
}

const CEpgInfoTag *CEpg::InfoTagNow(void) const
{
  CSingleLock lock(m_critSection);

  if (!m_nowActive || !m_nowActive->IsActive())
  {
    CDateTime now = CDateTime::GetCurrentDateTime();
    /* one of the first items will always match if the list is sorted */
    for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
    {
      CEpgInfoTag *tag = at(iTagPtr);
      if (tag->StartAsLocalTime() <= now && tag->EndAsLocalTime() > now)
      {
        m_nowActive = tag;
        break;
      }
    }
  }

  return m_nowActive;
}

const CEpgInfoTag *CEpg::InfoTagNext(void) const
{
  CSingleLock lock(m_critSection);

  const CEpgInfoTag *nowTag = InfoTagNow();

  return nowTag ? nowTag->GetNextEvent() : NULL;
}

const CEpgInfoTag *CEpg::GetTag(int uniqueID, const CDateTime &StartTime) const
{
  CEpgInfoTag *returnTag = NULL;
  CSingleLock lock(m_critSection);

  /* try to find the tag by UID */
  if (uniqueID > 0)
  {
    for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
    {
      CEpgInfoTag *tag = at(iEpgPtr);
      if (tag->UniqueBroadcastID() == uniqueID)
      {
        returnTag = tag;
        break;
      }
    }
  }

  /* if we haven't found it, search by start time */
  if (!returnTag)
  {
    for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
    {
      CEpgInfoTag *tag = at(iEpgPtr);
      if (tag->StartAsUTC() == StartTime)
      {
        returnTag = tag;
        break;
      }
    }
  }

  return returnTag;
}

const CEpgInfoTag *CEpg::GetTagBetween(const CDateTime &beginTime, const CDateTime &endTime) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    if (tag->StartAsLocalTime() >= beginTime && tag->EndAsLocalTime() <= endTime)
    {
      returnTag = tag;
      break;
    }
  }

  return returnTag;
}

const CEpgInfoTag *CEpg::GetTagAround(const CDateTime &time) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    if ((tag->StartAsLocalTime() <= time) && (tag->EndAsLocalTime() >= time))
    {
      returnTag = tag;
      break;
    }
  }

  return returnTag;
}

void CEpg::AddEntry(const CEpgInfoTag &tag)
{
  CEpgInfoTag *newTag = CreateTag();
  if (newTag)
  {
    push_back(newTag);

    newTag->m_Epg = this;
    newTag->Update(tag);
  }
}

bool CEpg::UpdateEntry(const CEpgInfoTag &tag, bool bUpdateDatabase /* = false */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CEpgInfoTag *infoTag = (CEpgInfoTag *) GetTag(tag.UniqueBroadcastID(), tag.StartAsUTC());

  /* create a new tag if no tag with this ID exists */
  if (!infoTag)
  {
    infoTag = CreateTag();
    infoTag->SetUniqueBroadcastID(tag.UniqueBroadcastID());
    push_back(infoTag);
  }

  infoTag->m_Epg = this;
  infoTag->Update(tag);

  Sort();

  if (bUpdateDatabase)
    bReturn = infoTag->Persist();
  else
    bReturn = true;

  return bReturn;
}

bool CEpg::Load(void)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "Epg - %s - could not open the database", __FUNCTION__);
    return bReturn;
  }

  int iEntriesLoaded = database->Get(this);
  if (iEntriesLoaded <= 0)
  {
    CLog::Log(LOGNOTICE, "Epg - %s - no database entries found for table '%s'.",
        __FUNCTION__, m_strName.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "Epg - %s - %d entries loaded for table '%s'.",
        __FUNCTION__, (int) size(), m_strName.c_str());
    Sort();
    UpdateFirstAndLastDates();
    bReturn = true;
  }

  database->Close();

  return bReturn;
}

bool CEpg::LoadFromClients(time_t start, time_t end)
{
  bool bReturn(false);
  CEpg tmpEpg(m_iEpgID, m_strName, m_strScraperName);
  if (tmpEpg.UpdateFromScraper(start, end))
    bReturn = UpdateEntries(tmpEpg, !g_guiSettings.GetBool("epg.ignoredbforclient"));

  return bReturn;
}

void CEpg::UpdateFirstAndLastDates(void)
{
  CSingleLock lock(m_critSection);

  /* update the first and last date */
  if (size() > 0)
  {
    m_firstDate = at(0)->StartAsLocalTime();
    m_lastDate = at(size() - 1)->EndAsLocalTime();
  }
  else
  {
    m_firstDate.SetValid(false);
    m_lastDate.SetValid(false);
  }
}

bool CEpg::UpdateEntries(const CEpg &epg, bool bStoreInDb /* = true */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  /* remove tags from the current list that will be replaced */
  RemoveTagsBetween(epg.GetFirstDate(), epg.GetLastDate(), bStoreInDb);

  /* copy over tags */
  for (unsigned int iTagPtr = 0; iTagPtr < epg.size(); iTagPtr++)
  {
    CEpgInfoTag *newTag = CreateTag();
    newTag->Update(*epg.at(iTagPtr));
    newTag->m_Epg = this;
    push_back(newTag);
  }

  /* sort the list */
  Sort();

  /* fix overlapping events */
  FixOverlappingEvents(false);

  /* update the last scan time of this table */
  m_lastScanTime = CDateTime::GetCurrentDateTime();

  /* update the first and last date */
  UpdateFirstAndLastDates();

  /* persist changes */
  if (bStoreInDb)
  {
    CEpgDatabase *database = g_EpgContainer.GetDatabase();
    if (database && database->Open())
    {
      database->PersistLastEpgScanTime(m_iEpgID, true);
      database->Persist(*this, true);
      PersistTags(true);
      bReturn = database->CommitInsertQueries();
      database->Close();
    }
    else
    {
      CLog::Log(LOGERROR, "Epg - %s - cannot open the database", __FUNCTION__);
    }
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

const CDateTime &CEpg::GetLastScanTime(void)
{
  CSingleLock lock(m_critSection);

  if (!m_lastScanTime.IsValid())
  {
    if (!g_guiSettings.GetBool("epg.ignoredbforclient"))
    {
      CEpgDatabase *database = g_EpgContainer.GetDatabase();
      CDateTime dtReturn; dtReturn.SetValid(false);

      if (database && database->Open())
      {
        database->GetLastEpgScanTime(m_iEpgID, &m_lastScanTime);
        database->Close();
      }
    }

    if (!m_lastScanTime.IsValid())
    {
      m_lastScanTime.SetDateTime(0, 0, 0, 0, 0, 0);
      m_lastScanTime.SetValid(true);
    }
  }

  return m_lastScanTime;
}

bool CEpg::Update(const time_t start, const time_t end, int iUpdateTime)
{
  bool bGrabSuccess = true;
  bool bUpdate = false;

  CSingleLock lock(m_critSection);

  /* get the last update time from the database */
  CDateTime lastScanTime = GetLastScanTime();

  /* check if we have to update */
  time_t iNow = 0;
  time_t iLastUpdate = 0;
  CDateTime::GetCurrentDateTime().GetAsTime(iNow);
  lastScanTime.GetAsTime(iLastUpdate);
  bUpdate = (iNow > iLastUpdate + iUpdateTime);

  lock.Leave();

  if (bUpdate)
    bGrabSuccess = LoadFromClients(start, end);

  return bGrabSuccess;
}

int CEpg::Get(CFileItemList *results) const
{
  int iInitialSize = results->Size();

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    if (at(iTagPtr)->EndAsLocalTime() < CDateTime::GetCurrentDateTime() - CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0))
      continue;

    CFileItemPtr entry(new CFileItem(*at(iTagPtr)));
    entry->SetLabel2(at(iTagPtr)->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
    results->Add(entry);
  }

  return size() - iInitialSize;
}

int CEpg::Get(CFileItemList *results, const EpgSearchFilter &filter) const
{
  int iInitialSize = results->Size();

  if (!HasValidEntries())
    return -1;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    if (filter.FilterEntry(*at(iTagPtr)))
    {
      CFileItemPtr entry(new CFileItem(*at(iTagPtr)));
      entry->SetLabel2(at(iTagPtr)->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
      results->Add(entry);
    }
  }

  return size() - iInitialSize;
}

bool CEpg::Persist(bool bPersistTags /* = false */, bool bQueueWrite /* = false */)
{
  bool bReturn = false;
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
    return bReturn;
  }

  CSingleLock lock(m_critSection);

  int iId = database->Persist(*this, bQueueWrite);
  if (iId >= 0)
  {
    if (iId > 0)
      m_iEpgID = iId;

    if (bPersistTags)
      bReturn = PersistTags(bQueueWrite);
    else
      bReturn = true;
  }

  database->Close();

  return bReturn;
}

const CDateTime &CEpg::GetFirstDate(void) const
{
  CSingleLock lock(m_critSection);

  return m_firstDate;
}

const CDateTime &CEpg::GetLastDate(void) const
{
  CSingleLock lock(m_critSection);

  return m_lastDate;
}

//@}

/** @name Protected methods */
//@{

bool CEpg::Update(const CEpg &epg, bool bUpdateDb /* = false */)
{
  bool bReturn = true;

  m_strName = epg.m_strName;
  m_strScraperName = epg.m_strScraperName;

  if (bUpdateDb)
    bReturn = Persist(false);

  return bReturn;
}

//@}

/** @name Private methods */
//@{

bool CEpg::FixOverlappingEvents(bool bStore /* = true */)
{
  bool bReturn = false;
  CEpgDatabase *database = NULL;

  if (bStore)
  {
    database = g_EpgContainer.GetDatabase();

    if (!database || !database->Open())
    {
      CLog::Log(LOGERROR, "EPG - %s - could not open the database", __FUNCTION__);
      return bReturn;
    }
  }

  bReturn = true;
  CEpgInfoTag *previousTag = NULL;

  CSingleLock lock(m_critSection);
  Sort();

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    /* skip the first entry or if previousTag is NULL */
    if (previousTag == NULL)
    {
      previousTag = at(ptr);
      continue;
    }

    CEpgInfoTag *currentTag = at(ptr);

    /* the previous tag ends after the current tag starts.
     * the start time of the current tag is leading, so change the time of the previous tag
     */
    if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
    {
      CLog::Log(LOGDEBUG, "EPG - %s - event '%s' ends after event '%s' starts. changing the end time of '%s' to the start time of '%s': '%s'",
          __FUNCTION__, previousTag->Title().c_str(), currentTag->Title().c_str(),
          previousTag->Title().c_str(), currentTag->Title().c_str(),
          currentTag->StartAsLocalTime().GetAsLocalizedDateTime(false, false).c_str());

      previousTag->SetEndFromUTC(currentTag->StartAsUTC());

      if (bStore)
        bReturn = previousTag->Persist(false, false) && bReturn;
    }

    previousTag = at(ptr);
  }

  return bReturn;
}

bool CEpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_strScraperName.IsEmpty()) /* no grabber defined */
  {
    CLog::Log(LOGERROR, "EPG - %s - no EPG grabber defined for table '%s'",
        __FUNCTION__, m_strName.c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "EPG - %s - updating EPG table '%s' with scraper '%s'",
        __FUNCTION__, m_strName.c_str(), m_strScraperName.c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CEpg::PersistTags(bool bQueueWrite /* = false */) const
{
  bool bReturn = false;
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "EPG - %s - could not load the database", __FUNCTION__);
    return bReturn;
  }

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    at(iTagPtr)->Persist(false);
  }

  if (!bQueueWrite)
    bReturn = database->CommitInsertQueries();
  else
    bReturn = true;

  return bReturn;
}

CEpgInfoTag *CEpg::CreateTag(void)
{
  CEpgInfoTag *newTag = new CEpgInfoTag();
  if (!newTag)
  {
    CLog::Log(LOGERROR, "EPG - %s - couldn't create new infotag",
        __FUNCTION__);
  }

  return newTag;
}

//@}
