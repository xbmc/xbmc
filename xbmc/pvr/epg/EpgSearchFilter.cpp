/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "ServiceBroker.h"
#include "utils/TextSearch.h"
#include "utils/log.h"

#include "EpgContainer.h"
#include "EpgSearchFilter.h"

using namespace PVR;


CPVREpgSearchFilter::CPVREpgSearchFilter()
{
  Reset();
}

void CPVREpgSearchFilter::Reset()
{
  m_strSearchTerm.clear();
  m_bIsCaseSensitive         = false;
  m_bSearchInDescription     = false;
  m_iGenreType               = EPG_SEARCH_UNSET;
  m_iGenreSubType            = EPG_SEARCH_UNSET;
  m_iMinimumDuration         = EPG_SEARCH_UNSET;
  m_iMaximumDuration         = EPG_SEARCH_UNSET;
  m_bIncludeUnknownGenres    = false;
  m_bRemoveDuplicates        = false;

  /* pvr specific filters */
  m_bIsRadio                 = false;
  m_iChannelNumber           = EPG_SEARCH_UNSET;
  m_bFreeToAirOnly           = false;
  m_iChannelGroup            = EPG_SEARCH_UNSET;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
  m_iUniqueBroadcastId       = EPG_TAG_INVALID_UID;
}

void CPVREpgSearchFilter::ValidateStartAndEndDate()
{
  const CDateTime start = CServiceBroker::GetPVRManager().EpgContainer().GetFirstEPGDate();
  const CDateTime end = CServiceBroker::GetPVRManager().EpgContainer().GetLastEPGDate();

  if (m_startDateTime < start || m_startDateTime > end)
    m_startDateTime.SetFromUTCDateTime(start);

  if (m_endDateTime > end || m_endDateTime < start)
    m_endDateTime.SetFromUTCDateTime(end);
}

const CDateTime& CPVREpgSearchFilter::GetStartDateTime()
{
  ValidateStartAndEndDate();
  return m_startDateTime;
}

const CDateTime& CPVREpgSearchFilter::GetEndDateTime()
{
  ValidateStartAndEndDate();
  return m_endDateTime;
}

bool CPVREpgSearchFilter::MatchGenre(const CPVREpgInfoTagPtr &tag) const
{
  bool bReturn(true);

  if (m_iGenreType != EPG_SEARCH_UNSET)
  {
    bool bIsUnknownGenre(tag->GenreType() > EPG_EVENT_CONTENTMASK_USERDEFINED ||
                         tag->GenreType() < EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
    bReturn = ((m_bIncludeUnknownGenres && bIsUnknownGenre) || tag->GenreType() == m_iGenreType);
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchDuration(const CPVREpgInfoTagPtr &tag) const
{
  bool bReturn(true);

  if (m_iMinimumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() > m_iMinimumDuration * 60);

  if (bReturn && m_iMaximumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag->GetDuration() < m_iMaximumDuration * 60);

  return bReturn;
}

bool CPVREpgSearchFilter::MatchStartAndEndTimes(const CPVREpgInfoTagPtr &tag) const
{
  return (tag->StartAsLocalTime() >= m_startDateTime && tag->EndAsLocalTime() <= m_endDateTime);
}

void CPVREpgSearchFilter::SetSearchPhrase(const std::string &strSearchPhrase)
{
  // match the exact phrase
  m_strSearchTerm = "\"";
  m_strSearchTerm.append(strSearchPhrase);
  m_strSearchTerm.append("\"");
}

bool CPVREpgSearchFilter::MatchSearchTerm(const CPVREpgInfoTagPtr &tag) const
{
  bool bReturn(true);

  if (!m_strSearchTerm.empty())
  {
    CTextSearch search(m_strSearchTerm, m_bIsCaseSensitive, SEARCH_DEFAULT_OR);
    bReturn = search.Search(tag->Title()) ||
              search.Search(tag->PlotOutline()) ||
              (m_bSearchInDescription && search.Search(tag->Plot()));
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchBroadcastId(const CPVREpgInfoTagPtr &tag) const
{
  if (m_iUniqueBroadcastId != EPG_TAG_INVALID_UID)
    return (tag->UniqueBroadcastID() == m_iUniqueBroadcastId);

  return true;
}

bool CPVREpgSearchFilter::FilterEntry(const CPVREpgInfoTagPtr &tag) const
{
  return (MatchGenre(tag) &&
      MatchBroadcastId(tag) &&
      MatchDuration(tag) &&
      MatchStartAndEndTimes(tag) &&
      MatchSearchTerm(tag) &&
      MatchTimers(tag) &&
      MatchRecordings(tag)) &&
      (!tag->HasPVRChannel() ||
       (MatchChannelType(tag) &&
        MatchChannelNumber(tag) &&
        MatchChannelGroup(tag) &&
        MatchFreeToAir(tag)));
}

int CPVREpgSearchFilter::RemoveDuplicates(CFileItemList &results)
{
  unsigned int iSize = results.Size();

  for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
  {
    const CPVREpgInfoTagPtr epgentry_1(results.Get(iResultPtr)->GetEPGInfoTag());
    if (!epgentry_1)
      continue;

    for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
    {
      if (iResultPtr == iTagPtr)
        continue;

      const CPVREpgInfoTagPtr epgentry_2(results.Get(iTagPtr)->GetEPGInfoTag());
      if (!epgentry_2)
        continue;

      if (epgentry_1->Title()       != epgentry_2->Title() ||
          epgentry_1->Plot()        != epgentry_2->Plot() ||
          epgentry_1->PlotOutline() != epgentry_2->PlotOutline())
        continue;

      results.Remove(iTagPtr);
      iResultPtr--;
      iTagPtr--;
      iSize--;
    }
  }

  return iSize;
}

bool CPVREpgSearchFilter::MatchChannelType(const CPVREpgInfoTagPtr &tag) const
{
  return (CServiceBroker::GetPVRManager().IsStarted() && tag->ChannelTag()->IsRadio() == m_bIsRadio);
}

bool CPVREpgSearchFilter::MatchChannelNumber(const CPVREpgInfoTagPtr &tag) const
{
  bool bReturn(true);

  if (m_iChannelNumber != EPG_SEARCH_UNSET && CServiceBroker::GetPVRManager().IsStarted())
  {
    CPVRChannelGroupPtr group = (m_iChannelGroup != EPG_SEARCH_UNSET) ? CServiceBroker::GetPVRManager().ChannelGroups()->GetByIdFromAll(m_iChannelGroup) : CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
    if (!group)
      group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();

    bReturn = (m_iChannelNumber == (int) group->GetChannelNumber(tag->ChannelTag()));
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchChannelGroup(const CPVREpgInfoTagPtr &tag) const
{
  bool bReturn(true);

  if (m_iChannelGroup != EPG_SEARCH_UNSET && CServiceBroker::GetPVRManager().IsStarted())
  {
    CPVRChannelGroupPtr group = CServiceBroker::GetPVRManager().ChannelGroups()->GetByIdFromAll(m_iChannelGroup);
    bReturn = (group && group->IsGroupMember(tag->ChannelTag()));
  }

  return bReturn;
}

bool CPVREpgSearchFilter::MatchFreeToAir(const CPVREpgInfoTagPtr &tag) const
{
  return (!m_bFreeToAirOnly || !tag->ChannelTag()->IsEncrypted());
}

bool CPVREpgSearchFilter::MatchTimers(const CPVREpgInfoTagPtr &tag) const
{
  return (!m_bIgnorePresentTimers || !CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(tag));
}

bool CPVREpgSearchFilter::MatchRecordings(const CPVREpgInfoTagPtr &tag) const
{
  return (!m_bIgnorePresentRecordings || !CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(tag));
}
