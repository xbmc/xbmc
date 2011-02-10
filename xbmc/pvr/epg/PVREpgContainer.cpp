/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PVREpgContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/recordings/PVRRecording.h"

using namespace std;

CPVREpgContainer g_PVREpgContainer;

void CPVREpgContainer::Clear(bool bClearDb /* = false */)
{
  // XXX stop the timers from being updated while clearing tags
  /* remove all pointers to epg tables on timers */
  for (unsigned int iTimerPtr = 0; iTimerPtr < g_PVRTimers.size(); iTimerPtr++)
    g_PVRTimers.at(iTimerPtr)->SetEpgInfoTag(NULL);

  CEpgContainer::Clear(bClearDb);
}

bool CPVREpgContainer::CreateChannelEpgs(void)
{
  for (int radio = 0; radio <= 1; radio++)
  {
    const CPVRChannelGroup *channels = CPVRManager::GetChannelGroups()->GetGroupAll(radio == 1);
    for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
    {
      CEpg *epg = GetById(channels->at(iChannelPtr)->ChannelID());
      if (!epg)
        channels->at(iChannelPtr)->GetEPG();
      else
      {
        channels->at(iChannelPtr)->m_EPG = (CPVREpg *) epg;
        epg->m_Channel = channels->at(iChannelPtr);
      }
    }
  }

  return true;
}

int CPVREpgContainer::GetEPGAll(CFileItemList* results, bool bRadio /* = false */)
{
  int iInitialSize = results->Size();

  for (unsigned int iEpgPtr = 0; iEpgPtr < size(); iEpgPtr++)
  {
    CPVREpg *epg = (CPVREpg *) at(iEpgPtr);
    CPVRChannel *channel = (CPVRChannel *) epg->Channel();
    if (!channel || channel->IsRadio() != bRadio)
      continue;

    epg->Get(results);
  }

  return results->Size() - iInitialSize;
}

void CPVREpgContainer::UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag)
{
  CEpgContainer::UpdateFirstAndLastEPGDates(tag);

  if (!tag.ChannelTag())
    return;

  if (tag.ChannelTag()->IsRadio())
  {
    if (tag.Start() < m_First)
      m_RadioFirst = tag.Start();
    if (tag.End() > m_Last)
      m_RadioLast = tag.End();
  }
  else
  {
    if (tag.Start() < m_First)
      m_TVFirst = tag.Start();
    if (tag.End() > m_Last)
      m_TVLast = tag.End();
  }
}

bool CPVREpgContainer::AutoCreateTablesHook(void)
{
  return CreateChannelEpgs();
}

CEpg* CPVREpgContainer::CreateEpg(int iEpgId)
{
  CPVRChannel *channel = (CPVRChannel *) CPVRManager::GetChannelGroups()->GetChannelById(iEpgId);
  if (channel)
  {
    return new CPVREpg(channel);
  }
  else
  {
    CLog::Log(LOGERROR, "PVREpgContainer - %s - cannot find channel '%d'. not creating an EPG table.",
        __FUNCTION__, iEpgId);
    return NULL;
  }
}

const CDateTime &CPVREpgContainer::GetFirstEPGDate(bool bRadio /* = false */)
{
  return bRadio ? m_RadioFirst : m_TVFirst;
}

const CDateTime &CPVREpgContainer::GetLastEPGDate(bool bRadio /* = false */)
{
  return bRadio ? m_RadioLast : m_TVLast;
}

int CPVREpgContainer::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  CEpgContainer::GetEPGSearch(results, filter);

  /* filter recordings */
  if (filter.m_bIgnorePresentRecordings && g_PVRRecordings.size() > 0)
  {
    for (unsigned int iRecordingPtr = 0; iRecordingPtr < g_PVRRecordings.size(); iRecordingPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry  = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRRecordingInfoTag *recording = &g_PVRRecordings[iRecordingPtr];
        if (epgentry)
        {
          if (epgentry->Title()       != recording->Title() ||
              epgentry->PlotOutline() != recording->PlotOutline() ||
              epgentry->Plot()        != recording->Plot())
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  /* filter timers */
  if (filter.m_bIgnorePresentTimers && g_PVRTimers.size() > 0)
  {
    for (unsigned int iTimerPtr = 0; iTimerPtr < g_PVRTimers.size(); iTimerPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRTimerInfoTag *timer        = g_PVRTimers.at(iTimerPtr);
        if (epgentry)
        {
          if (epgentry->ChannelTag()->ChannelNumber() != timer->ChannelNumber() ||
              epgentry->Start()                       <  timer->Start() ||
              epgentry->End()                         >  timer->Stop())
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  return results->Size();
}

int CPVREpgContainer::GetEPGNow(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = (CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(bRadio);
  int iInitialSize           = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries() || epg->IsUpdateRunning())
      continue;

    const CPVREpgInfoTag *epgNow = (CPVREpgInfoTag *) epg->InfoTagNow();
    if (!epgNow)
    {
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNow));
    entry->SetLabel2(epgNow->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}

int CPVREpgContainer::GetEPGNext(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = (CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(bRadio);
  int iInitialSize           = results->Size();

  for (unsigned int iChannelPtr = 0; iChannelPtr < channels->size(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries() || epg->IsUpdateRunning())
      continue;

    const CPVREpgInfoTag *epgNext = (CPVREpgInfoTag *) epg->InfoTagNext();
    if (!epgNext)
    {
      continue;
    }

    CFileItemPtr entry(new CFileItem(*epgNext));
    entry->SetLabel2(epgNext->Start().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}
