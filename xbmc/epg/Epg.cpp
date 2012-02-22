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
using namespace std;

CEpg::CEpg(int iEpgID, const CStdString &strName /* = "" */, const CStdString &strScraperName /* = "" */, bool bLoadedFromDb /* = false */) :
    m_bChanged(!bLoadedFromDb),
    m_bTagsChanged(false),
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
  m_bLoaded         = right.m_bLoaded;
  m_iEpgID          = right.m_iEpgID;
  m_strName         = right.m_strName;
  m_strScraperName  = right.m_strScraperName;
  m_nowActive       = right.m_nowActive;
  m_lastScanTime    = right.m_lastScanTime;
  m_firstDate       = right.m_firstDate;
  m_lastDate        = right.m_lastDate;
  m_Channel         = right.m_Channel;

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

  m_nowActive = NULL;
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
  for (map<CDateTime, CEpgInfoTag *>::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); it++)
  {
    if (it->second->EndAsUTC() < Time)
    {
      if (m_nowActive && *m_nowActive == *it->second)
        m_nowActive = NULL;

      delete it->second;
      m_tags.erase(it->first);
      bTagsChanged = true;
    }
  }

  if (bTagsChanged)
    UpdateFirstAndLastDates();
}

bool CEpg::InfoTagNow(CEpgInfoTag &tag) const
{
  CSingleLock lock(m_critSection);
  if (!m_nowActive || !m_nowActive->IsActive())
  {
    CDateTime now = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
    /* one of the first items will always match if the list is sorted */
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
    {
      if (it->second->StartAsUTC() <= now && it->second->EndAsUTC() > now)
      {
        m_nowActive = it->second;
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
    CSingleLock lock(m_critSection);
    map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.find(nowTag.StartAsUTC());
    if (it != m_tags.end() && ++it != m_tags.end())
    {
      tag = *it->second;
      return true;
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
  CEpgInfoTag *newTag = new CEpgInfoTag();
  if (newTag)
  {
    m_tags.insert(make_pair(tag.StartAsUTC(), newTag));

    newTag->m_iEpgId = m_iEpgID;
    newTag->Update(tag);
    newTag->m_bChanged = false;
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
    infoTag = new CEpgInfoTag();
    infoTag->SetUniqueBroadcastID(tag.UniqueBroadcastID());
    m_tags.insert(make_pair(tag.StartAsUTC(), infoTag));
    bNewTag = true;
  }

  infoTag->m_iEpgId = m_iEpgID;
  infoTag->Update(tag, bNewTag);

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
        __FUNCTION__, (int) m_tags.size(), m_strName.c_str());
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
  if (m_tags.size() > 0)
  {
    m_firstDate = m_tags.begin()->second->StartAsUTC();
    m_lastDate = m_tags.rbegin()->second->EndAsUTC();
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

  if (epg.m_tags.size() > 0)
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
    CLog::Log(LOGDEBUG, "%s - %u entries in memory before merging", __FUNCTION__, m_tags.size());
    /* copy over tags */
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = epg.m_tags.begin(); it != epg.m_tags.end(); it++)
      UpdateEntry(*it->second, bStoreInDb, false);

    CLog::Log(LOGDEBUG, "%s - %u entries in memory after merging and before fixing", __FUNCTION__, m_tags.size());
    FixOverlappingEvents(bStoreInDb);
    m_nowActive = NULL;
    CLog::Log(LOGDEBUG, "%s - %u entries in memory after fixing", __FUNCTION__, m_tags.size());
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

  for (map<CDateTime, CEpgInfoTag *>::reverse_iterator it = m_tags.rbegin(); it != m_tags.rend(); it++)
  {
    if (!previousTag)
    {
      previousTag = it->second;
      continue;
    }
    currentTag = it->second;

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
      m_tags.erase(it->first);
    }
    else if (previousTag->StartAsUTC() < currentTag->EndAsUTC())
    {
      currentTag->SetEndFromUTC(previousTag->StartAsUTC());
      if (bUpdateDb)
        bReturn = currentTag->Persist();
      else
        bReturn = true;
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

  if (m_tags.size() > 0)
  {
    for (map<CDateTime, CEpgInfoTag *>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++)
      bReturn = it->second->Persist() && bReturn;
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
