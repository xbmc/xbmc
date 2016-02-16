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

#include "Application.h"
#include "AddonCallbacksPVR.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/Settings.h"

#include "epg/EpgContainer.h"
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
  m_callbacks->ConnectionStateChange      = PVRConnectionStateChange;
  m_callbacks->EpgEventStateChange        = PVREpgEventStateChange;
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
    group->AddToGroup(channel, member->iChannelNumber);
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
  CPVRChannelPtr transferChannel(new CPVRChannel(*channel, client->GetID()));
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
  CPVRRecordingPtr transferRecording(new CPVRRecording(*recording, client->GetID()));
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

  /* Note: channel can be NULL here, for instance for epg-based repeating timers ("record on any channel" condition). */
  CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(timer->iClientChannelUid, client->GetID());

  /* transfer this entry to the timers container */
  CPVRTimerInfoTagPtr transferTimer(new CPVRTimerInfoTag(*timer, channel, client->GetID()));
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
    hookInt.category           = hook->category;

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

  std::string strLine1 = StringUtils::Format(g_localizeStrings.Get(bOnOff ? 19197 : 19198).c_str(), client->Name().c_str());
  std::string strLine2;
  if (strName)
    strLine2 = strName;
  else if (strFileName)
    strLine2 = strFileName;

  /* display a notification for 5 seconds */
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000, false);
  CEventLog::GetInstance().Add(EventPtr(new CNotificationEvent(client->Name(), strLine1, client->Icon(), strLine2)));

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

  g_EpgContainer.UpdateRequest(client->GetID(), iChannelUid);
}

void CAddonCallbacksPVR::PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonCallbacksPVR::PVRAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

void CAddonCallbacksPVR::PVRConnectionStateChange(void* addonData, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage)
{
  CPVRClient *client = GetPVRClient(addonData);
  if (!client || !strConnectionString)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  const PVR_CONNECTION_STATE prevState(client->GetConnectionState());
  if (prevState == newState)
    return;

  CLog::Log(LOGDEBUG, "PVR - %s - state for connection '%s' on client '%s' changed from '%d' to '%d'", __FUNCTION__, strConnectionString, client->Name().c_str(), prevState, newState);

  client->SetConnectionState(newState);

  int iMsg(-1);
  bool bError(true);
  bool bNotify(true);

  switch (newState)
  {
    case PVR_CONNECTION_STATE_SERVER_UNREACHABLE:
      iMsg = 35505; // Server is unreachable
      break;
    case PVR_CONNECTION_STATE_SERVER_MISMATCH:
      iMsg = 35506; // Server does not respond properly
      break;
    case PVR_CONNECTION_STATE_VERSION_MISMATCH:
      iMsg = 35507; // Server version is not compatible
      break;
    case PVR_CONNECTION_STATE_ACCESS_DENIED:
      iMsg = 35508; // Access denied
      break;
    case PVR_CONNECTION_STATE_CONNECTED:
      iMsg = 36034; // Connection established
      bError = false;
      // No notification for the first successful connect.
      bNotify = (prevState != PVR_CONNECTION_STATE_UNKNOWN);
      break;
    case PVR_CONNECTION_STATE_DISCONNECTED:
      iMsg = 36030; // Connection lost
      break;
    default:
      CLog::Log(LOGERROR, "PVR - %s - unknown connection state", __FUNCTION__);
      return;
  }

  // Use addon-supplied message, if present
  std::string strMsg;
  if (strMessage && strlen(strMessage) > 0)
    strMsg = strMessage;
  else
    strMsg = g_localizeStrings.Get(iMsg);

  // Notify user.
  if (bNotify && !CSettings::GetInstance().GetBool(CSettings::SETTING_PVRMANAGER_HIDECONNECTIONLOSTWARNING))
    CGUIDialogKaiToast::QueueNotification(
      bError ? CGUIDialogKaiToast::Error : CGUIDialogKaiToast::Info, client->Name().c_str(), strMsg, 5000, true);

  // Write event log entry.
  CEventLog::GetInstance().Add(EventPtr(new CNotificationEvent(
    client->Name(), strMsg, client->Icon(), bError ? EventLevel::Error : EventLevel::Information)));
}

typedef struct EpgEventStateChange
{
  int             iClientId;
  unsigned int    iUniqueChannelId;
  CEpgInfoTagPtr  event;
  EPG_EVENT_STATE state;

  EpgEventStateChange(int _iClientId, unsigned int _iUniqueChannelId, EPG_TAG *_event, EPG_EVENT_STATE _state)
  : iClientId(_iClientId),
    iUniqueChannelId(_iUniqueChannelId),
    event(new CEpgInfoTag(*_event)),
    state(_state) {}

} EpgEventStateChange;

void CAddonCallbacksPVR::UpdateEpgEvent(const EpgEventStateChange &ch, bool bQueued)
{
  const CPVRChannelPtr channel(g_PVRChannelGroups->GetByUniqueID(ch.iUniqueChannelId, ch.iClientId));
  if (channel)
  {
    const CEpgPtr epg(channel->GetEPG());
    if (epg)
    {
      if (!epg->UpdateEntry(ch.event, ch.state))
        CLog::Log(LOGERROR, "PVR - %s - epg update failed for %sevent change (%d)",
                  __FUNCTION__, bQueued ? "queued " : "", ch.event->UniqueBroadcastID());
    }
    else
    {
      CLog::Log(LOGERROR, "PVR - %s - channel '%s' does not have an EPG! Unable to deliver %sevent change (%d)!",
                __FUNCTION__, channel->ChannelName().c_str(), bQueued ? "queued " : "", ch.event->UniqueBroadcastID());
    }
  }
  else
    CLog::Log(LOGERROR, "PVR - %s - invalid channel (%d)! Unable to deliver %sevent change (%d)!",
              __FUNCTION__, ch.iUniqueChannelId, bQueued ? "queued " : "", ch.event->UniqueBroadcastID());
}

void CAddonCallbacksPVR::PVREpgEventStateChange(void* addonData, EPG_TAG* tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState)
{
  CPVRClient *client = GetPVRClient(addonData);
  if (!client || !tag)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CLog::Log(LOGDEBUG, "PVR - %s - state for epg event '%d' on channel '%d' on client '%s' changed to '%d'.",
            __FUNCTION__, tag->iUniqueBroadcastId, iUniqueChannelId, client->Name().c_str(), newState);

  static CCriticalSection queueMutex;
  static std::vector<EpgEventStateChange> queuedChanges;

  // during Kodi startup, addons may push updates very early, even before EPGs are ready to use.
  if (g_PVRManager.EpgsCreated())
  {
    {
      // deliver queued changes, if any. discard event if delivery fails.
      CSingleLock lock(queueMutex);
      if (!queuedChanges.empty())
        CLog::Log(LOGNOTICE, "PVR - %s - processing %ld queued epg event changes.", __FUNCTION__, queuedChanges.size());

      while (!queuedChanges.empty())
      {
        auto it = queuedChanges.begin();
        UpdateEpgEvent(*it, true);
        queuedChanges.erase(it);
      }
    }

    // deliver current change.
    UpdateEpgEvent(EpgEventStateChange(client->GetID(), iUniqueChannelId, tag, newState), false);
  }
  else
  {
    // queue for later delivery.
    CSingleLock lock(queueMutex);
    queuedChanges.push_back(EpgEventStateChange(client->GetID(), iUniqueChannelId, tag, newState));
  }
}

}; /* namespace ADDON */
