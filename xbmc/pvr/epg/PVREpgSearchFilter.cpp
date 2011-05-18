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

#include "PVREpgSearchFilter.h"
#include "PVREpgContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"

using namespace std;
using namespace PVR;
using namespace EPG;

void PVR::PVREpgSearchFilter::Reset(void)
{
  EpgSearchFilter::Reset();
  m_iChannelNumber           = EPG_SEARCH_UNSET;
  m_bFTAOnly                 = false;
  m_iChannelGroup            = EPG_SEARCH_UNSET;
  m_bIgnorePresentTimers     = true;
  m_bIgnorePresentRecordings = true;
  m_startDateTime            = g_PVREpg->GetFirstEPGDate();
  m_endDateTime              = g_PVREpg->GetLastEPGDate();
}

bool PVR::PVREpgSearchFilter::MatchChannelNumber(const CPVREpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelNumber != EPG_SEARCH_UNSET)
  {
    const CPVRChannelGroup *group = (m_iChannelGroup != EPG_SEARCH_UNSET) ? g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup) : g_PVRChannelGroups->GetGroupAllTV();
    if (!group)
      group = g_PVRChannelGroups->GetGroupAllTV();

    bReturn = (m_iChannelNumber == (int) group->GetChannelNumber(*tag.ChannelTag()));
  }

  return bReturn;
}

bool PVR::PVREpgSearchFilter::MatchChannelGroup(const CPVREpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iChannelGroup != EPG_SEARCH_UNSET)
  {
    const CPVRChannelGroup *group = g_PVRChannelGroups->GetByIdFromAll(m_iChannelGroup);
    bReturn = (group && group->IsGroupMember(tag.ChannelTag()));
  }

  return bReturn;
}

bool PVR::PVREpgSearchFilter::FilterEntry(const CPVREpgInfoTag &tag) const
{
  return EpgSearchFilter::FilterEntry(tag) &&
      MatchChannelNumber(tag) &&
      MatchChannelGroup(tag) &&
      (!m_bFTAOnly || !tag.ChannelTag()->IsEncrypted());
}

int PVR::PVREpgSearchFilter::FilterRecordings(CFileItemList *results)
{
  int iRemoved(0);
  CPVRRecordings *recordings = g_PVRRecordings;

  // TODO not thread safe and inefficient!
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < recordings->size(); iRecordingPtr++)
  {
    CPVRRecording *recording = recordings->at(iRecordingPtr);
    if (!recording)
      continue;

    for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
    {
      const CPVREpgInfoTag *epgentry  = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();

      /* no match */
      if (!epgentry ||
          epgentry->Title()       != recording->m_strTitle ||
          epgentry->Plot()        != recording->m_strPlot)
        continue;

      results->Remove(iResultPtr);
      iResultPtr--;
      ++iRemoved;
    }
  }

  return iRemoved;
}

int PVR::PVREpgSearchFilter::FilterTimers(CFileItemList *results)
{
  int iRemoved(0);
  CPVRTimers *timers = g_PVRTimers;

  // TODO not thread safe and inefficient!
  for (unsigned int iTimerPtr = 0; iTimerPtr < timers->size(); iTimerPtr++)
  {
    CPVRTimerInfoTag *timer = timers->at(iTimerPtr);
    if (!timer)
      continue;

    for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
    {
      const CPVREpgInfoTag *epgentry = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();
      if (!epgentry ||
          *epgentry->ChannelTag() != *timer->m_channel ||
          epgentry->StartAsUTC()   <  timer->StartAsUTC() ||
          epgentry->EndAsUTC()     >  timer->EndAsUTC())
        continue;

      results->Remove(iResultPtr);
      iResultPtr--;
      ++iRemoved;
    }
  }

  return iRemoved;
}
