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

#include "threads/SingleLock.h"
#include "PVREpgContainer.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace std;
using namespace PVR;
using namespace EPG;

void PVR::CPVREpgContainer::Clear(bool bClearDb /* = false */)
{
  CSingleLock lock(m_critSection);
  // XXX stop the timers from being updated while clearing tags
  /* remove all pointers to epg tables on timers */
  CPVRTimers *timers = g_PVRTimers;
  for (unsigned int iTimerPtr = 0; iTimerPtr < timers->size(); iTimerPtr++)
    timers->at(iTimerPtr)->SetEpgInfoTag(NULL);

  CEpgContainer::Clear(bClearDb);
}

bool PVR::CPVREpgContainer::CreateChannelEpgs(void)
{
  for (int radio = 0; radio <= 1; radio++)
  {
    const CPVRChannelGroup *channels = g_PVRChannelGroups->GetGroupAll(radio == 1);
    for (int iChannelPtr = 0; iChannelPtr < channels->GetNumChannels(); iChannelPtr++)
    {
      CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
      CEpg *epg = GetById(channel->ChannelID());
      if (!epg)
        channel->GetEPG();
      else
      {
        channel->m_EPG = (CPVREpg *) epg;
        epg->SetChannel(channel);
      }
    }
  }

  return true;
}

int PVR::CPVREpgContainer::GetEPGAll(CFileItemList* results, bool bRadio /* = false */)
{
  int iInitialSize = results->Size();
  const CPVRChannelGroup *group = g_PVRChannelGroups->GetGroupAll(bRadio);
  if (!group)
    return -1;

  CSingleLock lock(m_critSection);
  for (int iChannelPtr = 0; iChannelPtr < group->GetNumChannels(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) group->GetByIndex(iChannelPtr);
    if (!channel || channel->IsHidden())
      continue;

    channel->GetEPG(results);
  }

  return results->Size() - iInitialSize;
}

bool PVR::CPVREpgContainer::AutoCreateTablesHook(void)
{
  return CreateChannelEpgs();
}

CEpg* PVR::CPVREpgContainer::CreateEpg(int iEpgId)
{
  CPVRChannel *channel = (CPVRChannel *) g_PVRChannelGroups->GetChannelById(iEpgId);
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

const CDateTime PVR::CPVREpgContainer::GetFirstEPGDate(bool bRadio /* = false */)
{
  // TODO should use two separate containers, one for radio, one for tv
  return CEpgContainer::GetFirstEPGDate();
}

const CDateTime PVR::CPVREpgContainer::GetLastEPGDate(bool bRadio /* = false */)
{
  // TODO should use two separate containers, one for radio, one for tv
  return CEpgContainer::GetLastEPGDate();
}

int PVR::CPVREpgContainer::GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter)
{
  CEpgContainer::GetEPGSearch(results, filter);

  /* filter recordings */
  if (filter.m_bIgnorePresentRecordings && g_PVRRecordings->size() > 0)
  {
    for (unsigned int iRecordingPtr = 0; iRecordingPtr < g_PVRRecordings->size(); iRecordingPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry  = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRRecording *recording = g_PVRRecordings->at(iRecordingPtr);
        if (epgentry)
        {
          if (epgentry->Title()       != recording->m_strTitle ||
              epgentry->PlotOutline() != recording->m_strPlotOutline ||
              epgentry->Plot()        != recording->m_strPlot)
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  /* filter timers */
  if (filter.m_bIgnorePresentTimers)
  {
    CPVRTimers *timers = g_PVRTimers;
    for (unsigned int iTimerPtr = 0; iTimerPtr < timers->size(); iTimerPtr++)
    {
      for (int iResultPtr = 0; iResultPtr < results->Size(); iResultPtr++)
      {
        const CPVREpgInfoTag *epgentry = (CPVREpgInfoTag *) results->Get(iResultPtr)->GetEPGInfoTag();
        CPVRTimerInfoTag *timer        = timers->at(iTimerPtr);
        if (epgentry)
        {
          if (epgentry->ChannelTag()->ChannelNumber() != timer->ChannelNumber() ||
              epgentry->StartAsUTC()                   <  timer->StartAsUTC() ||
              epgentry->EndAsUTC()                     >  timer->EndAsUTC())
            continue;

          results->Remove(iResultPtr);
          iResultPtr--;
        }
      }
    }
  }

  return results->Size();
}

int PVR::CPVREpgContainer::GetEPGNow(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = g_PVRChannelGroups->GetGroupAll(bRadio);
  CSingleLock lock(m_critSection);
  int iInitialSize           = results->Size();

  for (int iChannelPtr = 0; iChannelPtr < channels->GetNumChannels(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries())
      continue;

    const CPVREpgInfoTag *epgNow = (CPVREpgInfoTag *) epg->InfoTagNow();
    if (!epgNow)
      continue;

    CFileItemPtr entry(new CFileItem(*epgNow));
    entry->SetLabel2(epgNow->StartAsLocalTime().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}

int PVR::CPVREpgContainer::GetEPGNext(CFileItemList* results, bool bRadio)
{
  CPVRChannelGroup *channels = g_PVRChannelGroups->GetGroupAll(bRadio);
  CSingleLock lock(m_critSection);
  int iInitialSize           = results->Size();

  for (int iChannelPtr = 0; iChannelPtr < channels->GetNumChannels(); iChannelPtr++)
  {
    CPVRChannel *channel = (CPVRChannel *) channels->GetByIndex(iChannelPtr);
    CPVREpg *epg = channel->GetEPG();
    if (!epg->HasValidEntries())
      continue;

    const CPVREpgInfoTag *epgNext = (CPVREpgInfoTag *) epg->InfoTagNext();
    if (!epgNext)
      continue;

    CFileItemPtr entry(new CFileItem(*epgNext));
    entry->SetLabel2(epgNext->StartAsLocalTime().GetAsLocalizedTime("", false));
    entry->m_strPath = channel->ChannelName();
    entry->SetThumbnailImage(channel->IconPath());
    results->Add(entry);
  }

  return results->Size() - iInitialSize;
}

bool PVR::CPVREpgContainer::UpdateEPG(bool bShowProgress /* = false */)
{
  bool bReturn = CEpgContainer::UpdateEPG(bShowProgress);

  if (bReturn)
  {
    CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
    if (pWindow)
      pWindow->InitializeEpgCache();
  }

  return bReturn;
}

bool PVR::CPVREpgContainer::InterruptUpdate(void) const
{
  return (CEpgContainer::InterruptUpdate() ||
      (g_PVRManager.IsStarted() && g_PVRManager.IsPlaying()));
}
