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
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
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
  m_bIncludeUnknownGenres = false;
  m_bRemoveDuplicates = false;

  /* pvr specific filters */
  m_channelNumber = CPVRChannelNumber();
  m_bFreeToAirOnly = false;
  m_iChannelGroup = EPG_SEARCH_UNSET;
  m_bIgnorePresentTimers = true;
  m_bIgnorePresentRecordings = true;
}

bool CPVREpgSearchFilter::MatchGenre(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (m_searchData.m_iGenreType != EPG_SEARCH_UNSET)
  {
    bool bIsUnknownGenre(tag->GenreType() > EPG_EVENT_CONTENTMASK_USERDEFINED ||
                         tag->GenreType() < EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
    bReturn = ((m_bIncludeUnknownGenres && bIsUnknownGenre) || m_bEpgSearchDataFiltered ||
               tag->GenreType() == m_searchData.m_iGenreType);
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchDuration(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (m_iMinimumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() > m_iMinimumDuration * 60);

  if (bReturn && m_iMaximumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() < m_iMaximumDuration * 60);

  return bReturn;
}

bool CPVREpgSearchFilter::MatchStartAndEndTimes(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  if (m_bEpgSearchDataFiltered)
    return true;

  return (tag->StartAsLocalTime() >= m_searchData.m_startDateTime &&
          tag->EndAsLocalTime() <= m_searchData.m_endDateTime);
}

void CPVREpgSearchFilter::SetSearchPhrase(const std::string& strSearchPhrase)
{
  // match the exact phrase
  m_searchData.m_strSearchTerm = "\"";
  m_searchData.m_strSearchTerm.append(strSearchPhrase);
  m_searchData.m_strSearchTerm.append("\"");
}

bool CPVREpgSearchFilter::MatchSearchTerm(const std::shared_ptr<CPVREpgInfoTag>& tag) const
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

bool CPVREpgSearchFilter::MatchBroadcastId(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  if (m_bEpgSearchDataFiltered)
    return true;

  if (m_searchData.m_iUniqueBroadcastId != EPG_TAG_INVALID_UID)
    return (tag->UniqueBroadcastID() == m_searchData.m_iUniqueBroadcastId);

  return true;
}

bool CPVREpgSearchFilter::FilterEntry(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  return (MatchGenre(tag) &&
      MatchBroadcastId(tag) &&
      MatchDuration(tag) &&
      MatchStartAndEndTimes(tag) &&
      MatchSearchTerm(tag) &&
      MatchTimers(tag) &&
      MatchRecordings(tag)) &&
      MatchChannelType(tag) &&
      MatchChannelNumber(tag) &&
      MatchChannelGroup(tag) &&
      MatchFreeToAir(tag);
}

void CPVREpgSearchFilter::RemoveDuplicates(std::vector<std::shared_ptr<CPVREpgInfoTag>>& results)
{
  for (auto it = results.begin(); it != results.end();)
  {
    it = results.erase(std::remove_if(results.begin(),
                                      results.end(),
                                      [&it](const std::shared_ptr<CPVREpgInfoTag>& entry)
                                      {
                                         return *it != entry &&
                                                (*it)->Title() == entry->Title() &&
                                                (*it)->Plot() == entry->Plot() &&
                                                (*it)->PlotOutline() == entry->PlotOutline();
                                      }),
                       results.end());
  }
}

bool CPVREpgSearchFilter::MatchChannelType(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  return tag && (tag->IsRadio() == m_bIsRadio);
}

bool CPVREpgSearchFilter::MatchChannelNumber(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (m_channelNumber.IsValid())
  {
    const std::shared_ptr<CPVRChannelGroupsContainer> groups =
        CServiceBroker::GetPVRManager().ChannelGroups();
    const std::shared_ptr<CPVRChannelGroup> group =
        (m_iChannelGroup == EPG_SEARCH_UNSET) ? groups->GetGroupAll(m_bIsRadio)
                                              : groups->Get(m_bIsRadio)->GetById(m_iChannelGroup);
    if (group)
    {
      const std::shared_ptr<CPVRChannelGroupMember>& groupMember =
          group->GetByUniqueID({tag->ClientID(), tag->UniqueChannelID()});
      bReturn = m_channelNumber == groupMember->ChannelNumber();
    }
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchChannelGroup(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  bool bReturn(true);

  if (m_iChannelGroup != EPG_SEARCH_UNSET)
  {
    std::shared_ptr<CPVRChannelGroup> group = CServiceBroker::GetPVRManager().ChannelGroups()->GetByIdFromAll(m_iChannelGroup);
    if (group)
    {
      const std::shared_ptr<CPVRChannel> channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(tag);
      bReturn = (channel && group->IsGroupMember(channel));
    }
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchFreeToAir(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  if (m_bFreeToAirOnly)
  {
    const std::shared_ptr<CPVRChannel> channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(tag);
    return channel && !channel->IsEncrypted();
  }

  return true;
}

bool CPVREpgSearchFilter::MatchTimers(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  return (!m_bIgnorePresentTimers || !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(tag));
}

bool CPVREpgSearchFilter::MatchRecordings(const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  return (!m_bIgnorePresentRecordings || !CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(tag));
}
