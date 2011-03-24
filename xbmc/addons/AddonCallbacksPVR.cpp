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

#include "Application.h"
#include "AddonCallbacksPVR.h"
#include "utils/log.h"

#include "pvr/epg/PVREpg.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/addons/PVRClient.h"

namespace ADDON
{

CAddonCallbacksPVR::CAddonCallbacksPVR(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PVRLib;

  /* write XBMC PVR specific add-on function addresses to callback table */
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
}

CAddonCallbacksPVR::~CAddonCallbacksPVR()
{
  /* delete the callback table */
  delete m_callbacks;
}

void CAddonCallbacksPVR::PVRTransferEpgEntry(void *addonData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || epgentry == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client = (CPVRClient*) handle->CALLER_ADDRESS;
  CPVREpg *xbmcEpg   = (CPVREpg*) handle->DATA_ADDRESS;

  PVR_PROGINFO *epgentry2 = (PVR_PROGINFO*) epgentry;
  epgentry2->starttime   += client->GetTimeCorrection(); // XXX time correction
  epgentry2->endtime     += client->GetTimeCorrection(); // XXX time correction

  bool bUpdateDatabase = handle->DATA_IDENTIFIER == 1;

  /* transfer this entry to the epg */
  xbmcEpg->UpdateFromClient(epgentry2, bUpdateDatabase);
}

void CAddonCallbacksPVR::PVRTransferChannelEntry(void *addonData, const PVRHANDLE handle, const PVR_CHANNEL *channel)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || channel == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client                     = (CPVRClient*) handle->CALLER_ADDRESS;
  CPVRChannelGroupInternal *xbmcChannels = (CPVRChannelGroupInternal*) handle->DATA_ADDRESS;

  CPVRChannel channelTag(*channel, client->GetClientID());

  /* transfer this entry to the internal channels group */
  xbmcChannels->UpdateFromClient(channelTag);
}

void CAddonCallbacksPVR::PVRTransferRecordingEntry(void *addonData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || recording == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client             = (CPVRClient*) handle->CALLER_ADDRESS;
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

  /* transfer this entry to the recordings container */
  xbmcRecordings->UpdateFromClient(tag);
}

void CAddonCallbacksPVR::PVRTransferTimerEntry(void *addonData, const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || timer == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRTimers *xbmcTimers     = (CPVRTimers*) handle->DATA_ADDRESS;
  CPVRClient* client         = (CPVRClient*) handle->CALLER_ADDRESS;
  const CPVRChannel *channel = CPVRManager::GetChannelGroups()->GetByUniqueID(timer->channelUid, client->GetClientID());

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - cannot find channel %d on client %d",
        __FUNCTION__, timer->channelUid, client->GetClientID());
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

  /* transfer this entry to the timers container */
  xbmcTimers->UpdateFromClient(tag);
}

void CAddonCallbacksPVR::PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || hook == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CAddonCallbacksPVR* addonHelper = addon->GetHelperPVR();
  CPVRClient* client  = (CPVRClient*) addonHelper->m_addon;
  PVR_MENUHOOKS *hooks = client->GetMenuHooks();

  PVR_MENUHOOK hookInt;
  hookInt.hook_id   = hook->hook_id;
  hookInt.string_id = hook->string_id;

  /* add this new hook */
  hooks->push_back(hookInt);
}

void CAddonCallbacksPVR::PVRRecording(void *addonData, const char *strName, const char *strFileName, bool bOnOff)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CAddonCallbacksPVR* addonHelper = addon->GetHelperPVR();

  CStdString strLine1;
  if (bOnOff)
    strLine1.Format(g_localizeStrings.Get(19197), addonHelper->m_addon->Name());
  else
    strLine1.Format(g_localizeStrings.Get(19198), addonHelper->m_addon->Name());

  CStdString strLine2;
  if (strName)
    strLine2 = strName;
  else if (strFileName)
    strLine2 = strFileName;
  else
    strLine2 = "";

  /* display a notification for 5 seconds */
  g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000, false);

  CLog::Log(LOGDEBUG, "CAddonCallbacksPVR - %s - recording %s on client '%s'. name='%s' filename='%s'",
      __FUNCTION__, bOnOff ? "started" : "finished", addonHelper->m_addon->Name().c_str(), strName, strFileName);
}

void CAddonCallbacksPVR::PVRTriggerChannelUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the channels table in the next iteration of the pvrmanager's main loop */
  CPVRManager::Get()->TriggerChannelsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerTimerUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the timers table in the next iteration of the pvrmanager's main loop */
  CPVRManager::Get()->TriggerTimersUpdate();
}

void CAddonCallbacksPVR::PVRTriggerRecordingUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the recordings table in the next iteration of the pvrmanager's main loop */
  CPVRManager::Get()->TriggerRecordingsUpdate();
}

void CAddonCallbacksPVR::PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonCallbacksPVR::PVRAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

}; /* namespace ADDON */
