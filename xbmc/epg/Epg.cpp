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

#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include "EpgDatabase.h"
#include "EpgContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/StringUtils.h"

#include "../addons/include/xbmc_pvr_types.h" // TODO extract the epg specific stuff

using namespace PVR;
using namespace EPG;

struct sortEPGbyDate
{
  bool operator()(CEpgInfoTag* strItem1, CEpgInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->StartAsUTC() < strItem2->StartAsUTC();
  }
};

CEpg::CEpg(int iEpgID, const CStdString &strName /* = "" */, const CStdString &strScraperName /* = "" */, bool bLoadedFromDb /* = false */) :
    m_bChanged(!bLoadedFromDb),
    m_bTagsChanged(false),
    m_bInhibitSorting(false),
    m_bLoaded(false),
    m_iEpgID(iEpgID),
    m_strName(strName),
    m_strScraperName(strScraperName),
    m_nowActive(NULL),
    m_Channel(NULL)
{
  m_lastScanTime.SetValid(false);
  m_firstDate.SetValid(false);
  m_lastDate.SetValid(false);
}

CEpg::CEpg(CPVRChannel *channel, bool bLoadedFromDb /* = false */) :
    m_bChanged(!bLoadedFromDb),
    m_bTagsChanged(false),
    m_bInhibitSorting(false),
    m_bLoaded(false),
    m_iEpgID(channel->EpgID()),
    m_strName(channel->ChannelName()),
    m_strScraperName(channel->EPGScraper()),
    m_nowActive(NULL),
    m_Channel(channel)
{
  m_lastScanTime.SetValid(false);
  m_firstDate.SetValid(false);
  m_lastDate.SetValid(false);
}

CEpg::CEpg(void) :
    m_bChanged(false),
    m_bTagsChanged(false),
    m_bInhibitSorting(false),
    m_bLoaded(false),
    m_iEpgID(0),
    m_strName(StringUtils::EmptyString),
    m_strScraperName(StringUtils::EmptyString),
    m_nowActive(NULL),
    m_Channel(NULL)
{
  m_lastScanTime.SetValid(false);
  m_firstDate.SetValid(false);
  m_lastDate.SetValid(false);
}

CEpg::~CEpg(void)
{
  Clear();
}

CEpg &CEpg::operator =(const CEpg &right)
{
  m_bChanged        = right.m_bChanged;
  m_bTagsChanged    = right.m_bTagsChanged;
  m_bInhibitSorting = right.m_bInhibitSorting;
  m_bLoaded         = right.m_bLoaded;
  m_iEpgID          = right.m_iEpgID;
  m_strName         = right.m_strName;
  m_strScraperName  = right.m_strScraperName;
  m_nowActive       = right.m_nowActive;
  m_lastScanTime    = right.m_lastScanTime;
  m_firstDate       = right.m_firstDate;
  m_lastDate        = right.m_lastDate;
  m_Channel         = right.m_Channel;

  for (size_t iPtr = 0; iPtr < right.size(); iPtr++)
  {
    CEpgInfoTag *tag = new CEpgInfoTag(*right.at(iPtr));
    push_back(tag);
  }

  return *this;
}

/** @name Public methods */
//@{

void CEpg::SetName(const CStdString &strName)
{
  CSingleLock lock(m_critSection);

  if (!m_strName.Equals(strName))
  {
    m_bChanged = true;
    m_strName = strName;
  }
}

void CEpg::SetScraperName(const CStdString &strScraperName)
{
  CSingleLock lock(m_critSection);

  if (!m_strScraperName.Equals(strScraperName))
  {
    m_bChanged = true;
    m_strScraperName = strScraperName;
  }
}

bool CEpg::HasValidEntries(void) const
{
  CSingleLock lock(m_critSection);

  return (m_iEpgID > 0 && /* valid EPG ID */
          size() > 0 && /* contains at least 1 tag */
          at(size()-1)->EndAsUTC() >= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()); /* the last end time hasn't passed yet */
}

void CEpg::Sort(void)
{
  CSingleLock lock(m_critSection);

  if (m_bInhibitSorting)
    return;

  /* sort the EPG */
  sort(begin(), end(), sortEPGbyDate());

  /* reset the previous and next pointers on each tag */
  UpdatePreviousAndNextPointers();
}

void CEpg::UpdatePreviousAndNextPointers(void)
{
  int iTagAmount = size();
  for (int ptr = 0; ptr < iTagAmount; ptr++)
  {
    CEpgInfoTag *tag = at(ptr);

    if (ptr == 0)
    {
      /* first tag has no previous event */
      tag->SetPreviousEvent(NULL);
    }
    else
    {
      /* set the next event in the previous tag */
      CEpgInfoTag *previousTag = at(ptr-1);
      previousTag->SetNextEvent(tag);

      /* set the previous event in this tag */
      tag->SetPreviousEvent(previousTag);
    }

    if (ptr == iTagAmount - 1)
    {
      /* ensure that the next event for the last tag is set to NULL */
      tag->SetNextEvent(NULL);
    }
  }
}

void CEpg::Clear(void)
{
  CSingleLock lock(m_critSection);

  m_nowActive = NULL;
  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
    delete at(iTagPtr);
  erase(begin(), end());
}

void CEpg::Cleanup(void)
{
  CDateTime cleanupTime = CDateTime::GetCurrentDateTime().GetAsUTCDateTime() -
      CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);
  Cleanup(cleanupTime);
}

void CEpg::Cleanup(const CDateTime &Time)
{
  bool bTagsChanged(false);
  CSingleLock lock(m_critSection);
  for (int iPtr = size() - 1; iPtr >= 0; iPtr--)
  {
    if (at(iPtr)->EndAsUTC() < Time)
    {
      if (m_nowActive && *m_nowActive == *at(iPtr))
        m_nowActive = NULL;

      delete at(iPtr);
      erase(begin() + iPtr);
      bTagsChanged = true;
    }
  }

  if (bTagsChanged)
  {
    UpdatePreviousAndNextPointers();
    UpdateFirstAndLastDates();
  }
}

bool CEpg::InfoTagNow(CEpgInfoTag &tag) const
{
  CSingleLock lock(m_critSection);
  if (!m_nowActive || !m_nowActive->IsActive())
  {
    CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
    /* one of the first items will always match if the list is sorted */
    for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
    {
      CEpgInfoTag *tag = at(iTagPtr);
      if (tag->StartAsUTC() <= now && tag->EndAsUTC() > now)
      {
        m_nowActive = tag;
        break;
      }
    }
  }

  if (m_nowActive)
    tag = *m_nowActive;
  return m_nowActive != NULL;
}

bool CEpg::InfoTagNext(CEpgInfoTag &tag) const
{
  CEpgInfoTag nowTag;
  if (InfoTagNow(nowTag))
  {
    const CEpgInfoTag *nextTag = nowTag.GetNextEvent();
    if (nextTag)
      tag = *nextTag;
    return nextTag != NULL;
  }

  CSingleLock lock(m_critSection);
  if (size() >  0)
  {
    CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
    for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
    {
      if (at(iTagPtr)->StartAsUTC() > now)
      {
        tag = *at(iTagPtr);
        return true;
      }
    }
  }

  return false;
}

bool CEpg::CheckPlayingEvent(void)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);
  const CEpgInfoTag *previousTag = m_nowActive;
  CEpgInfoTag tag;
  if (InfoTagNow(tag) && (!previousTag || *previousTag != tag))
  {
    NotifyObservers("epg-current-event");
    bReturn = true;
  }
  return bReturn;
}

CEpgInfoTag *CEpg::GetTag(int uniqueID, const CDateTime &StartTime) const
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
    if (tag->StartAsUTC() >= beginTime && tag->EndAsUTC() <= endTime)
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
    if ((tag->StartAsUTC() <= time) && (tag->EndAsUTC() >= time))
    {
      returnTag = tag;
      break;
    }
  }

  return returnTag;
}

void CEpg::AddEntry(const CEpgInfoTag &tag)
{
  CEpgInfoTag *newTag = new CEpgInfoTag();
  if (newTag)
  {
    push_back(newTag);

    newTag->m_Epg = this;
    newTag->Update(tag);
    newTag->m_bChanged = false;
  }
}

bool CEpg::UpdateEntry(const CEpgInfoTag &tag, bool bUpdateDatabase /* = false */, bool bSort /* = true */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CEpgInfoTag *infoTag = GetTag(tag.UniqueBroadcastID(), tag.StartAsUTC());

  /* create a new tag if no tag with this ID exists */
  bool bNewTag(false);
  if (!infoTag)
  {
    infoTag = new CEpgInfoTag();
    infoTag->SetUniqueBroadcastID(tag.UniqueBroadcastID());
    push_back(infoTag);
    bNewTag = true;
  }

  infoTag->m_Epg = this;
  infoTag->Update(tag, bNewTag);

  if (bSort)
    Sort();

  if (bUpdateDatabase)
    bReturn = infoTag->Persist();
  else
    bReturn = true;

  return bReturn;
}

bool CEpg::Load(void)
{
  bool bReturn(false);
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "Epg - %s - could not open the database", __FUNCTION__);
    return bReturn;
  }

  CSingleLock lock(m_critSection);
  int iEntriesLoaded = database->Get(*this);
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

  m_bLoaded = true;
  database->Close();

  return bReturn;
}

void CEpg::UpdateFirstAndLastDates(void)
{
  CSingleLock lock(m_critSection);

  /* update the first and last date */
  if (size() > 0)
  {
    m_firstDate = at(0)->StartAsUTC();
    m_lastDate = at(size() - 1)->EndAsUTC();
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
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  CSingleLock lock(m_critSection);

  if (epg.size() > 0)
  {
    if (bStoreInDb)
    {
      if (!database || !database->Open())
      {
        CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
        return bReturn;
      }
      database->BeginTransaction();
    }
    CLog::Log(LOGDEBUG, "%s - %u entries in memory before merging", __FUNCTION__, size());
    /* copy over tags */
    for (unsigned int iTagPtr = 0; iTagPtr < epg.size(); iTagPtr++)
    {
      UpdateEntry(*epg.at(iTagPtr), bStoreInDb, false);
    }

    Sort();
    CLog::Log(LOGDEBUG, "%s - %u entries in memory after merging and before fixing", __FUNCTION__, size());
    FixOverlappingEvents(bStoreInDb);
    m_nowActive = NULL;
    CLog::Log(LOGDEBUG, "%s - %u entries in memory after fixing", __FUNCTION__, size());
    /* update the last scan time of this table */
    m_lastScanTime = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();

    /* update the first and last date */
    UpdateFirstAndLastDates();

    //m_bTagsChanged = true;
    /* persist changes */
    if (bStoreInDb)
    {
      bReturn = database->CommitTransaction();
      database->Close();
      if (bReturn)
        Persist(true);
    }
    else
      bReturn = true;
  }
  else
  {
    if (bStoreInDb)
      bReturn = Persist(true);
    else
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
  bool bGrabSuccess(true);
  bool bUpdate(false);

  /* load the entries from the db first */
  if (!m_bLoaded && !g_EpgContainer.IgnoreDB())
    Load();

  /* clean up if needed */
  if (m_bLoaded)
    Cleanup();

  /* get the last update time from the database */
  CDateTime lastScanTime = GetLastScanTime();

  /* check if we have to update */
  time_t iNow = 0;
  time_t iLastUpdate = 0;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
  lastScanTime.GetAsTime(iLastUpdate);
  bUpdate = (iNow > iLastUpdate + iUpdateTime);

  if (bUpdate)
    bGrabSuccess = LoadFromClients(start, end);

  if (bGrabSuccess)
  {
    g_PVRManager.ResetPlayingTag();
    m_bLoaded = true;
  }
  else
    CLog::Log(LOGERROR, "EPG - %s - failed to update table '%s'", __FUNCTION__, Name().c_str());

  return bGrabSuccess;
}

int CEpg::Get(CFileItemList &results) const
{
  int iInitialSize = results.Size();

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    CFileItemPtr entry(new CFileItem(*at(iTagPtr)));
    entry->SetLabel2(at(iTagPtr)->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
    results.Add(entry);
  }

  return results.Size() - iInitialSize;
}

int CEpg::Get(CFileItemList &results, const EpgSearchFilter &filter) const
{
  int iInitialSize = results.Size();

  if (!HasValidEntries())
    return -1;

  CSingleLock lock(m_critSection);

  for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
  {
    if (filter.FilterEntry(*at(iTagPtr)))
    {
      CFileItemPtr entry(new CFileItem(*at(iTagPtr)));
      entry->SetLabel2(at(iTagPtr)->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
      results.Add(entry);
    }
  }

  return results.Size() - iInitialSize;
}

bool CEpg::Persist(bool bUpdateLastScanTime /* = false */)
{
  if (g_guiSettings.GetBool("epg.ignoredbforclient"))
    return true;

  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
    return false;
  }

  CEpg epgCopy;
  {
    CSingleLock lock(m_critSection);
    epgCopy = *this;
    m_bChanged     = false;
    m_bTagsChanged = false;
  }

  database->BeginTransaction();

  if (epgCopy.m_iEpgID <= 0 || epgCopy.m_bChanged)
  {
    int iId = database->Persist(epgCopy);
    if (iId > 0)
    {
      epgCopy.m_iEpgID   = iId;
      epgCopy.m_bChanged = false;
      if (m_iEpgID != epgCopy.m_iEpgID)
      {
        CSingleLock lock(m_critSection);
        m_iEpgID = epgCopy.m_iEpgID;
      }
    }
  }

  bool bReturn(true);

  if (bUpdateLastScanTime)
    bReturn = database->PersistLastEpgScanTime(epgCopy.m_iEpgID);

  database->CommitTransaction();

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

bool CEpg::UpdateMetadata(const CEpg &epg, bool bUpdateDb /* = false */)
{
  bool bReturn = true;
  CSingleLock lock(m_critSection);

  m_strName = epg.m_strName;
  m_strScraperName = epg.m_strScraperName;
  if (epg.HasPVRChannel())
    m_Channel = epg.m_Channel;

  if (bUpdateDb)
    bReturn = Persist();

  return bReturn;
}

//@}

/** @name Private methods */
//@{

bool CEpg::FixOverlappingEvents(bool bUpdateDb /* = false */)
{
  bool bReturn(false);
  CEpgInfoTag *previousTag(NULL), *currentTag(NULL);
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  for (int iPtr = size() - 1; iPtr >= 0; iPtr--)
  {
    if (!previousTag)
    {
      previousTag = at(iPtr);
      continue;
    }
    currentTag = at(iPtr);

    if (previousTag->StartAsUTC() <= currentTag->StartAsUTC())
    {
      if (bUpdateDb)
      {
        if (!database || !database->Open())
        {
          CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
          bReturn = false;
          continue;
        }
        bReturn = database->Delete(*currentTag);
      }
      else
        bReturn = true;

      if (m_nowActive && *m_nowActive == *currentTag)
        m_nowActive = NULL;

      delete currentTag;
      erase(begin() + iPtr);
    }
    else if (previousTag->StartAsUTC() < currentTag->EndAsUTC())
    {
      currentTag->SetEndFromUTC(previousTag->StartAsUTC());
      if (bUpdateDb)
        bReturn = currentTag->Persist();
      else
        bReturn = true;
      previousTag = at(iPtr);
    }
    else
    {
      previousTag = at(iPtr);
    }
  }

  return bReturn;
}

bool CEpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (g_PVRManager.IsStarted() && HasPVRChannel() && m_Channel->EPGEnabled() && ScraperName() == "client")
  {
    if (g_PVRManager.IsStarted() && g_PVRClients->GetAddonCapabilities(m_Channel->ClientID()).bSupportsEPG)
    {
      CLog::Log(LOGINFO, "%s - updating EPG for channel '%s' from client '%i'",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
      PVR_ERROR error;
      g_PVRClients->GetEPGForChannel(*m_Channel, this, start, end, &error);
      bGrabSuccess = error == PVR_ERROR_NO_ERROR;
    }
    else
    {
      CLog::Log(LOGINFO, "%s - channel '%s' on client '%i' does not support EPGs",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
    }
  }
  else if (m_strScraperName.IsEmpty()) /* no grabber defined */
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

  if (bGrabSuccess)
    Sort();

  return bGrabSuccess;
}

bool CEpg::PersistTags(void) const
{
  bool bReturn = false;
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->Open())
  {
    CLog::Log(LOGERROR, "EPG - %s - could not load the database", __FUNCTION__);
    return bReturn;
  }

  time_t iStart, iEnd;
  GetFirstDate().GetAsTime(iStart);
  GetLastDate().GetAsTime(iEnd);
  database->Delete(*this, iStart, iEnd);

  if (size() > 0)
  {
    for (unsigned int iTagPtr = 0; iTagPtr < size(); iTagPtr++)
      bReturn = at(iTagPtr)->Persist() && bReturn;
  }
  else
  {
    /* Return true if we have no tags, so that no error is logged */
    bReturn = true;
  }

  return bReturn;
}

//@}

const CStdString &CEpg::ConvertGenreIdToString(int iID, int iSubID)
{
  unsigned int iLabelId = 19499;
  switch (iID)
  {
    case EPG_EVENT_CONTENTMASK_MOVIEDRAMA:
      iLabelId = (iSubID <= 8) ? 19500 + iSubID : 19500;
      break;
    case EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS:
      iLabelId = (iSubID <= 4) ? 19516 + iSubID : 19516;
      break;
    case EPG_EVENT_CONTENTMASK_SHOW:
      iLabelId = (iSubID <= 3) ? 19532 + iSubID : 19532;
      break;
    case EPG_EVENT_CONTENTMASK_SPORTS:
      iLabelId = (iSubID <= 11) ? 19548 + iSubID : 19548;
      break;
    case EPG_EVENT_CONTENTMASK_CHILDRENYOUTH:
      iLabelId = (iSubID <= 5) ? 19564 + iSubID : 19564;
      break;
    case EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE:
      iLabelId = (iSubID <= 6) ? 19580 + iSubID : 19580;
      break;
    case EPG_EVENT_CONTENTMASK_ARTSCULTURE:
      iLabelId = (iSubID <= 11) ? 19596 + iSubID : 19596;
      break;
    case EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS:
      iLabelId = (iSubID <= 3) ? 19612 + iSubID : 19612;
      break;
    case EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE:
      iLabelId = (iSubID <= 7) ? 19628 + iSubID : 19628;
      break;
    case EPG_EVENT_CONTENTMASK_LEISUREHOBBIES:
      iLabelId = (iSubID <= 7) ? 19644 + iSubID : 19644;
      break;
    case EPG_EVENT_CONTENTMASK_SPECIAL:
      iLabelId = (iSubID <= 3) ? 19660 + iSubID : 19660;
      break;
    case EPG_EVENT_CONTENTMASK_USERDEFINED:
      iLabelId = (iSubID <= 3) ? 19676 + iSubID : 19676;
      break;
    default:
      break;
  }

  return g_localizeStrings.Get(iLabelId);
}

bool CEpg::UpdateEntry(const EPG_TAG *data, bool bUpdateDatabase /* = false */)
{
  if (!data)
    return false;

  CEpgInfoTag tag(*data);
  return UpdateEntry(tag, bUpdateDatabase);
}

bool CEpg::IsRadio(void) const
{
  CSingleLock lock(m_critSection);

  return HasPVRChannel() ? m_Channel->IsRadio() : false;
}

bool CEpg::IsRemovableTag(const CEpgInfoTag *tag) const
{
  CSingleLock lock(m_critSection);

  if (HasPVRChannel())
  {
    return (!tag || !tag->HasTimer());
  }

  return true;
}


bool CEpg::LoadFromClients(time_t start, time_t end)
{
  bool bReturn(false);
  if (HasPVRChannel())
  {
    CEpg tmpEpg(m_Channel);
    if (tmpEpg.UpdateFromScraper(start, end))
      bReturn = UpdateEntries(tmpEpg, !g_guiSettings.GetBool("epg.ignoredbforclient"));
  }
  else
  {
    CEpg tmpEpg(m_iEpgID, m_strName, m_strScraperName);
    if (tmpEpg.UpdateFromScraper(start, end))
      bReturn = UpdateEntries(tmpEpg, !g_guiSettings.GetBool("epg.ignoredbforclient"));
  }

  return bReturn;
}
