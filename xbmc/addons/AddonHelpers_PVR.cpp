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

#include "Application.h"
#include "AddonHelpers_PVR.h"
#include "utils/log.h"

#include "pvr/epg/PVREpg.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/addons/PVRClient.h"

namespace ADDON
{

CAddonHelpers_PVR::CAddonHelpers_PVR(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PVRLib;

  /* Write XBMC PVR specific Add-on function addresses to callback table */
  m_callbacks->TransferEpgEntry       = PVRTransferEpgEntry;
  m_callbacks->TransferChannelEntry   = PVRTransferChannelEntry;
  m_callbacks->TransferTimerEntry     = PVRTransferTimerEntry;
  m_callbacks->TransferRecordingEntry = PVRTransferRecordingEntry;
  m_callbacks->AddMenuHook            = PVRAddMenuHook;
  m_callbacks->Recording              = PVRRecording;
  m_callbacks->TriggerChannelUpdate   = PVRTriggerChannelUpdate;
  m_callbacks->TriggerTimerUpdate     = PVRTriggerTimerUpdate;
  m_callbacks->TriggerRecordingUpdate = PVRTriggerRecordingUpdate;
  m_callbacks->FreeDemuxPacket        = PVRFreeDemuxPacket;
  m_callbacks->AllocateDemuxPacket    = PVRAllocateDemuxPacket;
};

CAddonHelpers_PVR::~CAddonHelpers_PVR()
{
  delete m_callbacks;
};

void CAddonHelpers_PVR::PVRTransferEpgEntry(void *addonData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || epgentry == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferEpgEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVREpg *xbmcEpg        = (CPVREpg*) handle->DATA_ADDRESS;
  PVR_PROGINFO *epgentry2 = (PVR_PROGINFO*) epgentry;
  CPVRClient* client      = (CPVRClient*) handle->CALLER_ADDRESS;
  epgentry2->starttime   += client->GetTimeCorrection();
  epgentry2->endtime     += client->GetTimeCorrection();
  xbmcEpg->UpdateEntry(epgentry2, handle->DATA_IDENTIFIER == 1);

  return;
}

void CAddonHelpers_PVR::PVRTransferChannelEntry(void *addonData, const PVRHANDLE handle, const PVR_CHANNEL *channel)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferChannelEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVRClient* client = (CPVRClient*) handle->CALLER_ADDRESS;
  CPVRChannelGroupInternal *xbmcChannels = (CPVRChannelGroupInternal*) handle->DATA_ADDRESS;

  CPVRChannel channelTag(*channel, client->GetClientID());
  xbmcChannels->UpdateFromClient(channelTag);
}

void CAddonHelpers_PVR::PVRTransferRecordingEntry(void *addonData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || recording == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferRecordingEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVRClient* client = (CPVRClient*) handle->CALLER_ADDRESS;
  CPVRRecordings *xbmcRecordings = (CPVRRecordings*) handle->DATA_ADDRESS;

  CPVRRecording tag;

  tag.m_clientIndex    = recording->index;
  tag.m_clientID       = client->GetClientID();
  tag.m_strTitle       = recording->title;
  tag.m_recordingTime  = recording->recording_time;
  tag.m_duration       = CDateTimeSpan(0, 0, recording->duration / 60, recording->duration % 60);
  tag.m_Priority       = recording->priority;
  tag.m_Lifetime       = recording->lifetime;
  tag.m_strDirectory   = recording->directory;
  tag.m_strPlot        = recording->description;
  tag.m_strPlotOutline = recording->subtitle;
  tag.m_strStreamURL   = recording->stream_url;
  tag.m_strChannel     = recording->channel_name;

  xbmcRecordings->UpdateEntry(tag);
}

void CAddonHelpers_PVR::PVRTransferTimerEntry(void *addonData, const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || timer == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVRTimers *xbmcTimers     = (CPVRTimers*) handle->DATA_ADDRESS;
  CPVRClient* client         = (CPVRClient*) handle->CALLER_ADDRESS;
  const CPVRChannel *channel = CPVRManager::GetChannelGroups()->GetByUniqueID(timer->channelUid, client->GetClientID());

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with not present channel");
    return;
  }

  CPVRTimerInfoTag tag;
  tag.m_iClientID         = client->GetClientID();
  tag.m_iClientIndex      = timer->index;
  tag.m_bIsActive         = timer->active == 1;
  tag.m_strTitle          = timer->title;
  tag.m_strDir            = timer->directory;
  tag.m_iClientNumber     = timer->channelNum;
  tag.m_iClientChannelUid = timer->channelUid;
  tag.m_StartTime         = (time_t) (timer->starttime+client->GetTimeCorrection());
  tag.m_StopTime          = (time_t) (timer->endtime+client->GetTimeCorrection());
  tag.m_FirstDay          = (time_t) (timer->firstday+client->GetTimeCorrection());
  tag.m_iPriority         = timer->priority;
  tag.m_iLifetime         = timer->lifetime;
  tag.m_bIsRecording      = timer->recording == 1;
  tag.m_bIsRepeating      = timer->repeat == 1;
  tag.m_iWeekdays         = timer->repeatflags;
  tag.m_strFileNameAndPath.Format("pvr://client%i/timers/%i", tag.m_iClientID, tag.m_iClientIndex);
  tag.UpdateSummary();

  xbmcTimers->Update(tag);
}

void CAddonHelpers_PVR::PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || hook == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRAddMenuHook is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();
  CPVRClient* client  = (CPVRClient*) addonHelper->m_addon;
  PVR_MENUHOOKS *hooks = client->GetMenuHooks();

  PVR_MENUHOOK hookInt;
  hookInt.hook_id   = hook->hook_id;
  hookInt.string_id = hook->string_id;
  hooks->push_back(hookInt);
}

void CAddonHelpers_PVR::PVRRecording(void *addonData, const char *Name, const char *FileName, bool On)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRRecording is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();

  CStdString line1;
  CStdString line2;
  if (On)
    line1.Format(g_localizeStrings.Get(19197), addonHelper->m_addon->Name());
  else
    line1.Format(g_localizeStrings.Get(19198), addonHelper->m_addon->Name());

  if (Name)
    line2 = Name;
  else if (FileName)
    line2 = FileName;
  else
    line2 = "";

  g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info, line1, line2, 5000, false);
  CLog::Log(LOGDEBUG, "%s: %s-%s - Recording %s : %s %s", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), On ? "started" : "finished", Name, FileName);
}

void CAddonHelpers_PVR::PVRTriggerChannelUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR addon - %s - called from an invalid client", __FUNCTION__);
    return;
  }

  CPVRManager::Get()->TriggerChannelsUpdate();
}

void CAddonHelpers_PVR::PVRTriggerTimerUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTriggerTimerUpdate is called with NULL-Pointer!!!");
    return;
  }

  CPVRManager::Get()->TriggerTimersUpdate();
}

void CAddonHelpers_PVR::PVRTriggerRecordingUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTriggerRecordingUpdate is called with NULL-Pointer!!!");
    return;
  }

  CPVRManager::Get()->TriggerRecordingsUpdate();
}

void CAddonHelpers_PVR::PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonHelpers_PVR::PVRAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

}; /* namespace ADDON */
