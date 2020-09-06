/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Epg.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <memory>
#include <utility>
#include <vector>

using namespace PVR;

CPVREpg::CPVREpg(int iEpgID,
                 const std::string& strName,
                 const std::string& strScraperName,
                 const std::shared_ptr<CPVREpgDatabase>& database)
  : m_iEpgID(iEpgID),
    m_strName(strName),
    m_strScraperName(strScraperName),
    m_channelData(new CPVREpgChannelData),
    m_tags(m_iEpgID, m_channelData, database)
{
}

CPVREpg::CPVREpg(int iEpgID,
                 const std::string& strName,
                 const std::string& strScraperName,
                 const std::shared_ptr<CPVREpgChannelData>& channelData,
                 const std::shared_ptr<CPVREpgDatabase>& database)
  : m_bChanged(true),
    m_iEpgID(iEpgID),
    m_strName(strName),
    m_strScraperName(strScraperName),
    m_channelData(channelData),
    m_tags(m_iEpgID, m_channelData, database)
{
}

CPVREpg::~CPVREpg()
{
  Clear();
}

void CPVREpg::ForceUpdate()
{
  {
    CSingleLock lock(m_critSection);
    m_bUpdatePending = true;
  }

  m_events.Publish(PVREvent::EpgUpdatePending);
}

void CPVREpg::Clear()
{
  CSingleLock lock(m_critSection);
  m_tags.Clear();
}

void CPVREpg::Cleanup(int iPastDays)
{
  const CDateTime cleanupTime = CDateTime::GetUTCDateTime() - CDateTimeSpan(iPastDays, 0, 0, 0);
  Cleanup(cleanupTime);
}

void CPVREpg::Cleanup(const CDateTime& time)
{
  CSingleLock lock(m_critSection);
  m_tags.Cleanup(time);
}

std::shared_ptr<CPVREpgInfoTag> CPVREpg::GetTagNow(bool bUpdateIfNeeded /* = true */) const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetActiveTag(bUpdateIfNeeded);
}

std::shared_ptr<CPVREpgInfoTag> CPVREpg::GetTagNext() const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetNextStartingTag();
}

std::shared_ptr<CPVREpgInfoTag> CPVREpg::GetTagPrevious() const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetLastEndedTag();
}

bool CPVREpg::CheckPlayingEvent()
{
  const std::shared_ptr<CPVREpgInfoTag> previousTag = GetTagNow(false);
  const std::shared_ptr<CPVREpgInfoTag> newTag = GetTagNow(true);

  bool bTagChanged = newTag && (!previousTag || *previousTag != *newTag);
  bool bTagRemoved = !newTag && previousTag;
  if (bTagChanged || bTagRemoved)
  {
    m_events.Publish(PVREvent::EpgActiveItem);
    return true;
  }
  return false;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpg::GetTagByBroadcastId(unsigned int iUniqueBroadcastId) const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetTag(iUniqueBroadcastId);
}

std::shared_ptr<CPVREpgInfoTag> CPVREpg::GetTagBetween(const CDateTime& beginTime, const CDateTime& endTime, bool bUpdateFromClient /* = false */)
{
  std::shared_ptr<CPVREpgInfoTag> tag;

  CSingleLock lock(m_critSection);
  tag = m_tags.GetTagBetween(beginTime, endTime);

  if (!tag && bUpdateFromClient)
  {
    // not found locally; try to fetch from client
    time_t b;
    beginTime.GetAsTime(b);
    time_t e;
    endTime.GetAsTime(e);

    const std::shared_ptr<CPVREpg> tmpEpg = std::make_shared<CPVREpg>(
        m_iEpgID, m_strName, m_strScraperName, m_channelData, std::shared_ptr<CPVREpgDatabase>());
    if (tmpEpg->UpdateFromScraper(b, e, true))
      tag = tmpEpg->GetTagBetween(beginTime, endTime, false);

    if (tag)
      m_tags.UpdateEntry(tag);
  }

  return tag;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpg::GetTimeline(
    const CDateTime& timelineStart,
    const CDateTime& timelineEnd,
    const CDateTime& minEventEnd,
    const CDateTime& maxEventStart) const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetTimeline(timelineStart, timelineEnd, minEventEnd, maxEventStart);
}

bool CPVREpg::UpdateEntries(const CPVREpg& epg)
{
  CSingleLock lock(m_critSection);

  /* copy over tags */
  m_tags.UpdateEntries(epg.m_tags);

  /* update the last scan time of this table */
  m_lastScanTime = CDateTime::GetUTCDateTime();
  m_bUpdateLastScanTime = true;

  m_events.Publish(PVREvent::Epg);
  return true;
}

namespace
{

bool IsTagExpired(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  // Respect epg linger time.
  const int iPastDays = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_EPG_PAST_DAYSTODISPLAY);
  const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(iPastDays, 0, 0, 0));

  return tag->EndAsUTC() < cleanupTime;
}

} // unnamed namespace

bool CPVREpg::UpdateEntry(const EPG_TAG* data, int iClientId)
{
  if (!data)
    return false;

  const std::shared_ptr<CPVREpgInfoTag> tag =
      std::make_shared<CPVREpgInfoTag>(*data, iClientId, m_channelData, m_iEpgID);

  return !IsTagExpired(tag) && m_tags.UpdateEntry(tag);
}

bool CPVREpg::UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE newState)
{
  bool bRet = true;
  bool bNotify = true;

  if (newState == EPG_EVENT_CREATED || newState == EPG_EVENT_UPDATED)
  {
    bRet = !IsTagExpired(tag) && m_tags.UpdateEntry(tag);
  }
  else if (newState == EPG_EVENT_DELETED)
  {
    CSingleLock lock(m_critSection);
    const std::shared_ptr<CPVREpgInfoTag> existingTag = m_tags.GetTag(tag->UniqueBroadcastID());
    if (!existingTag)
    {
      bRet = false;
    }
    else
    {
      if (IsTagExpired(existingTag))
      {
        m_tags.DeleteEntry(existingTag);
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
    m_events.Publish(PVREvent::EpgItemUpdate);

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

  /* clean up if needed */
  Cleanup(iPastDays);

  /* enforce advanced settings update interval override for channels with no EPG data */
  if (m_tags.IsEmpty() && !bUpdate && ChannelID() > 0)
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

  if (!bGrabSuccess)
    CLog::LogF(LOGERROR, "Failed to update table '%s'", Name().c_str());

  CSingleLock lock(m_critSection);
  m_bUpdatePending = false;

  return bGrabSuccess;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpg::GetTags() const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetAllTags();
}

bool CPVREpg::QueuePersistQuery(const std::shared_ptr<CPVREpgDatabase>& database)
{
  // Note: It is guaranteed that both this EPG instance and database instance are already
  //       locked when this method gets called! No additional locking is needed here!

  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  if (m_iEpgID <= 0 || m_bChanged)
  {
    const int iId = database->Persist(*this, m_iEpgID > 0);
    if (iId > 0 && m_iEpgID != iId)
    {
      m_iEpgID = iId;
      m_tags.SetEpgID(iId);
    }
  }

  if (m_tags.NeedsSave())
    m_tags.QueuePersistQuery();

  if (m_bUpdateLastScanTime)
    database->QueuePersistLastEpgScanTimeQuery(m_iEpgID, m_lastScanTime);

  m_bChanged = false;
  m_bUpdateLastScanTime = false;

  return true;
}

bool CPVREpg::Delete(const std::shared_ptr<CPVREpgDatabase>& database)
{
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  // delete own epg db entry
  database->Delete(*this);

  // delete all tags for this epg from db
  m_tags.Delete();

  Clear();

  return true;
}

CDateTime CPVREpg::GetFirstDate() const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetFirstStartTime();
}

CDateTime CPVREpg::GetLastDate() const
{
  CSingleLock lock(m_critSection);
  return m_tags.GetLastEndTime();
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

  const std::shared_ptr<CPVREpg> tmpEpg = std::make_shared<CPVREpg>(
      m_iEpgID, m_strName, m_strScraperName, m_channelData, std::shared_ptr<CPVREpgDatabase>());
  if (tmpEpg->UpdateFromScraper(start, end, bForceUpdate))
    bReturn = UpdateEntries(*tmpEpg);

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
  m_tags.SetChannelData(data);
}

int CPVREpg::ChannelID() const
{
  CSingleLock lock(m_critSection);
  return m_channelData->ChannelId();
}

const std::string& CPVREpg::ScraperName() const
{
  CSingleLock lock(m_critSection);
  return m_strScraperName;
}

const std::string& CPVREpg::Name() const
{
  CSingleLock lock(m_critSection);
  return m_strName;
}

int CPVREpg::EpgID() const
{
  CSingleLock lock(m_critSection);
  return m_iEpgID;
}

bool CPVREpg::UpdatePending() const
{
  CSingleLock lock(m_critSection);
  return m_bUpdatePending;
}

bool CPVREpg::NeedsSave() const
{
  CSingleLock lock(m_critSection);
  return m_bChanged || m_tags.NeedsSave();
}

bool CPVREpg::IsValid() const
{
  CSingleLock lock(m_critSection);
  if (ScraperName() == "client")
    return m_channelData->ClientId() != -1 && m_channelData->UniqueClientChannelId() != PVR_CHANNEL_INVALID_UID;

  return true;
}
