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
#include "utils/TextSearch.h"
#include "utils/log.h"
#include "FileItem.h"
#include "../addons/include/xbmc_pvr_types.h"

#include "EpgSearchFilter.h"
#include "EpgContainer.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace std;
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
  m_bIgnorePresentTimers     = false;
  m_bIgnorePresentRecordings = false;
  m_bPreventRepeats          = false;

  /* pvr specific filters */
  m_iChannelNumber           = EPG_SEARCH_UNSET;
  m_bFTAOnly                 = false;
  m_iChannelGroup            = EPG_SEARCH_UNSET;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
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

  if (!m_strSearchTerm.IsEmpty())
  {
    CTextSearch search(m_strSearchTerm, m_bIsCaseSensitive, SEARCH_DEFAULT_OR);
    bReturn = search.Search(tag.Title()) ||
        search.Search(tag.PlotOutline());
  }

  return bReturn;
}

bool EpgSearchFilter::FilterEntry(const CEpgInfoTag &tag) const
{
  return (MatchGenre(tag) &&
      MatchDuration(tag) &&
      MatchStartAndEndTimes(tag) &&
      MatchSearchTerm(tag)) &&
      (!tag.HasPVRChannel() ||
      (MatchChannelNumber(tag) &&
       MatchChannelGroup(tag) &&
       (!m_bFTAOnly || !tag.ChannelTag()->IsEncrypted())));
}

int EpgSearchFilter::RemoveDuplicates(CFileItemList &results)
{
  unsigned int iSize = results.Size();

  for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
  {
    const CEpgInfoTag *epgentry_1 = results.Get(iResultPtr)->GetEPGInfoTag();
    for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
    {
      const CEpgInfoTag *epgentry_2 = results.Get(iTagPtr)->GetEPGInfoTag();
      if (iResultPtr == iTagPtr)
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


bool EpgSearchFilter::MatchChannelNumber(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelNumber != EPG_SEARCH_UNSET && g_PVRManager.IsStarted())
  {
    const CPVRChannelGroup *group = (m_iChannelGroup != EPG_SEARCH_UNSET) ? g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup) : g_PVRChannelGroups->GetGroupAllTV();
    if (!group)
      group = CPVRManager::Get().ChannelGroups()->GetGroupAllTV();

    bReturn = (m_iChannelNumber == (int) group->GetChannelNumber(*tag.ChannelTag()));
  }

  return bReturn;
}

bool EpgSearchFilter::MatchChannelGroup(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelGroup != EPG_SEARCH_UNSET && g_PVRManager.IsStarted())
  {
    const CPVRChannelGroup *group = g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup);
    bReturn = (group && group->IsGroupMember(*tag.ChannelTag()));
  }

  return bReturn;
}

int EpgSearchFilter::FilterRecordings(CFileItemList &results)
{
  int iRemoved(0);
  if (!g_PVRManager.IsStarted())
    return iRemoved;

  CPVRRecordings *recordings = CPVRManager::Get().Recordings();

  // TODO not thread safe and inefficient!
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < recordings->size(); iRecordingPtr++)
  {
    CPVRRecording *recording = recordings->at(iRecordingPtr);
    if (!recording)
      continue;

    for (int iResultPtr = 0; iResultPtr < results.Size(); iResultPtr++)
    {
      const CEpgInfoTag *epgentry  = results.Get(iResultPtr)->GetEPGInfoTag();

      /* no match */
      if (!epgentry ||
          epgentry->Title()       != recording->m_strTitle ||
          epgentry->Plot()        != recording->m_strPlot)
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

  CPVRTimers *timers = CPVRManager::Get().Timers();

  // TODO not thread safe and inefficient!
  for (unsigned int iTimerPtr = 0; iTimerPtr < timers->size(); iTimerPtr++)
  {
    CPVRTimerInfoTag *timer = timers->at(iTimerPtr);
    if (!timer)
      continue;

    for (int iResultPtr = 0; iResultPtr < results.Size(); iResultPtr++)
    {
      const CEpgInfoTag *epgentry = results.Get(iResultPtr)->GetEPGInfoTag();
      if (!epgentry ||
          *epgentry->ChannelTag() != *timer->m_channel ||
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
