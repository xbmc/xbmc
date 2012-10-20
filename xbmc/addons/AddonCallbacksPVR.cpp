/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Application.h"
#include "AddonCallbacksPVR.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "dialogs/GUIDialogKaiToast.h"

#include "epg/Epg.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"

using namespace PVR;
using namespace EPG;

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
  m_callbacks->TriggerEpgUpdate           = PVRTriggerEpgUpdate;
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

CPVRClient *CAddonCallbacksPVR::GetPVRClient(void *addonData)
{
  CAddonCallbacks *addon = static_cast<CAddonCallbacks *>(addonData);
  if (!addon || !addon->GetHelperPVR())
  {
    CLog::Log(LOGERROR, "PVR - %s - called with a null pointer", __FUNCTION__);
    return NULL;
  }

  return dynamic_cast<CPVRClient *>(addon->GetHelperPVR()->m_addon);
}

void CAddonCallbacksPVR::PVRTransferChannelGroup(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRChannelGroups *xbmcGroups = static_cast<CPVRChannelGroups *>(handle->dataAddress);
  if (!group || !xbmcGroups)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  if (strlen(group->strGroupName) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty group name", __FUNCTION__);
    return;
  }

  /* transfer this entry to the groups container */
  CPVRChannelGroup transferGroup(*group);
  xbmcGroups->UpdateFromClient(transferGroup);
}

void CAddonCallbacksPVR::PVRTransferChannelGroupMember(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }
  
  CPVRClient *client      = GetPVRClient(addonData);
  CPVRChannelGroup *group = static_cast<CPVRChannelGroup *>(handle->dataAddress);
  if (!member || !client || !group)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRChannelPtr channel  = g_PVRChannelGroups->GetByUniqueID(member->iChannelUniqueId, client->GetID());
  if (!channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find group '%s' or channel '%d'", __FUNCTION__, member->strGroupName, member->iChannelUniqueId);
  }
  else if (group->IsRadio() == channel->IsRadio())
  {
    /* transfer this entry to the group */
    group->AddToGroup(*channel, member->iChannelNumber, false);
  }
}

void CAddonCallbacksPVR::PVRTransferEpgEntry(void *addonData, const ADDON_HANDLE handle, const EPG_TAG *epgentry)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CEpg *xbmcEpg = static_cast<CEpg *>(handle->dataAddress);
  if (!xbmcEpg)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the epg */
  xbmcEpg->UpdateEntry(epgentry, handle->dataIdentifier == 1 /* update db */);
}

void CAddonCallbacksPVR::PVRTransferChannelEntry(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL *channel)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client                     = GetPVRClient(addonData);
  CPVRChannelGroupInternal *xbmcChannels = static_cast<CPVRChannelGroupInternal *>(handle->dataAddress);
  if (!channel || !client || !xbmcChannels)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the internal channels group */
  CPVRChannel transferChannel(*channel, client->GetID());
  xbmcChannels->UpdateFromClient(transferChannel);
}

void CAddonCallbacksPVR::PVRTransferRecordingEntry(void *addonData, const ADDON_HANDLE handle, const PVR_RECORDING *recording)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client             = GetPVRClient(addonData);
  CPVRRecordings *xbmcRecordings = static_cast<CPVRRecordings *>(handle->dataAddress);
  if (!recording || !client || !xbmcRecordings)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the recordings container */
  CPVRRecording transferRecording(*recording, client->GetID());
  xbmcRecordings->UpdateFromClient(transferRecording);
}

void CAddonCallbacksPVR::PVRTransferTimerEntry(void *addonData, const ADDON_HANDLE handle, const PVR_TIMER *timer)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client     = GetPVRClient(addonData);
  CPVRTimers *xbmcTimers = static_cast<CPVRTimers *>(handle->dataAddress);
  if (!timer || !client || !xbmcTimers)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(timer->iClientChannelUid, client->GetID());
  if (!channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot find channel %d on client %d", __FUNCTION__, timer->iClientChannelUid, client->GetID());
    return;
  }

  /* transfer this entry to the timers container */
  CPVRTimerInfoTag transferTimer(*timer, channel, client->GetID());
  xbmcTimers->UpdateFromClient(transferTimer);
}

void CAddonCallbacksPVR::PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook)
{
  CPVRClient *client = GetPVRClient(addonData);
  if (!hook || !client)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  PVR_MENUHOOKS *hooks = client->GetMenuHooks();
  if (hooks)
  {
    PVR_MENUHOOK hookInt;
    hookInt.iHookId            = hook->iHookId;
    hookInt.iLocalizedStringId = hook->iLocalizedStringId;

    /* add this new hook */
    hooks->push_back(hookInt);
  }
}

void CAddonCallbacksPVR::PVRRecording(void *addonData, const char *strName, const char *strFileName, bool bOnOff)
{
  CPVRClient *client = GetPVRClient(addonData);
  if (!client || !strFileName)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CStdString strLine1;
  if (bOnOff)
    strLine1.Format(g_localizeStrings.Get(19197), client->Name());
  else
    strLine1.Format(g_localizeStrings.Get(19198), client->Name());

  CStdString strLine2;
  if (strName)
    strLine2 = strName;
  else if (strFileName)
    strLine2 = strFileName;

  /* display a notification for 5 seconds */
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000, false);

  CLog::Log(LOGDEBUG, "PVR - %s - recording %s on client '%s'. name='%s' filename='%s'",
      __FUNCTION__, bOnOff ? "started" : "finished", client->Name().c_str(), strName, strFileName);
}

void CAddonCallbacksPVR::PVRTriggerChannelUpdate(void *addonData)
{
  /* update the channels table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerChannelsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerTimerUpdate(void *addonData)
{
  /* update the timers table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerTimersUpdate();
}

void CAddonCallbacksPVR::PVRTriggerRecordingUpdate(void *addonData)
{
  /* update the recordings table in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerRecordingsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerChannelGroupsUpdate(void *addonData)
{
  /* update all channel groups in the next iteration of the pvrmanager's main loop */
  g_PVRManager.TriggerChannelGroupsUpdate();
}

void CAddonCallbacksPVR::PVRTriggerEpgUpdate(void *addonData, unsigned int iChannelUid)
{
  // get the client
  CPVRClient *client = GetPVRClient(addonData);
  if (!client)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  // get the channel
  CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(iChannelUid, client->GetID());
  CEpg* epg(NULL);
  // get the EPG for the channel
  if (!channel || (epg = channel->GetEPG()) == NULL)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel or channel doesn't have an EPG", __FUNCTION__);
    return;
  }

  // force an update
  epg->ForceUpdate();
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
