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
#include "addons/include/xbmc_pvr_types.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/TextSearch.h"
#include "utils/log.h"

#include "EpgContainer.h"
#include "EpgSearchFilter.h"

using namespace EPG;
using namespace PVR;

void EpgSearchFilter::Reset()
{
  m_strSearchTerm            = "";
  m_bIsCaseSensitive         = false;
  m_bSearchInDescription     = false;
  m_iGenreType               = EPG_SEARCH_UNSET;
  m_iGenreSubType            = EPG_SEARCH_UNSET;
  m_iMinimumDuration         = EPG_SEARCH_UNSET;
  m_iMaximumDuration         = EPG_SEARCH_UNSET;
  m_startDateTime.SetFromUTCDateTime(g_EpgContainer.GetFirstEPGDate());
  m_endDateTime.SetFromUTCDateTime(g_EpgContainer.GetLastEPGDate());
  m_bIncludeUnknownGenres    = false;
  m_bPreventRepeats          = false;

  /* pvr specific filters */
  m_iChannelNumber           = EPG_SEARCH_UNSET;
  m_bFTAOnly                 = false;
  m_iChannelGroup            = EPG_SEARCH_UNSET;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
  m_iUniqueBroadcastId	     = EPG_SEARCH_UNSET;
}

bool EpgSearchFilter::MatchGenre(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iGenreType != EPG_SEARCH_UNSET)
  {
    bool bIsUnknownGenre(tag.GenreType() > EPG_EVENT_CONTENTMASK_USERDEFINED ||
        tag.GenreType() < EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
    bReturn = ((m_bIncludeUnknownGenres && bIsUnknownGenre) || tag.GenreType() == m_iGenreType);
  }

  return bReturn;
}

bool EpgSearchFilter::MatchDuration(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iMinimumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag.GetDuration() > m_iMinimumDuration * 60);

  if (bReturn && m_iMaximumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag.GetDuration() < m_iMaximumDuration * 60);

  return bReturn;
}

bool EpgSearchFilter::MatchStartAndEndTimes(const CEpgInfoTag &tag) const
{
  return (tag.StartAsLocalTime() >= m_startDateTime && tag.EndAsLocalTime() <= m_endDateTime);
}

bool EpgSearchFilter::MatchSearchTerm(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (!m_strSearchTerm.empty())
  {
    CTextSearch search(m_strSearchTerm, m_bIsCaseSensitive, SEARCH_DEFAULT_OR);
    bReturn = search.Search(tag.Title()) ||
        search.Search(tag.PlotOutline());
  }

  return bReturn;
}

bool EpgSearchFilter::MatchBroadcastId(const CEpgInfoTag &tag) const
{
  if (m_iUniqueBroadcastId != EPG_SEARCH_UNSET)
    return (tag.UniqueBroadcastID() == m_iUniqueBroadcastId);

  return true;
}

bool EpgSearchFilter::FilterEntry(const CEpgInfoTag &tag) const
{
  return (MatchGenre(tag) &&
      MatchBroadcastId(tag) &&
      MatchDuration(tag) &&
      MatchStartAndEndTimes(tag) &&
      MatchSearchTerm(tag)) &&
      (!tag.HasPVRChannel() ||
       (MatchChannelType(tag) &&
        MatchChannelNumber(tag) &&
        MatchChannelGroup(tag) &&
        (!m_bFTAOnly || !tag.ChannelTag()->IsEncrypted())));
}

int EpgSearchFilter::RemoveDuplicates(CFileItemList &results)
{
  unsigned int iSize = results.Size();

  for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
  {
    const CEpgInfoTagPtr epgentry_1(results.Get(iResultPtr)->GetEPGInfoTag());
    if (!epgentry_1)
      continue;

    for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
    {
      if (iResultPtr == iTagPtr)
        continue;

      const CEpgInfoTagPtr epgentry_2(results.Get(iTagPtr)->GetEPGInfoTag());
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

bool EpgSearchFilter::MatchChannelType(const CEpgInfoTag &tag) const
{
  return (g_PVRManager.IsStarted() && tag.ChannelTag()->IsRadio() == m_bIsRadio);
}

bool EpgSearchFilter::MatchChannelNumber(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelNumber != EPG_SEARCH_UNSET && g_PVRManager.IsStarted())
  {
    CPVRChannelGroupPtr group = (m_iChannelGroup != EPG_SEARCH_UNSET) ? g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup) : g_PVRChannelGroups->GetGroupAllTV();
    if (!group)
      group = CPVRManager::Get().ChannelGroups()->GetGroupAllTV();

    bReturn = (m_iChannelNumber == (int) group->GetChannelNumber(tag.ChannelTag()));
  }

  return bReturn;
}

bool EpgSearchFilter::MatchChannelGroup(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelGroup != EPG_SEARCH_UNSET && g_PVRManager.IsStarted())
  {
    CPVRChannelGroupPtr group = g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup);
    bReturn = (group && group->IsGroupMember(tag.ChannelTag()));
  }

  return bReturn;
}

int EpgSearchFilter::FilterRecordings(CFileItemList &results)
{
  int iRemoved(0);
  if (!g_PVRManager.IsStarted())
    return iRemoved;

  CFileItemList recordings;
  g_PVRRecordings->GetAll(recordings);

  // TODO inefficient!
  CPVRRecordingPtr recording;
  for (int iRecordingPtr = 0; iRecordingPtr < recordings.Size(); iRecordingPtr++)
  {
    recording = recordings.Get(iRecordingPtr)->GetPVRRecordingInfoTag();
    if (!recording)
      continue;

    for (int iResultPtr = 0; iResultPtr < results.Size(); iResultPtr++)
    {
      const CEpgInfoTagPtr epgentry(results.Get(iResultPtr)->GetEPGInfoTag());

      /* no match */
      if (!epgentry ||
          epgentry->Title() != recording->m_strTitle ||
          epgentry->Plot()  != recording->m_strPlot)
        continue;

      results.Remove(iResultPtr);
      iResultPtr--;
      ++iRemoved;
    }
  }

  return iRemoved;
}

int EpgSearchFilter::FilterTimers(CFileItemList &results)
{
  int iRemoved(0);
  if (!g_PVRManager.IsStarted())
    return iRemoved;

  std::vector<CFileItemPtr> timers = g_PVRTimers->GetActiveTimers();
  // TODO inefficient!
  for (unsigned int iTimerPtr = 0; iTimerPtr < timers.size(); iTimerPtr++)
  {
    CFileItemPtr fileItem = timers.at(iTimerPtr);
    if (!fileItem || !fileItem->HasPVRTimerInfoTag())
      continue;

    CPVRTimerInfoTagPtr timer = fileItem->GetPVRTimerInfoTag();
    if (!timer)
      continue;

    for (int iResultPtr = 0; iResultPtr < results.Size(); iResultPtr++)
    {
      const CEpgInfoTagPtr epgentry(results.Get(iResultPtr)->GetEPGInfoTag());
      if (!epgentry ||
          *epgentry->ChannelTag() != *timer->ChannelTag() ||
          epgentry->StartAsUTC()   <  timer->StartAsUTC() ||
          epgentry->EndAsUTC()     >  timer->EndAsUTC())
        continue;

      results.Remove(iResultPtr);
      iResultPtr--;
      ++iRemoved;
    }
  }

  return iRemoved;
}
