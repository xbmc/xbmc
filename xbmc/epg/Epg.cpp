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

#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "log.h"
#include "TimeUtils.h"

#include "EpgDatabase.h"
#include "EpgContainer.h"

struct sortEPGbyDate
{
  bool operator()(CEpgInfoTag* strItem1, CEpgInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->Start() < strItem2->Start();
  }
};

CEpg::CEpg(int iEpgID, const CStdString &strName /* = CStdString() */, const CStdString &strScraperName /* = CStdString() */)
{
  m_iEpgID          = iEpgID;
  m_strName         = strName;
  m_strScraperName  = strScraperName;
  m_nowActive       = NULL;
  m_Channel         = NULL;
  m_bInhibitSorting = false;
  m_bHasChannel     = false;
  m_lastScanTime.SetValid(false);
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
          at(size()-1)->m_endTime >= CDateTime::GetCurrentDateTime()); /* the last end time hasn't passed yet */
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
  Cleanup(CDateTime::GetCurrentDateTime());
}

void CEpg::Cleanup(const CDateTime &Time)
{
  CSingleLock lock(m_critSection);

  unsigned int iSize = size();
  for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    if ( tag && /* valid tag */
        (tag->End() + CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0) < Time))
    {
      erase(begin() + iTagPtr);
      --iSize;
      --iTagPtr;
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
      if (tag->Start() <= now && tag->End() > now)
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
  const CEpgInfoTag *nowTag = InfoTagNow();

  return nowTag ? nowTag->GetNextEvent() : NULL;
}

const CEpgInfoTag *CEpg::InfoTag(int uniqueID, const CDateTime &StartTime) const
{
  CEpgInfoTag *returnTag = NULL;
  CSingleLock locka(m_critSection);

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
      if (tag->Start() == StartTime)
      {
        returnTag = tag;
        break;
      }
    }
  }

  return returnTag;
}

const CEpgInfoTag *CEpg::InfoTagBetween(CDateTime BeginTime, CDateTime EndTime) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    if (tag->Start() >= BeginTime && tag->End() <= EndTime)
    {
      returnTag = tag;
      break;
    }
  }

  return returnTag;
}

const CEpgInfoTag *CEpg::InfoTagAround(CDateTime Time) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CEpgInfoTag *tag = at(iTagPtr);
    if ((tag->Start() <= Time) && (tag->End() >= Time))
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
  bool bReturn = false;

  /* XXX tags aren't always fetched correctly here */
  CEpgInfoTag *InfoTag = (CEpgInfoTag *) this->InfoTag(tag.UniqueBroadcastID(), tag.Start());
  /* create a new tag if no tag with this ID exists */
  if (!InfoTag)
  {
    CSingleLock lock(m_critSection);
    InfoTag = CreateTag();
    InfoTag->SetUniqueBroadcastID(tag.UniqueBroadcastID());
    push_back(InfoTag);
  }

  InfoTag->m_Epg = this;
  InfoTag->Update(tag);

  Sort();

  if (bUpdateDatabase)
    bReturn = InfoTag->Persist();
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
        __FUNCTION__, size(), m_strName.c_str());
    Sort();
    bReturn = true;
  }

  database->Close();

  return bReturn;
}

bool CEpg::Update(time_t start, time_t end, int iUpdateTime, bool bStoreInDb /* = true */)
{
  bool bGrabSuccess = true;
  bool bUpdate = false;
  CEpgDatabase *database = g_EpgContainer.GetDatabase();
  if (bStoreInDb && (!database || !database->Open()))
  {
    CLog::Log(LOGERROR, "Epg - %s - cannot open the database", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_critSection);

  /* get the last update time from the database */
  if (!m_lastScanTime.IsValid() && bStoreInDb)
    database->GetLastEpgScanTime(m_iEpgID, &m_lastScanTime);

  /* check if we have to update */
  time_t iNow = 0;
  time_t iLastUpdate = 0;
  CDateTime::GetCurrentDateTime().GetAsTime(iNow);
  m_lastScanTime.GetAsTime(iLastUpdate);
  bUpdate = (iNow > iLastUpdate + iUpdateTime);

  lock.Leave();

  if (bUpdate)
  {
    m_bInhibitSorting = true;
    bGrabSuccess = UpdateFromScraper(start, end);
    m_bInhibitSorting = false;

    /* store the loaded EPG entries in the database */
    if (bGrabSuccess && size() > 0)
    {
      Sort();
      lock.Enter();
      FixOverlappingEvents(false);

      if (bStoreInDb)
      {
        database->PersistLastEpgScanTime(m_iEpgID, true);
        database->Persist(*this, true);
        PersistTags(true);
        database->CommitInsertQueries();
      }

      lock.Leave();
      m_lastScanTime = CDateTime::GetCurrentDateTime();
    }
    else
    {
      m_lastScanTime = CDateTime::GetCurrentDateTime() - CDateTimeSpan(0, 0, 0, iUpdateTime + 300); /* try again in 5 minutes */
    }
  }

  if (bStoreInDb)
    database->Close();

  return bGrabSuccess;
}

int CEpg::Get(CFileItemList *results) const
{
  int iInitialSize = results->Size();

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CFileItemPtr entry(new CFileItem(*at(iTagPtr)));
    entry->SetLabel2(at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
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
      entry->SetLabel2(at(iTagPtr)->Start().GetAsLocalizedDateTime(false, false));
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

CDateTime CEpg::GetFirstDate(void)
{
  CSingleLock lock(m_critSection);

  return size() > 0 ? at(0)->Start() : CDateTime();
}

CDateTime CEpg::GetLastDate(void)
{
  CSingleLock lock(m_critSection);

  return size() > 0 ? at(size() - 1)->End() : CDateTime();
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
    if (previousTag->End() > currentTag->Start())
    {
      CLog::Log(LOGDEBUG, "EPG - %s - event '%s' ends after event '%s' starts. changing the end time of '%s' to the start time of '%s': '%s'",
          __FUNCTION__, previousTag->Title().c_str(), currentTag->Title().c_str(),
          previousTag->Title().c_str(), currentTag->Title().c_str(),
          currentTag->Start().GetAsLocalizedDateTime(false, false).c_str());

      previousTag->SetEnd(currentTag->Start());

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
