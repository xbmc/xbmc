/*
 *      Copyright (C) 2012-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_PVR.h"

#include "Application.h"
#include "addons/PVRClient.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "dialogs/GUIDialogKaiToast.h"
#include "epg/EpgContainer.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace ADDON;
using namespace PVR;
using namespace EPG;

namespace V2
{
namespace KodiAPI
{

namespace PVR
{
extern "C"
{

void CAddonInterfacesPVR::Init(struct CB_AddOnLib *interfaces)
{
  /* write Kodi PVR specific add-on function addresses to callback table */
  interfaces->PVR.add_menu_hook                  = V2::KodiAPI::PVR::CAddonInterfacesPVR::add_menu_hook;
  interfaces->PVR.recording                      = V2::KodiAPI::PVR::CAddonInterfacesPVR::recording;
  interfaces->PVR.connection_state_change        = V2::KodiAPI::PVR::CAddonInterfacesPVR::connection_state_change;
  interfaces->PVR.epg_event_state_change         = V2::KodiAPI::PVR::CAddonInterfacesPVR::epg_event_state_change;

  interfaces->PVR.transfer_epg_entry             = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_epg_entry;
  interfaces->PVR.transfer_channel_entry         = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_entry;
  interfaces->PVR.transfer_channel_group         = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_group;
  interfaces->PVR.transfer_channel_group_member  = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_group_member;
  interfaces->PVR.transfer_timer_entry           = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_timer_entry;
  interfaces->PVR.transfer_recording_entry       = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_recording_entry;

  interfaces->PVR.trigger_channel_update         = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_channel_update;
  interfaces->PVR.trigger_channel_groups_update  = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_channel_groups_update;
  interfaces->PVR.trigger_timer_update           = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_timer_update;
  interfaces->PVR.trigger_recording_update       = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_recording_update;
  interfaces->PVR.trigger_epg_update             = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_epg_update;
}
/*\_____________________________________________________________________________
\*/
CPVRClient *CAddonInterfacesPVR::GetPVRClient(void* hdl)
{
  try
  {
    CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!addon || !addon->AddOnLib_GetHelper())
    {
      CLog::Log(LOGERROR, "PVR - %s - called with a null pointer", __FUNCTION__);
      return nullptr;
    }

    return dynamic_cast<CPVRClient *>(static_cast<CAddonInterfaceAddon*>(addon->AddOnLib_GetHelper())->GetAddon());
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}
/*\_____________________________________________________________________________
\*/
void CAddonInterfacesPVR::add_menu_hook(void* hdl, PVR_MENUHOOK* hook)
{
  try
  {
    CPVRClient *client = GetPVRClient(hdl);
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
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::recording(void* hdl, const char* strName, const char* strFileName, bool bOnOff)
{
  try
  {
    CPVRClient *client = GetPVRClient(hdl);
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
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void CAddonInterfacesPVR::transfer_epg_entry(void *userData, const ADDON_HANDLE_STRUCT* handle, const EPG_TAG *epgentry)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CEpg *kodiEpg = static_cast<CEpg *>(handle->dataAddress);
    if (!kodiEpg)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    /* transfer this entry to the epg */
    kodiEpg->UpdateEntry(epgentry, handle->dataIdentifier == 1 /* update db */);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::transfer_channel_entry(void* hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL *channel)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CPVRClient               *client       = GetPVRClient(hdl);
    CPVRChannelGroupInternal *kodiChannels = static_cast<CPVRChannelGroupInternal *>(handle->dataAddress);
    if (!channel || !client || !kodiChannels)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    /* transfer this entry to the internal channels group */
    CPVRChannelPtr transferChannel(new CPVRChannel(*channel, client->GetID()));
    kodiChannels->UpdateFromClient(transferChannel);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::transfer_channel_group(void* hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP* group)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CPVRChannelGroups *kodiGroups = static_cast<CPVRChannelGroups *>(handle->dataAddress);
    if (!group || !kodiGroups)
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
    kodiGroups->UpdateFromClient(transferGroup);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::transfer_channel_group_member(void* hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP_MEMBER* member)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CPVRClient *client      = GetPVRClient(hdl);
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
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::transfer_timer_entry(void* hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_TIMER *timer)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CPVRClient *client     = GetPVRClient(hdl);
    CPVRTimers *kodiTimers = static_cast<CPVRTimers *>(handle->dataAddress);
    if (!timer || !client || !kodiTimers)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    /* Note: channel can be NULL here, for instance for epg-based repeating timers ("record on any channel" condition). */
    CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(timer->iClientChannelUid, client->GetID());

    /* transfer this entry to the timers container */
    CPVRTimerInfoTagPtr transferTimer(new CPVRTimerInfoTag(*timer, channel, client->GetID()));
    kodiTimers->UpdateFromClient(transferTimer);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::transfer_recording_entry(void* hdl, const ADDON_HANDLE_STRUCT* handle, const PVR_RECORDING *recording)
{
  try
  {
    if (!handle)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    CPVRClient *client             = GetPVRClient(hdl);
    CPVRRecordings *kodiRecordings = static_cast<CPVRRecordings *>(handle->dataAddress);
    if (!recording || !client || !kodiRecordings)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    /* transfer this entry to the recordings container */
    CPVRRecordingPtr transferRecording(new CPVRRecording(*recording, client->GetID()));
    kodiRecordings->UpdateFromClient(transferRecording);
  }
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void CAddonInterfacesPVR::trigger_channel_update(void* hdl)
{
  try
  {
    /* update the channels table in the next iteration of the pvrmanager's main loop */
    g_PVRManager.TriggerChannelsUpdate();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::trigger_channel_groups_update(void* hdl)
{
  try
  {
    /* update all channel groups in the next iteration of the pvrmanager's main loop */
    g_PVRManager.TriggerChannelGroupsUpdate();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::trigger_timer_update(void* hdl)
{
  try
  {
    /* update the timers table in the next iteration of the pvrmanager's main loop */
    g_PVRManager.TriggerTimersUpdate();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::trigger_recording_update(void* hdl)
{
  try
  {
    /* update the recordings table in the next iteration of the pvrmanager's main loop */
    g_PVRManager.TriggerRecordingsUpdate();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::trigger_epg_update(void* hdl, unsigned int iChannelUid)
{
  try
  {
    // get the client
    CPVRClient *client = GetPVRClient(hdl);
    if (!client)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    g_EpgContainer.UpdateRequest(client->GetID(), iChannelUid);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddonInterfacesPVR::connection_state_change(void* hdl, const char* strConnectionString, int newState, const char *strMessage)
{
  try
  {
    CPVRClient *client = GetPVRClient(hdl);
    if (!client || !strConnectionString)
    {
      CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
      return;
    }

    const PVR_CONNECTION_STATE prevState(client->GetConnectionState());
    if (prevState == newState)
      return;

    CLog::Log(LOGDEBUG, "PVR - %s - state for connection '%s' on client '%s' changed from '%d' to '%d'", __FUNCTION__, strConnectionString, client->Name().c_str(), prevState, newState);

    client->SetConnectionState((PVR_CONNECTION_STATE)newState);

    std::string msg;
    if (strMessage != nullptr)
      msg = strMessage;

    g_PVRManager.ConnectionStateChange(client->GetID(), std::string(strConnectionString), (PVR_CONNECTION_STATE)newState, msg);
  }
  HANDLE_ADDON_EXCEPTION
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

void CAddonInterfacesPVR::UpdateEpgEvent(const EpgEventStateChange &ch, bool bQueued)
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

void CAddonInterfacesPVR::epg_event_state_change(void* hdl, EPG_TAG* tag, unsigned int iUniqueChannelId, int newState)
{
  CPVRClient *client = GetPVRClient(hdl);
  if (!client || !tag)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

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
    UpdateEpgEvent(EpgEventStateChange(client->GetID(), iUniqueChannelId, tag, (EPG_EVENT_STATE)newState), false);
  }
  else
  {
    // queue for later delivery.
    CSingleLock lock(queueMutex);
    queuedChanges.push_back(EpgEventStateChange(client->GetID(), iUniqueChannelId, tag, (EPG_EVENT_STATE)newState));
  }
}

} /* extern "C" */
} /* namespace PVR */

} /* namespace KodiAPI */
} /* namespace V2 */
