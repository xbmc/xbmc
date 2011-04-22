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
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "pvr/epg/PVREpg.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"

using namespace PVR;

namespace ADDON
{

CAddonCallbacksPVR::CAddonCallbacksPVR(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PVRLib;

  /* write XBMC PVR specific add-on function addresses to callback table */
  m_callbacks->TransferEpgEntry           = PVRTransferEpgEntry;
  m_callbacks->TransferChannelEntry       = PVRTransferChannelEntry;
  m_callbacks->TransferTimerEntry         = PVRTransferTimerEntry;
  m_callbacks->TransferRecordingEntry     = PVRTransferRecordingEntry;
  m_callbacks->AddMenuHook                = PVRAddMenuHook;
  m_callbacks->Recording                  = PVRRecording;
  m_callbacks->TriggerChannelUpdate       = PVRTriggerChannelUpdate;
  m_callbacks->TriggerChannelGroupsUpdate = PVRTriggerChannelGroupsUpdate;
  m_callbacks->TriggerTimerUpdate         = PVRTriggerTimerUpdate;
  m_callbacks->TriggerRecordingUpdate     = PVRTriggerRecordingUpdate;
  m_callbacks->FreeDemuxPacket            = PVRFreeDemuxPacket;
  m_callbacks->AllocateDemuxPacket        = PVRAllocateDemuxPacket;
  m_callbacks->TransferChannelGroup       = PVRTransferChannelGroup;
  m_callbacks->TransferChannelGroupMember = PVRTransferChannelGroupMember;
}

CAddonCallbacksPVR::~CAddonCallbacksPVR()
{
  /* delete the callback table */
  delete m_callbacks;
}

void CAddonCallbacksPVR::PVRTransferChannelGroup(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL_GROUP *group)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || group == NULL || handle->dataAddress == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  if (strlen(group->strGroupName) == 0)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - empty group name", __FUNCTION__);
    return;
  }

  CPVRChannelGroups *xbmcGroups = (CPVRChannelGroups *) handle->dataAddress;
  CPVRChannelGroup xbmcGroup(*group);

  /* transfer this entry to the groups container */
  xbmcGroups->UpdateFromClient(xbmcGroup);
}

void CAddonCallbacksPVR::PVRTransferChannelGroupMember(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || member == NULL || handle->dataAddress == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client      = (CPVRClient*) handle->callerAddress;
  CPVRChannelGroup *group = (CPVRChannelGroup *) handle->dataAddress;
  CPVRChannel *channel    = (CPVRChannel *) g_PVRChannelGroups->GetByUniqueID(member->iChannelUniqueId, client->GetClientID());
  if (group != NULL && channel != NULL)
  {
    /* transfer this entry to the group */
    group->AddToGroup(channel, member->iChannelNumber, false);
  }
  else
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - cannot find group '%s' or channel '%d'",
        __FUNCTION__, member->strGroupName, member->iChannelUniqueId);
  }
}

void CAddonCallbacksPVR::PVRTransferEpgEntry(void *addonData, const PVR_HANDLE handle, const EPG_TAG *epgentry)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || epgentry == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVREpg *xbmcEpg   = (CPVREpg*) handle->dataAddress;

  EPG_TAG *epgentry2 = (EPG_TAG*) epgentry;
  bool bUpdateDatabase = handle->dataIdentifier == 1;

  /* transfer this entry to the epg */
  xbmcEpg->UpdateFromClient(epgentry2, bUpdateDatabase);
}

void CAddonCallbacksPVR::PVRTransferChannelEntry(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL *channel)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || channel == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client                     = (CPVRClient*) handle->callerAddress;
  CPVRChannelGroupInternal *xbmcChannels = (CPVRChannelGroupInternal*) handle->dataAddress;

  CPVRChannel channelTag(*channel, client->GetClientID());

  /* transfer this entry to the internal channels group */
  xbmcChannels->UpdateFromClient(channelTag);
}

void CAddonCallbacksPVR::PVRTransferRecordingEntry(void *addonData, const PVR_HANDLE handle, const PVR_RECORDING *recording)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || recording == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRClient* client             = (CPVRClient*) handle->callerAddress;
  CPVRRecordings *xbmcRecordings = (CPVRRecordings*) handle->dataAddress;

  CPVRRecording tag(*recording, client->GetClientID());

  /* transfer this entry to the recordings container */
  xbmcRecordings->UpdateFromClient(tag);
}

void CAddonCallbacksPVR::PVRTransferTimerEntry(void *addonData, const PVR_HANDLE handle, const PVR_TIMER *timer)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || handle == NULL || timer == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CPVRTimers *xbmcTimers = (CPVRTimers*) handle->dataAddress;
  CPVRClient* client     = (CPVRClient*) handle->callerAddress;
  CPVRChannel *channel   = (CPVRChannel *) g_PVRChannelGroups->GetByUniqueID(timer->iClientChannelUid, client->GetClientID());

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - cannot find channel %d on client %d",
        __FUNCTION__, timer->iClientChannelUid, client->GetClientID());
    return;
  }

  CPVRTimerInfoTag tag(*timer, channel, client->GetClientID());

  /* transfer this entry to the timers container */
  xbmcTimers->UpdateFromClient(tag);
}

void CAddonCallbacksPVR::PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || hook == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CAddonCallbacksPVR* addonHelper = addon->GetHelperPVR();
  CPVRClient* client  = (CPVRClient*) addonHelper->m_addon;
  PVR_MENUHOOKS *hooks = client->GetMenuHooks();

  PVR_MENUHOOK hookInt;
  hookInt.iHookId            = hook->iHookId;
  hookInt.iLocalizedStringId = hook->iLocalizedStringId;

  /* add this new hook */
  hooks->push_back(hookInt);
}

void CAddonCallbacksPVR::PVRRecording(void *addonData, const char *strName, const char *strFileName, bool bOnOff)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
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
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the channels table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerChannelsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerTimerUpdate(void *addonData)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the timers table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerTimersUpdate();
}

void CAddonCallbacksPVR::PVRTriggerRecordingUpdate(void *addonData)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update the recordings table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerRecordingsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerChannelGroupsUpdate(void *addonData)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksPVR - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  /* update all channel groups in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerChannelGroupsUpdate();
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
