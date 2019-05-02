/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Epg.h"

#include <utility>

#include "addons/PVRClient.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_epg_types.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgDatabase.h"

using namespace PVR;

CPVREpg::CPVREpg(int iEpgID, const std::string& strName, const std::string& strScraperName)
: m_bChanged(false),
  m_iEpgID(iEpgID),
  m_strName(strName),
  m_strScraperName(strScraperName),
  m_channelData(new CPVREpgChannelData)
{
}

CPVREpg::CPVREpg(int iEpgID, const std::string& strName, const std::string& strScraperName, const std::shared_ptr<CPVREpgChannelData>& channelData)
: m_bChanged(true),
  m_iEpgID(iEpgID),
  m_strName(strName),
  m_strScraperName(strScraperName),
  m_channelData(channelData)
{
}

CPVREpg::~CPVREpg(void)
{
  Clear();
}

void CPVREpg::ForceUpdate(void)
{
  {
    CSingleLock lock(m_critSection);
    m_bUpdatePending = true;
  }

  SetChanged(true);
  NotifyObservers(ObservableMessageEpgUpdatePending);
}

bool CPVREpg::HasValidEntries(void) const
{
  CSingleLock lock(m_critSection);
  return (m_iEpgID > 0 && /* valid EPG ID */
          !m_tags.empty() && /* contains at least 1 tag */
          m_tags.rbegin()->second->EndAsUTC() >= CDateTime::GetCurrentDateTime().GetAsUTCDateTime()); /* the last end time hasn't passed yet */
}

void CPVREpg::Clear(void)
{
  CSingleLock lock(m_critSection);
  m_tags.clear();
}

void CPVREpg::Cleanup(int iPastDays)
{
  const CDateTime cleanupTime = CDateTime::GetUTCDateTime() - CDateTimeSpan(iPastDays, 0, 0, 0);
  Cleanup(cleanupTime);
}

void CPVREpg::Cleanup(const CDateTime &time)
{
  CSingleLock lock(m_critSection);
  for (auto it = m_tags.begin(); it != m_tags.end();)
  {
    if (it->second->EndAsUTC() < time)
    {
      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      it = m_tags.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

CPVREpgInfoTagPtr CPVREpg::GetTagNow(bool bUpdateIfNeeded /* = true */) const
{
  CSingleLock lock(m_critSection);
  if (m_nowActiveStart.IsValid())
  {
    const auto it = m_tags.find(m_nowActiveStart);
    if (it != m_tags.end() && it->second->IsActive())
      return it->second;
  }

  if (bUpdateIfNeeded)
  {
    CPVREpgInfoTagPtr lastActiveTag;

    /* one of the first items will always match if the list is sorted */
    for (const auto& tag : m_tags)
    {
      if (tag.second->IsActive())
      {
        m_nowActiveStart = tag.first;
        return tag.second;
      }
      else if (tag.second->WasActive())
        lastActiveTag = tag.second;
    }

    /* there might be a gap between the last and next event. return the last if found and it ended not more than 5 minutes ago */
    if (lastActiveTag &&
        lastActiveTag->EndAsUTC() + CDateTimeSpan(0, 0, 5, 0) >= CDateTime::GetUTCDateTime())
      return lastActiveTag;
  }

  return CPVREpgInfoTagPtr();
}

CPVREpgInfoTagPtr CPVREpg::GetTagNext() const
{
  const CPVREpgInfoTagPtr nowTag = GetTagNow();
  if (nowTag)
  {
    CSingleLock lock(m_critSection);
    auto it = m_tags.find(nowTag->StartAsUTC());
    if (it != m_tags.end() && ++it != m_tags.end())
      return it->second;
  }
  else if (m_tags.size() > 0)
  {
    /* return the first event that is in the future */
    for (const auto& tag : m_tags)
    {
      if (tag.second->IsUpcoming())
        return tag.second;
    }
  }

  return CPVREpgInfoTagPtr();
}

CPVREpgInfoTagPtr CPVREpg::GetTagPrevious() const
{
  const CPVREpgInfoTagPtr nowTag = GetTagNow();
  if (nowTag)
  {
    CSingleLock lock(m_critSection);
    auto it = m_tags.find(nowTag->StartAsUTC());
    if (it != m_tags.end() && it != m_tags.begin())
    {
      --it;
      return it->second;
    }
  }
  else if (m_tags.size() > 0)
  {
    /* return the first event that is in the past */
    for (auto it = m_tags.rbegin(); it != m_tags.rend(); ++it)
    {
      if (it->second->WasActive())
        return it->second;
    }
  }

  return CPVREpgInfoTagPtr();
}

bool CPVREpg::CheckPlayingEvent(void)
{
  const CPVREpgInfoTagPtr previousTag = GetTagNow(false);
  const CPVREpgInfoTagPtr newTag = GetTagNow(true);

  bool bTagChanged = newTag && (!previousTag || *previousTag != *newTag);
  bool bTagRemoved = !newTag && previousTag;
  if (bTagChanged || bTagRemoved)
  {
    NotifyObservers(ObservableMessageEpgActiveItem);
    return true;
  }
  return false;
}

CPVREpgInfoTagPtr CPVREpg::GetTagByBroadcastId(unsigned int iUniqueBroadcastId) const
{
  if (iUniqueBroadcastId != EPG_TAG_INVALID_UID)
  {
    CSingleLock lock(m_critSection);
    for (const auto &infoTag : m_tags)
    {
      if (infoTag.second->UniqueBroadcastID() == iUniqueBroadcastId)
        return infoTag.second;
    }
  }
  return CPVREpgInfoTagPtr();
}

CPVREpgInfoTagPtr CPVREpg::GetTagBetween(const CDateTime &beginTime, const CDateTime &endTime, bool bUpdateFromClient /* = false */)
{
  CPVREpgInfoTagPtr tag;

  CSingleLock lock(m_critSection);
  for (const auto& epgTag : m_tags)
  {
    if (epgTag.second->StartAsUTC() >= beginTime && epgTag.second->EndAsUTC() <= endTime)
    {
      tag = epgTag.second;
      break;
    }
  }

  if (!tag && bUpdateFromClient)
  {
    // not found locally; try to fetch from client
    time_t b;
    beginTime.GetAsTime(b);
    time_t e;
    endTime.GetAsTime(e);

    const std::shared_ptr<CPVREpg> tmpEpg = std::make_shared<CPVREpg>(m_iEpgID, m_strName, m_strScraperName, m_channelData);
    if (tmpEpg->UpdateFromScraper(b, e, true))
      tag = tmpEpg->GetTagBetween(beginTime, endTime, false);

    if (tag)
    {
      m_tags.insert(std::make_pair(tag->StartAsUTC(), tag));
      UpdateEntry(tag, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EPG_STOREEPGINDATABASE));
    }
  }

  return tag;
}

void CPVREpg::AddEntry(const CPVREpgInfoTag &tag)
{
  CPVREpgInfoTagPtr newTag;

  CSingleLock lock(m_critSection);
  const auto it = m_tags.find(tag.StartAsUTC());
  if (it != m_tags.end())
    newTag = it->second;
  else
  {
    newTag.reset(new CPVREpgInfoTag());
    m_tags.insert(std::make_pair(tag.StartAsUTC(), newTag));
  }

  newTag->Update(tag);
  newTag->SetChannelData(m_channelData);
  newTag->SetEpgID(m_iEpgID);
}

bool CPVREpg::Load(const std::shared_ptr<CPVREpgDatabase>& database)
{
  bool bReturn = false;

  if (!database)
  {
    CLog::LogF(LOGERROR, "Could not open the EPG database");
    return bReturn;
  }

  const std::vector<CPVREpgInfoTagPtr> result = database->Get(*this);

  CSingleLock lock(m_critSection);
  if (result.empty())
  {
    CLog::LogFC(LOGDEBUG, LOGEPG, "No database entries found for table '%s'.", m_strName.c_str());
  }
  else
  {
    for (const auto& entry : result)
      AddEntry(*entry);

    if (!m_lastScanTime.IsValid())
      database->GetLastEpgScanTime(m_iEpgID, &m_lastScanTime);

    if (!m_lastScanTime.IsValid())
    {
      m_lastScanTime.SetDateTime(1970, 1, 1, 0, 0, 0);
      m_bUpdateLastScanTime = true;
    }

    bReturn = true;
  }

  m_bLoaded = true;

  return bReturn;
}

bool CPVREpg::UpdateEntries(const CPVREpg &epg, bool bStoreInDb /* = true */)
{
  CSingleLock lock(m_critSection);
  /* copy over tags */
  for (const auto& tag : epg.m_tags)
    UpdateEntry(tag.second, bStoreInDb);

  FixOverlappingEvents(bStoreInDb);

  /* update the last scan time of this table */
  m_lastScanTime = CDateTime::GetUTCDateTime();
  m_bUpdateLastScanTime = true;

  SetChanged(true);

  lock.Leave();
  NotifyObservers(ObservableMessageEpg);

  return true;
}

bool CPVREpg::UpdateEntry(const EPG_TAG *data, int iClientId, bool bUpdateDatabase)
{
  if (!data)
    return false;

  const std::shared_ptr<CPVREpgInfoTag> tag = std::make_shared<CPVREpgInfoTag>(*data, iClientId, m_channelData, m_iEpgID);
  return UpdateEntry(tag, bUpdateDatabase);
}

bool CPVREpg::UpdateEntry(const CPVREpgInfoTagPtr &tag, bool bUpdateDatabase)
{
  CPVREpgInfoTagPtr infoTag;

  CSingleLock lock(m_critSection);
  const auto it = m_tags.find(tag->StartAsUTC());
  bool bNewTag = false;
  if (it != m_tags.end())
  {
    infoTag = it->second;
  }
  else
  {
    infoTag.reset(new CPVREpgInfoTag());
    infoTag->SetUniqueBroadcastID(tag->UniqueBroadcastID());
    m_tags.insert(std::make_pair(tag->StartAsUTC(), infoTag));
    bNewTag = true;
  }

  infoTag->Update(*tag, bNewTag);
  infoTag->SetChannelData(m_channelData);
  infoTag->SetEpgID(m_iEpgID);

  if (bUpdateDatabase)
    m_changedTags.insert(std::make_pair(infoTag->UniqueBroadcastID(), infoTag));

  return true;
}

bool CPVREpg::UpdateEntry(const CPVREpgInfoTagPtr &tag, EPG_EVENT_STATE newState, bool bUpdateDatabase)
{
  bool bRet = true;
  bool bNotify = true;

  if (newState == EPG_EVENT_CREATED || newState == EPG_EVENT_UPDATED)
  {
    bRet = UpdateEntry(tag, bUpdateDatabase);
  }
  else if (newState == EPG_EVENT_DELETED)
  {
    CSingleLock lock(m_critSection);
    auto it = m_tags.begin();
    for (; it != m_tags.end(); ++it)
    {
      if (it->second->UniqueBroadcastID() == tag->UniqueBroadcastID())
        break;
    }

    if (it == m_tags.end())
    {
      bRet = false;
    }
    else
    {
      // Respect epg linger time.
      int iPastDays = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY);
      const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(iPastDays, 0, 0, 0));
      if (it->second->EndAsUTC() < cleanupTime)
      {
        if (bUpdateDatabase)
          m_deletedTags.insert(std::make_pair(it->second->UniqueBroadcastID(), it->second));

        m_tags.erase(it);
      }
      else
      {
        bNotify = false;
      }
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Unknown epg event state value: %d", newState);
    bRet = false;
  }

  if (bRet && bNotify)
  {
    SetChanged();
    NotifyObservers(ObservableMessageEpgItemUpdate);
  }

  return bRet;
}

bool CPVREpg::Update(time_t start,
                     time_t end,
                     int iUpdateTime,
                     int iPastDays,
                     const std::shared_ptr<CPVREpgDatabase>& database,
                     bool bForceUpdate /* = false */)
{
  bool bGrabSuccess = true;
  bool bUpdate = false;

  /* load the entries from the db first */
  if (!m_bLoaded && database)
    Load(database);

  /* clean up if needed */
  if (m_bLoaded)
    Cleanup(iPastDays);

  /* enforce advanced settings update interval override for channels with no EPG data */
  if (m_tags.empty() && !bUpdate && ChannelID() > 0)
    iUpdateTime = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEpgUpdateEmptyTagsInterval;

  if (!bForceUpdate)
  {
    /* check if we have to update */
    time_t iNow = 0;
    time_t iLastUpdate = 0;
    CDateTime::GetUTCDateTime().GetAsTime(iNow);
    m_lastScanTime.GetAsTime(iLastUpdate);
    bUpdate = (iNow > iLastUpdate + iUpdateTime);
  }
  else
    bUpdate = true;

  if (bUpdate)
    bGrabSuccess = LoadFromClients(start, end, bForceUpdate);

  if (bGrabSuccess)
    m_bLoaded = true;
  else
    CLog::LogF(LOGERROR, "Failed to update table '%s'", Name().c_str());

  CSingleLock lock(m_critSection);
  m_bUpdatePending = false;

  return bGrabSuccess;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpg::GetTags() const
{
  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

  CSingleLock lock(m_critSection);
  for (const auto& tag : m_tags)
    tags.emplace_back(tag.second);

  return tags;
}

bool CPVREpg::Persist(const std::shared_ptr<CPVREpgDatabase>& database)
{
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  database->Lock();

  {
    CSingleLock lock(m_critSection);
    bool bEpgIdChanged = false;
    if (m_iEpgID <= 0 || m_bChanged)
    {
      int iId = database->Persist(*this, m_iEpgID > 0);
      if (iId > 0 && m_iEpgID != iId)
      {
        m_iEpgID = iId;
        bEpgIdChanged = true;
      }
    }

    for (const auto& tag : m_deletedTags)
      database->Delete(*tag.second);

    for (const auto& tag : m_changedTags)
      tag.second->Persist(database, false);

    if (m_bUpdateLastScanTime)
      database->PersistLastEpgScanTime(m_iEpgID, m_lastScanTime, true);

    if (bEpgIdChanged)
    {
      for (const auto& tag : m_tags)
        tag.second->SetEpgID(m_iEpgID);
    }

    m_deletedTags.clear();
    m_changedTags.clear();
    m_bChanged            = false;
    m_bTagsChanged        = false;
    m_bUpdateLastScanTime = false;
  }

  bool bRet = database->CommitInsertQueries();

  database->Unlock();
  return bRet;
}

CDateTime CPVREpg::GetFirstDate(void) const
{
  CDateTime first;

  CSingleLock lock(m_critSection);
  if (!m_tags.empty())
    first = m_tags.begin()->second->StartAsUTC();

  return first;
}

CDateTime CPVREpg::GetLastDate(void) const
{
  CDateTime last;

  CSingleLock lock(m_critSection);
  if (!m_tags.empty())
    last = m_tags.rbegin()->second->StartAsUTC();

  return last;
}

bool CPVREpg::FixOverlappingEvents(bool bUpdateDb /* = false */)
{
  bool bReturn = true;
  CPVREpgInfoTagPtr previousTag, currentTag;

  for (auto it = m_tags.begin(); it != m_tags.end(); it != m_tags.end() ? it++ : it)
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
        m_deletedTags.insert(std::make_pair(currentTag->UniqueBroadcastID(), currentTag));

      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      m_tags.erase(it++);
    }
    else if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
    {
      previousTag->SetEndFromUTC(currentTag->StartAsUTC());
      if (bUpdateDb)
        m_changedTags.insert(std::make_pair(previousTag->UniqueBroadcastID(), previousTag));

      previousTag = it->second;
    }
    else
    {
      previousTag = it->second;
    }
  }

  return bReturn;
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end, bool bForceUpdate)
{
  if (m_strScraperName.empty())
  {
    CLog::LogF(LOGERROR, "No EPG scraper defined for table '%s'", m_strName.c_str());
  }
  else if (m_strScraperName == "client")
  {
    if (!CServiceBroker::GetPVRManager().EpgsCreated())
      return false;

    if (!m_channelData->IsEPGEnabled() || m_channelData->IsHidden())
    {
      // ignore. not interested in any updates.
      return true;
    }

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_channelData->ClientId());
    if (client)
    {
      if (!client->GetClientCapabilities().SupportsEPG())
      {
        CLog::LogF(LOGERROR, "The backend for channel '%s' on client '%i' does not support EPGs",
                   m_channelData->ChannelName().c_str(), m_channelData->ClientId());
      }
      else if (!bForceUpdate && client->GetClientCapabilities().SupportsAsyncEPGTransfer())
      {
        // nothing to do. client will provide epg updates asynchronously
        return true;
      }
      else
      {
        CLog::LogFC(LOGDEBUG, LOGEPG, "Updating EPG for channel '%s' from client '%i'",
                    m_channelData->ChannelName().c_str(), m_channelData->ClientId());
        return (client->GetEPGForChannel(m_channelData->UniqueClientChannelId(), this, start, end) == PVR_ERROR_NO_ERROR);
      }
    }
    else
    {
      CLog::LogF(LOGERROR, "Client '%i' not found, can't update", m_channelData->ClientId());
    }
  }
  else // other non-empty scraper name...
  {
    CLog::LogF(LOGERROR, "Loading the EPG via scraper is not yet implemented!");
    //! @todo Add Support for Web EPG Scrapers here
  }

  return false;
}

const std::string& CPVREpg::ConvertGenreIdToString(int iID, int iSubID)
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
      iLabelId = (iSubID <= 8) ? 19676 + iSubID : 19676;
      break;
    default:
      break;
  }

  return g_localizeStrings.Get(iLabelId);
}

bool CPVREpg::LoadFromClients(time_t start, time_t end, bool bForceUpdate)
{
  bool bReturn = false;

  const std::shared_ptr<CPVREpg> tmpEpg = std::make_shared<CPVREpg>(m_iEpgID, m_strName, m_strScraperName, m_channelData);
  if (tmpEpg->UpdateFromScraper(start, end, bForceUpdate))
    bReturn = UpdateEntries(*tmpEpg, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EPG_STOREEPGINDATABASE));

  return bReturn;
}

std::shared_ptr<CPVREpgChannelData> CPVREpg::GetChannelData() const
{
  CSingleLock lock(m_critSection);
  return m_channelData;
}

void CPVREpg::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  CSingleLock lock(m_critSection);
  m_channelData = data;

  for (const auto& tag : m_tags)
    tag.second->SetChannelData(data);
}

int CPVREpg::ChannelID(void) const
{
  CSingleLock lock(m_critSection);
  return m_channelData->ChannelId();
}

const std::string& CPVREpg::ScraperName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strScraperName;
}

const std::string& CPVREpg::Name(void) const
{
  CSingleLock lock(m_critSection);
  return m_strName;
}

int CPVREpg::EpgID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iEpgID;
}

bool CPVREpg::UpdatePending(void) const
{
  CSingleLock lock(m_critSection);
  return m_bUpdatePending;
}

bool CPVREpg::NeedsSave(void) const
{
  CSingleLock lock(m_critSection);
  return !m_changedTags.empty() || !m_deletedTags.empty() || m_bChanged;
}

bool CPVREpg::IsValid(void) const
{
  CSingleLock lock(m_critSection);
  if (ScraperName() == "client")
    return m_channelData->ClientId() != -1 && m_channelData->UniqueClientChannelId() != PVR_CHANNEL_INVALID_UID;

  return true;
}
