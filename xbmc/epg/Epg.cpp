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
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "utils/StringUtils.h"

#include "../addons/include/xbmc_pvr_types.h" // TODO extract the epg specific stuff

using namespace PVR;
using namespace EPG;
using namespace std;

CEpg::CEpg(int iEpgID, const CStdString &strName /* = "" */, const CStdString &strScraperName /* = "" */, bool bLoadedFromDb /* = false */) :
    m_bChanged(!bLoadedFromDb),
    m_bTagsChanged(false),
    m_bLoaded(false),
    m_iEpgID(iEpgID),
    m_strName(strName),
    m_strScraperName(strScraperName),
    m_iPVRChannelId(-1),
    m_iPVRChannelNumber(-1)
{
}

CEpg::CEpg(CPVRChannel *channel, bool bLoadedFromDb /* = false */) :
    m_bChanged(!bLoadedFromDb),
    m_bTagsChanged(false),
    m_bLoaded(false),
    m_iEpgID(channel->EpgID()),
    m_strName(channel->ChannelName()),
    m_strScraperName(channel->EPGScraper()),
    m_iPVRChannelId(channel->ChannelID()),
    m_iPVRChannelNumber(channel->ChannelNumber())
{
}

CEpg::CEpg(void) :
    m_bChanged(false),
    m_bTagsChanged(false),
    m_bLoaded(false),
    m_iEpgID(0),
    m_strName(StringUtils::EmptyString),
    m_strScraperName(StringUtils::EmptyString),
    m_iPVRChannelId(-1),
    m_iPVRChannelNumber(-1)
{
}

CEpg::~CEpg(void)
{
  Clear();
}

CEpg &CEpg::operator =(const CEpg &right)
{
  m_bChanged          = right.m_bChanged;
  m_bTagsChanged      = right.m_bTagsChanged;
  m_bLoaded           = right.m_bLoaded;
  m_iEpgID            = right.m_iEpgID;
  m_strName           = right.m_strName;
  m_strScraperName    = right.m_strScraperName;
  m_nowActiveStart    = right.m_nowActiveStart;
  m_lastScanTime      = right.m_lastScanTime;
  m_iPVRChannelId     = right.m_iPVRChannelId;
  m_iPVRChannelNumber = right.m_iPVRChannelNumber;

  for (map<CDateTime, CEpgInfoTag *>::const_iterator it = right.m_tags.begin(); it != right.m_tags.end(); it++)
    m_tags.insert(make_pair(it->first, new CEpgInfoTag(*it->second)));

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
      m_tags.size() > 0 && /* contains at least 1 tag */
      m_tags.rbegin()->second->EndAsUTC() >= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()); /* the last end time hasn't passed yet */
}

void CEpg::Clear(void)
{
  CSingleLock lock(m_critSection);

  for (map<CDateTime, CEpgInfoTag *>::iterator it = m_tags.begin(); it != m_tags.end(); it++)
    delete it->second;
  m_tags.clear();
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
  for (map<CDateTime, CEpgInfoTag *>::iterator it = m_tags.begin(); it != m_tags.end(); it != m_tags.end() ? it++ : it)
  {
    if (it->second->EndAsUTC() < Time)
    {
      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      delete it->second;
      m_tags.erase(it++);
      bTagsChanged = true;
    }
  }
}

bool CEpg::InfoTagNow(CEpgInfoTag &tag, bool bUpdateIfNeeded /* = true */)
{
  CSingleLock lock(m_critSection);
  if (m_nowActiveStart.IsValid())
  {
    map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(m_nowActiveStart);
    if (it != m_tags.end() && it->second->IsActive())
    {
      tag = *it->second;
      return true;
    }
  }

  if (bUpdateIfNeeded)
  {
    CDateTime lastActiveTag;

    /* one of the first items will always match if the list is sorted */
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      if (it->second->IsActive())
      {
        m_nowActiveStart = it->first;
        tag = *it->second;
        return true;
      }
      else if (it->second->WasActive())
        lastActiveTag = it->first;
    }

    /* there might be a gap between the last and next event. just return the last if found */
    map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(lastActiveTag);
    if (it != m_tags.end())
    {
      tag = *it->second;
      return true;
    }
  }

  return false;
}

bool CEpg::InfoTagNext(CEpgInfoTag &tag)
{
  CEpgInfoTag nowTag;
  if (InfoTagNow(nowTag))
  {
    CSingleLock lock(m_critSection);
    map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(nowTag.StartAsUTC());
    if (it != m_tags.end() && ++it != m_tags.end())
    {
      tag = *it->second;
      return true;
    }
  }
  else if (Size() > 0)
  {
    /* return the first event that is in the future */
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      if (it->second->InTheFuture())
      {
        tag = *it->second;
        return true;
      }
    }
  }

  return false;
}

bool CEpg::CheckPlayingEvent(void)
{
  bool bReturn(false);
  CEpgInfoTag previousTag, newTag;
  bool bGotPreviousTag = InfoTagNow(previousTag, false);
  bool bGotCurrentTag = InfoTagNow(newTag);

  if (!bGotPreviousTag || (bGotCurrentTag && previousTag != newTag))
  {
    NotifyObservers("epg-current-event");
    bReturn = true;
  }

  return bReturn;
}

CEpgInfoTag *CEpg::GetTag(int uniqueID, const CDateTime &StartTime) const // TODO remove uid param
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);
  map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(StartTime);
  if (it != m_tags.end())
    returnTag = it->second;

  return returnTag;
}

const CEpgInfoTag *CEpg::GetTagBetween(const CDateTime &beginTime, const CDateTime &endTime) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    if (it->second->StartAsUTC() >= beginTime && it->second->EndAsUTC() <= endTime)
    {
      returnTag = it->second;
      break;
    }
  }

  return returnTag;
}

const CEpgInfoTag *CEpg::GetTagAround(const CDateTime &time) const
{
  CEpgInfoTag *returnTag = NULL;

  CSingleLock lock(m_critSection);

  for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    if ((it->second->StartAsUTC() <= time) && (it->second->EndAsUTC() >= time))
    {
      returnTag = it->second;
      break;
    }
  }

  return returnTag;
}

void CEpg::AddEntry(const CEpgInfoTag &tag)
{
  CEpgInfoTag *newTag(NULL);
  {
    CSingleLock lock(m_critSection);
    map<CDateTime, CEpgInfoTag*>::iterator itr = m_tags.find(tag.StartAsUTC());
    if (itr != m_tags.end())
      newTag = itr->second;
    else
    {
      newTag = new CEpgInfoTag(m_iEpgID, m_iPVRChannelNumber, m_iPVRChannelId, m_strName);
      m_tags.insert(make_pair(tag.StartAsUTC(), newTag));
    }
  }

  if (newTag)
  {
    newTag->Update(tag);
    newTag->m_iEpgId            = m_iEpgID;
    newTag->m_iPVRChannelNumber = m_iPVRChannelNumber;
    newTag->m_iPVRChannelID     = m_iPVRChannelId;
    newTag->m_strTableName      = m_strName;
    newTag->m_bChanged          = false;
  }
}

bool CEpg::UpdateEntry(const CEpgInfoTag &tag, bool bUpdateDatabase /* = false */, bool bSort /* = true */)
{
  bool bReturn(false);
  CSingleLock lock(m_critSection);

  CEpgInfoTag *infoTag(NULL);
  map<CDateTime, CEpgInfoTag *>::iterator it = m_tags.find(tag.StartAsUTC());
  bool bNewTag(false);
  if (it != m_tags.end())
  {
    infoTag = it->second;
  }
  else
  {
    /* create a new tag if no tag with this ID exists */
    infoTag = new CEpgInfoTag(m_iEpgID, m_iPVRChannelNumber, m_iPVRChannelId, m_strName);
    infoTag->SetUniqueBroadcastID(tag.UniqueBroadcastID());
    m_tags.insert(make_pair(tag.StartAsUTC(), infoTag));
    bNewTag = true;
  }

  infoTag->Update(tag, bNewTag);
  infoTag->m_iEpgId            = m_iEpgID;
  infoTag->m_iPVRChannelNumber = m_iPVRChannelNumber;
  infoTag->m_iPVRChannelID     = m_iPVRChannelId;
  infoTag->m_strTableName      = m_strName;

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

  if (!database || !database->IsOpen())
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
        __FUNCTION__, (int) m_tags.size(), m_strName.c_str());
    bReturn = true;
  }

  m_bLoaded = true;

  return bReturn;
}

bool CEpg::UpdateEntries(const CEpg &epg, bool bStoreInDb /* = true */)
{
  bool bReturn(false);
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  CSingleLock lock(m_critSection);

  if (epg.m_tags.size() > 0)
  {
    if (bStoreInDb)
    {
      if (!database || !database->IsOpen())
      {
        CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
        return bReturn;
      }
      database->BeginTransaction();
    }
    CLog::Log(LOGDEBUG, "%s - %u entries in memory before merging", __FUNCTION__, m_tags.size());
    /* copy over tags */
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = epg.m_tags.begin(); it != epg.m_tags.end(); it++)
      UpdateEntry(*it->second, bStoreInDb, false);

    CLog::Log(LOGDEBUG, "%s - %u entries in memory after merging and before fixing", __FUNCTION__, m_tags.size());
    FixOverlappingEvents(bStoreInDb);
    CLog::Log(LOGDEBUG, "%s - %u entries in memory after fixing", __FUNCTION__, m_tags.size());
    /* update the last scan time of this table */
    m_lastScanTime = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();

    //m_bTagsChanged = true;
    /* persist changes */
    if (bStoreInDb)
    {
      bReturn = database->CommitTransaction();
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

CDateTime CEpg::GetLastScanTime(void)
{
  CDateTime lastScanTime;
  {
    CSingleLock lock(m_critSection);

    if (!m_lastScanTime.IsValid())
    {
      if (!g_guiSettings.GetBool("epg.ignoredbforclient"))
      {
        CEpgDatabase *database = g_EpgContainer.GetDatabase();
        CDateTime dtReturn; dtReturn.SetValid(false);

        if (database && database->IsOpen())
          database->GetLastEpgScanTime(m_iEpgID, &m_lastScanTime);
      }

      if (!m_lastScanTime.IsValid())
      {
        m_lastScanTime.SetDateTime(0, 0, 0, 0, 0, 0);
        m_lastScanTime.SetValid(true);
      }
    }
    lastScanTime = m_lastScanTime;
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

  for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    CDateTime localStartTime;
    localStartTime.SetFromUTCDateTime(it->first);

    CFileItemPtr entry(new CFileItem(*it->second));
    entry->SetLabel2(localStartTime.GetAsLocalizedDateTime(false, false));
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

  for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    if (filter.FilterEntry(*it->second))
    {
      CDateTime localStartTime;
      localStartTime.SetFromUTCDateTime(it->first);

      CFileItemPtr entry(new CFileItem(*it->second));
      entry->SetLabel2(localStartTime.GetAsLocalizedDateTime(false, false));
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

  if (!database || !database->IsOpen())
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

  return bReturn;
}

CDateTime CEpg::GetFirstDate(void) const
{
  CDateTime first;

  CSingleLock lock(m_critSection);
  if (m_tags.size() > 0)
    first = m_tags.begin()->second->StartAsUTC();

  return first;
}

CDateTime CEpg::GetLastDate(void) const
{
  CDateTime last;

  CSingleLock lock(m_critSection);
  if (m_tags.size() > 0)
    last = m_tags.rbegin()->second->StartAsUTC();

  return last;
}

//@}

/** @name Protected methods */
//@{

bool CEpg::UpdateMetadata(const CEpg &epg, bool bUpdateDb /* = false */)
{
  bool bReturn = true;
  CSingleLock lock(m_critSection);

  m_strName        = epg.m_strName;
  m_strScraperName = epg.m_strScraperName;
  if (m_iPVRChannelId == -1 || epg.m_iPVRChannelId != -1)
  {
    m_iPVRChannelId     = epg.m_iPVRChannelId;
    m_iPVRChannelNumber = epg.m_iPVRChannelNumber;
  }

  if (bUpdateDb)
    bReturn = Persist();

  return bReturn;
}

//@}

/** @name Private methods */
//@{

bool CEpg::FixOverlappingEvents(bool bUpdateDb /* = false */)
{
  bool bReturn(true);
  CEpgInfoTag *previousTag(NULL), *currentTag(NULL);
  CEpgDatabase *database(NULL);
  if (bUpdateDb)
  {
    database = g_EpgContainer.GetDatabase();
    if (!database || !database->IsOpen())
    {
      CLog::Log(LOGERROR, "%s - could not open the database", __FUNCTION__);
      return false;
    }
  }

  for (map<CDateTime, CEpgInfoTag *>::iterator it = m_tags.begin(); it != m_tags.end(); it != m_tags.end() ? it++ : it)
  {
    if (!previousTag)
    {
      previousTag = it->second;
      continue;
    }
    currentTag = it->second;

    if (previousTag->EndAsUTC() >= currentTag->EndAsUTC())
    {
      // delete the current tag. it's completely overlapped
      if (bUpdateDb)
        bReturn &= database->Delete(*currentTag);

      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      delete currentTag;
      m_tags.erase(it++);
    }
    else if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
    {
      currentTag->SetStartFromUTC(previousTag->EndAsUTC());
      if (bUpdateDb)
        bReturn &= currentTag->Persist();

      previousTag = it->second;
    }
    else if (previousTag->EndAsUTC() < currentTag->StartAsUTC())
    {
      time_t start, end, middle;
      previousTag->EndAsUTC().GetAsTime(start);
      currentTag->StartAsUTC().GetAsTime(end);
      middle = start + ((end - start) / 2);
      CDateTime newTime(middle);

      currentTag->SetStartFromUTC(newTime);
      previousTag->SetEndFromUTC(newTime);

      if (m_nowActiveStart == it->first)
        m_nowActiveStart = currentTag->StartAsUTC();

      if (bUpdateDb)
      {
        bReturn &= currentTag->Persist();
        bReturn &= previousTag->Persist();
      }

      previousTag = it->second;
    }
    else
    {
      previousTag = it->second;
    }
  }

  return bReturn;
}

bool CEpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;
  if (ScraperName() == "client")
  {
    CPVRChannel *channel = Channel();
    if (!channel)
      CLog::Log(LOGINFO, "%s - channel not found, can't update", __FUNCTION__);
    else if (!channel->EPGEnabled())
      CLog::Log(LOGINFO, "%s - EPG updating disabled in the channel configuration", __FUNCTION__);
    else if (!g_PVRClients->GetAddonCapabilities(channel->ClientID()).bSupportsEPG)
      CLog::Log(LOGINFO, "%s - the backend for channel '%s' on client '%i' does not support EPGs", __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
    else
    {
      CLog::Log(LOGINFO, "%s - updating EPG for channel '%s' from client '%i'", __FUNCTION__, channel->ChannelName().c_str(), channel->ClientID());
      PVR_ERROR error;
      g_PVRClients->GetEPGForChannel(*channel, this, start, end, &error);
      bGrabSuccess = error == PVR_ERROR_NO_ERROR;
    }
  }
  else if (m_strScraperName.IsEmpty()) /* no grabber defined */
    CLog::Log(LOGERROR, "EPG - %s - no EPG scraper defined for table '%s'", __FUNCTION__, m_strName.c_str());
  else
  {
    CLog::Log(LOGINFO, "EPG - %s - updating EPG table '%s' with scraper '%s'", __FUNCTION__, m_strName.c_str(), m_strScraperName.c_str());
    CLog::Log(LOGERROR, "loading the EPG via scraper has not been implemented yet");
    // TODO: Add Support for Web EPG Scrapers here
  }

  return bGrabSuccess;
}

bool CEpg::PersistTags(void) const
{
  bool bReturn = false;
  CEpgDatabase *database = g_EpgContainer.GetDatabase();

  if (!database || !database->IsOpen())
  {
    CLog::Log(LOGERROR, "EPG - %s - could not load the database", __FUNCTION__);
    return bReturn;
  }

  CDateTime first = GetFirstDate();
  CDateTime last = GetLastDate();

  time_t iStart(0), iEnd(0);
  if (first.IsValid())
    first.GetAsTime(iStart);
  if (last.IsValid())
    last.GetAsTime(iEnd);
  database->Delete(*this, iStart, iEnd);

  if (m_tags.size() > 0)
  {
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      if (!it->second->Persist())
      {
        CLog::Log(LOGERROR, "failed to persist epg tag %d", it->second->UniqueBroadcastID());
        bReturn = false;
      }
    }
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
  CPVRChannel *channel = Channel();
  return channel ? channel->IsRadio() : false;
}

bool CEpg::IsRemovableTag(const CEpgInfoTag *tag) const
{
  return (!tag || !tag->HasTimer());
}

bool CEpg::LoadFromClients(time_t start, time_t end)
{
  bool bReturn(false);
  CPVRChannel *channel = Channel();
  if (channel)
  {
    CEpg tmpEpg(channel);
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

const CEpgInfoTag *CEpg::GetNextEvent(const CEpgInfoTag& tag) const
{
  CSingleLock lock(m_critSection);
  map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(tag.StartAsUTC());
  if (it != m_tags.end() && ++it != m_tags.end())
    return it->second;
  return NULL;
}

const CEpgInfoTag *CEpg::GetPreviousEvent(const CEpgInfoTag& tag) const
{
  CSingleLock lock(m_critSection);
  map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(tag.StartAsUTC());
  if (it != m_tags.end() && it != m_tags.begin())
  {
    it--;
    return it->second;
  }
  return NULL;
}

CPVRChannel *CEpg::Channel(void) const
{
  int iChannelId(-1);
  {
    CSingleLock lock(m_critSection);
    iChannelId = m_iPVRChannelId;
  }

  if (iChannelId != -1 && g_PVRManager.IsStarted())
    return g_PVRChannelGroups->GetByChannelIDFromAll(iChannelId);

  return NULL;
}

int CEpg::ChannelID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iPVRChannelId;
}

int CEpg::ChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iPVRChannelNumber;
}

void CEpg::SetChannel(PVR::CPVRChannel *channel)
{
  CSingleLock lock(m_critSection);
  m_iPVRChannelId     = channel->ChannelID();
  m_iPVRChannelNumber = channel->ChannelNumber();
  for (map<CDateTime, CEpgInfoTag *>::iterator it = m_tags.begin(); it != m_tags.end(); it++)
  {
    it->second->m_iPVRChannelID     = m_iPVRChannelId;
    it->second->m_iPVRChannelNumber = m_iPVRChannelNumber;
  }
}

bool CEpg::HasPVRChannel(void) const
{
  CSingleLock lock(m_critSection);
  return m_iPVRChannelId != -1;
}
