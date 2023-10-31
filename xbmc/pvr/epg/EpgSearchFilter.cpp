/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgSearchFilter.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchPath.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/TextSearch.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>

using namespace PVR;

CPVREpgSearchFilter::CPVREpgSearchFilter(bool bRadio)
: m_bIsRadio(bRadio)
{
  Reset();
}

void CPVREpgSearchFilter::Reset()
{
  m_searchData.Reset();
  m_bEpgSearchDataFiltered = false;

  m_bIsCaseSensitive = false;
  m_iMinimumDuration = EPG_SEARCH_UNSET;
  m_iMaximumDuration = EPG_SEARCH_UNSET;
  m_bRemoveDuplicates = false;

  /* pvr specific filters */
  m_iClientID = -1;
  m_iChannelGroupID = -1;
  m_iChannelUID = -1;
  m_bFreeToAirOnly = false;
  m_bIgnorePresentTimers = true;
  m_bIgnorePresentRecordings = true;

  m_groupIdMatches.reset();

  m_iDatabaseId = -1;
  m_title.clear();
  m_lastExecutedDateTime.SetValid(false);
}

std::string CPVREpgSearchFilter::GetPath() const
{
  return CPVREpgSearchPath(*this).GetPath();
}

void CPVREpgSearchFilter::SetSearchTerm(const std::string& strSearchTerm)
{
  if (m_searchData.m_strSearchTerm != strSearchTerm)
  {
    m_searchData.m_strSearchTerm = strSearchTerm;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetSearchPhrase(const std::string& strSearchPhrase)
{
  // match the exact phrase
  SetSearchTerm("\"" + strSearchPhrase + "\"");
}

void CPVREpgSearchFilter::SetCaseSensitive(bool bIsCaseSensitive)
{
  if (m_bIsCaseSensitive != bIsCaseSensitive)
  {
    m_bIsCaseSensitive = bIsCaseSensitive;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetSearchInDescription(bool bSearchInDescription)
{
  if (m_searchData.m_bSearchInDescription != bSearchInDescription)
  {
    m_searchData.m_bSearchInDescription = bSearchInDescription;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetGenreType(int iGenreType)
{
  if (m_searchData.m_iGenreType != iGenreType)
  {
    m_searchData.m_iGenreType = iGenreType;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetMinimumDuration(int iMinimumDuration)
{
  if (m_iMinimumDuration != iMinimumDuration)
  {
    m_iMinimumDuration = iMinimumDuration;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetMaximumDuration(int iMaximumDuration)
{
  if (m_iMaximumDuration != iMaximumDuration)
  {
    m_iMaximumDuration = iMaximumDuration;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetIgnoreFinishedBroadcasts(bool bIgnoreFinishedBroadcasts)
{
  if (m_searchData.m_bIgnoreFinishedBroadcasts != bIgnoreFinishedBroadcasts)
  {
    m_searchData.m_bIgnoreFinishedBroadcasts = bIgnoreFinishedBroadcasts;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetIgnoreFutureBroadcasts(bool bIgnoreFutureBroadcasts)
{
  if (m_searchData.m_bIgnoreFutureBroadcasts != bIgnoreFutureBroadcasts)
  {
    m_searchData.m_bIgnoreFutureBroadcasts = bIgnoreFutureBroadcasts;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetStartDateTime(const CDateTime& startDateTime)
{
  if (m_searchData.m_startDateTime != startDateTime)
  {
    m_searchData.m_startDateTime = startDateTime;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetEndDateTime(const CDateTime& endDateTime)
{
  if (m_searchData.m_endDateTime != endDateTime)
  {
    m_searchData.m_endDateTime = endDateTime;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetIncludeUnknownGenres(bool bIncludeUnknownGenres)
{
  if (m_searchData.m_bIncludeUnknownGenres != bIncludeUnknownGenres)
  {
    m_searchData.m_bIncludeUnknownGenres = bIncludeUnknownGenres;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetRemoveDuplicates(bool bRemoveDuplicates)
{
  if (m_bRemoveDuplicates != bRemoveDuplicates)
  {
    m_bRemoveDuplicates = bRemoveDuplicates;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetClientID(int iClientID)
{
  if (m_iClientID != iClientID)
  {
    m_iClientID = iClientID;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetChannelGroupID(int iChannelGroupID)
{
  if (m_iChannelGroupID != iChannelGroupID)
  {
    m_iChannelGroupID = iChannelGroupID;
    m_groupIdMatches.reset();
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetChannelUID(int iChannelUID)
{
  if (m_iChannelUID != iChannelUID)
  {
    m_iChannelUID = iChannelUID;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetFreeToAirOnly(bool bFreeToAirOnly)
{
  if (m_bFreeToAirOnly != bFreeToAirOnly)
  {
    m_bFreeToAirOnly = bFreeToAirOnly;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetIgnorePresentTimers(bool bIgnorePresentTimers)
{
  if (m_bIgnorePresentTimers != bIgnorePresentTimers)
  {
    m_bIgnorePresentTimers = bIgnorePresentTimers;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetIgnorePresentRecordings(bool bIgnorePresentRecordings)
{
  if (m_bIgnorePresentRecordings != bIgnorePresentRecordings)
  {
    m_bIgnorePresentRecordings = bIgnorePresentRecordings;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetDatabaseId(int iDatabaseId)
{
  if (m_iDatabaseId != iDatabaseId)
  {
    m_iDatabaseId = iDatabaseId;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetTitle(const std::string& title)
{
  if (m_title != title)
  {
    m_title = title;
    m_bChanged = true;
  }
}

void CPVREpgSearchFilter::SetLastExecutedDateTime(const CDateTime& lastExecutedDateTime)
{
  // Note: No need to set m_bChanged here
  m_lastExecutedDateTime = lastExecutedDateTime;
}

bool CPVREpgSearchFilter::MatchGenre(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  if (m_bEpgSearchDataFiltered)
    return true;

  if (m_searchData.m_iGenreType != EPG_SEARCH_UNSET)
  {
    if (m_searchData.m_bIncludeUnknownGenres)
    {
      // match the exact genre and everything with unknown genre
      return (tag->GenreType() == m_searchData.m_iGenreType ||
              tag->GenreType() < EPG_EVENT_CONTENTMASK_MOVIEDRAMA ||
              tag->GenreType() > EPG_EVENT_CONTENTMASK_USERDEFINED);
    }
    else
    {
      // match only the exact genre
      return (tag->GenreType() == m_searchData.m_iGenreType);
    }
  }

  // match any genre
  return true;
}

bool CPVREpgSearchFilter::MatchDuration(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (m_iMinimumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() > m_iMinimumDuration * 60);

  if (bReturn && m_iMaximumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() < m_iMaximumDuration * 60);

  return bReturn;
}

bool CPVREpgSearchFilter::MatchStartAndEndTimes(
    const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  if (m_bEpgSearchDataFiltered)
    return true;

  return ((!m_searchData.m_bIgnoreFinishedBroadcasts ||
           tag->EndAsUTC() > CDateTime::GetUTCDateTime()) &&
          (!m_searchData.m_bIgnoreFutureBroadcasts ||
           tag->StartAsUTC() < CDateTime::GetUTCDateTime()) &&
          (!m_searchData.m_startDateTime.IsValid() || // invalid => match any datetime
           tag->StartAsUTC() >= m_searchData.m_startDateTime) &&
          (!m_searchData.m_endDateTime.IsValid() || // invalid => match any datetime
           tag->EndAsUTC() <= m_searchData.m_endDateTime));
}

bool CPVREpgSearchFilter::MatchSearchTerm(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (!m_searchData.m_strSearchTerm.empty())
  {
    bReturn = !CServiceBroker::GetPVRManager().IsParentalLocked(tag);
    if (bReturn && (m_bIsCaseSensitive || !m_bEpgSearchDataFiltered))
    {
      CTextSearch search(m_searchData.m_strSearchTerm, m_bIsCaseSensitive, SEARCH_DEFAULT_OR);

      bReturn = search.Search(tag->Title()) || search.Search(tag->PlotOutline()) ||
                (m_searchData.m_bSearchInDescription && search.Search(tag->Plot()));
    }
  }

  return bReturn;
}

bool CPVREpgSearchFilter::FilterEntry(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  return MatchGenre(tag) && MatchDuration(tag) && MatchStartAndEndTimes(tag) &&
         MatchSearchTerm(tag) && MatchChannel(tag) && MatchChannelGroup(tag) && MatchTimers(tag) &&
         MatchRecordings(tag) && MatchFreeToAir(tag);
}

void CPVREpgSearchFilter::RemoveDuplicates(std::vector<std::shared_ptr<CPVREpgInfoTag>>& results)
{
  for (auto it = results.begin(); it != results.end();)
  {
    it = results.erase(std::remove_if(results.begin(), results.end(),
                                      [&it](const std::shared_ptr<const CPVREpgInfoTag>& entry) {
                                        return *it != entry && (*it)->Title() == entry->Title() &&
                                               (*it)->Plot() == entry->Plot() &&
                                               (*it)->PlotOutline() == entry->PlotOutline();
                                      }),
                       results.end());
  }
}

bool CPVREpgSearchFilter::MatchChannel(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  return tag && (tag->IsRadio() == m_bIsRadio) &&
         (m_iClientID == -1 || tag->ClientID() == m_iClientID) &&
         (m_iChannelUID == -1 || tag->UniqueChannelID() == m_iChannelUID) &&
         CServiceBroker::GetPVRManager().Clients()->IsCreatedClient(tag->ClientID());
}

bool CPVREpgSearchFilter::MatchChannelGroup(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  if (m_iChannelGroupID != -1)
  {
    if (!m_groupIdMatches.has_value())
    {
      const std::shared_ptr<const CPVRChannelGroup> group = CServiceBroker::GetPVRManager()
                                                                .ChannelGroups()
                                                                ->Get(m_bIsRadio)
                                                                ->GetById(m_iChannelGroupID);
      m_groupIdMatches =
          group && (group->GetByUniqueID({tag->ClientID(), tag->UniqueChannelID()}) != nullptr);
    }

    return *m_groupIdMatches;
  }

  return true;
}

bool CPVREpgSearchFilter::MatchFreeToAir(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  if (m_bFreeToAirOnly)
  {
    const std::shared_ptr<const CPVRChannel> channel =
        CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(tag);
    return channel && !channel->IsEncrypted();
  }

  return true;
}

bool CPVREpgSearchFilter::MatchTimers(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  return (!m_bIgnorePresentTimers || !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(tag));
}

bool CPVREpgSearchFilter::MatchRecordings(const std::shared_ptr<const CPVREpgInfoTag>& tag) const
{
  return (!m_bIgnorePresentRecordings || !CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(tag));
}
