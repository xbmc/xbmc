/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
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

#include "AddonCallbacksPVR.h"

#include "Application.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "pvr/PVRManager.h"
#include "addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

using namespace ADDON;
using namespace PVR;

namespace KodiAPI
{
namespace PVR
{

class CCodecIds
{
public:
  virtual ~CCodecIds(void) {}

  static CCodecIds& GetInstance()
  {
    static CCodecIds _instance;
    return _instance;
  }

  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    xbmc_codec_t retVal = XBMC_INVALID_CODEC;
    if (strlen(strCodecName) == 0)
      return retVal;

    std::string strUpperCodecName = strCodecName;
    StringUtils::ToUpper(strUpperCodecName);

    std::map<std::string, xbmc_codec_t>::const_iterator it = m_lookup.find(strUpperCodecName);
    if (it != m_lookup.end())
      retVal = it->second;

    return retVal;
  }

private:
  CCodecIds(void)
  {
    // get ids and names
    AVCodec* codec = NULL;
    xbmc_codec_t tmp;
    while ((codec = av_codec_next(codec)))
    {
      if (av_codec_is_decoder(codec))
      {
        tmp.codec_type = (xbmc_codec_type_t)codec->type;
        tmp.codec_id   = codec->id;

        std::string strUpperCodecName = codec->name;
        StringUtils::ToUpper(strUpperCodecName);

        m_lookup.insert(std::make_pair(strUpperCodecName, tmp));
      }
    }

    // teletext is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = XBMC_CODEC_TYPE_SUBTITLE;
    tmp.codec_id   = AV_CODEC_ID_DVB_TELETEXT;
    m_lookup.insert(std::make_pair("TELETEXT", tmp));

    // rds is not returned by av_codec_next. we got our own decoder
    tmp.codec_type = XBMC_CODEC_TYPE_RDS;
    tmp.codec_id   = AV_CODEC_ID_NONE;
    m_lookup.insert(std::make_pair("RDS", tmp));
  }

  std::map<std::string, xbmc_codec_t> m_lookup;
};

CAddonCallbacksPVR::CAddonCallbacksPVR(CAddon* addon)
  : m_addon(addon),
    m_callbacks(new CB_PVRLib)
{
  /* write XBMC PVR specific add-on function addresses to callback table */
  m_callbacks->TransferEpgEntry = cb_transfer_epg_entry;
  m_callbacks->TransferChannelEntry = cb_transfer_channel_entry;
  m_callbacks->TransferTimerEntry = cb_transfer_timer_entry;
  m_callbacks->TransferRecordingEntry = cb_transfer_recording_entry;
  m_callbacks->AddMenuHook = cb_add_menu_hook;
  m_callbacks->Recording = cb_recording;
  m_callbacks->TriggerChannelUpdate = cb_trigger_channel_update;
  m_callbacks->TriggerChannelGroupsUpdate = cb_trigger_channel_groups_update;
  m_callbacks->TriggerTimerUpdate = cb_trigger_timer_update;
  m_callbacks->TriggerRecordingUpdate = cb_trigger_recording_update;
  m_callbacks->TriggerEpgUpdate = cb_trigger_epg_update;
  m_callbacks->FreeDemuxPacket = cb_free_demux_packet;
  m_callbacks->AllocateDemuxPacket = cb_allocate_demux_packet;
  m_callbacks->TransferChannelGroup = cb_transfer_channel_group;
  m_callbacks->TransferChannelGroupMember = cb_transfer_channel_group_member;
  m_callbacks->ConnectionStateChange = cb_connection_state_change;
  m_callbacks->EpgEventStateChange = cb_epg_event_state_change;
  m_callbacks->GetCodecByName = cb_get_codec_by_name;
}

CAddonCallbacksPVR::~CAddonCallbacksPVR()
{
  /* delete the callback table */
  delete m_callbacks;
}

CPVRClient *CAddonCallbacksPVR::GetPVRClient(void *addonData)
{
  CAddonInterfaces *addon = static_cast<CAddonInterfaces *>(addonData);
  if (!addon || !addon->PVRLib_GetHelper())
  {
    CLog::Log(LOGERROR, "PVR - %s - called with a null pointer", __FUNCTION__);
    return NULL;
  }

  return dynamic_cast<CPVRClient *>(static_cast<CAddonCallbacksPVR*>(addon->PVRLib_GetHelper())->m_addon);
}

void CAddonCallbacksPVR::cb_transfer_channel_group(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
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

void CAddonCallbacksPVR::cb_transfer_channel_group_member(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
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

  CPVRChannelPtr channel  = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(member->iChannelUniqueId, client->GetID());
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

void CAddonCallbacksPVR::cb_transfer_epg_entry(void *addonData, const ADDON_HANDLE handle, const EPG_TAG *epgentry)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVREpg *xbmcEpg = static_cast<CPVREpg *>(handle->dataAddress);
  if (!xbmcEpg)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* transfer this entry to the epg */
  xbmcEpg->UpdateEntry(epgentry, handle->dataIdentifier == 1 /* update db */);
}

void CAddonCallbacksPVR::cb_transfer_channel_entry(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL *channel)
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

void CAddonCallbacksPVR::cb_transfer_recording_entry(void *addonData, const ADDON_HANDLE handle, const PVR_RECORDING *recording)
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

void CAddonCallbacksPVR::cb_transfer_timer_entry(void *addonData, const ADDON_HANDLE handle, const PVR_TIMER *timer)
{
  if (!handle)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CPVRClient *client = GetPVRClient(addonData);
  CPVRTimersContainer *xbmcTimers = static_cast<CPVRTimersContainer *>(handle->dataAddress);
  if (!timer || !client || !xbmcTimers)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  /* Note: channel can be NULL here, for instance for epg-based timer rules ("record on any channel" condition). */
  CPVRChannelPtr channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(timer->iClientChannelUid, client->GetID());

  /* transfer this entry to the timers container */
  CPVRTimerInfoTagPtr transferTimer(new CPVRTimerInfoTag(*timer, channel, client->GetID()));
  xbmcTimers->UpdateFromClient(transferTimer);
}

void CAddonCallbacksPVR::cb_add_menu_hook(void *addonData, PVR_MENUHOOK *hook)
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

void CAddonCallbacksPVR::cb_recording(void *addonData, const char *strName, const char *strFileName, bool bOnOff)
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

void CAddonCallbacksPVR::cb_trigger_channel_update(void *addonData)
{
  /* update the channels table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerChannelsUpdate();
}

void CAddonCallbacksPVR::cb_trigger_timer_update(void *addonData)
{
  /* update the timers table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerTimersUpdate();
}

void CAddonCallbacksPVR::cb_trigger_recording_update(void *addonData)
{
  /* update the recordings table in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
}

void CAddonCallbacksPVR::cb_trigger_channel_groups_update(void *addonData)
{
  /* update all channel groups in the next iteration of the pvrmanager's main loop */
  CServiceBroker::GetPVRManager().TriggerChannelGroupsUpdate();
}

void CAddonCallbacksPVR::cb_trigger_epg_update(void *addonData, unsigned int iChannelUid)
{
  // get the client
  CPVRClient *client = GetPVRClient(addonData);
  if (!client)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  CServiceBroker::GetPVRManager().EpgContainer().UpdateRequest(client->GetID(), iChannelUid);
}

void CAddonCallbacksPVR::cb_free_demux_packet(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonCallbacksPVR::cb_allocate_demux_packet(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

void CAddonCallbacksPVR::cb_connection_state_change(void* addonData, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage)
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

  std::string msg;
  if (strMessage != nullptr)
    msg = strMessage;

  CServiceBroker::GetPVRManager().ConnectionStateChange(client, std::string(strConnectionString), newState, msg);
}

typedef struct EpgEventStateChange
{
  int             iClientId;
  unsigned int    iUniqueChannelId;
  CPVREpgInfoTagPtr  event;
  EPG_EVENT_STATE state;

  EpgEventStateChange(int _iClientId, unsigned int _iUniqueChannelId, EPG_TAG *_event, EPG_EVENT_STATE _state)
  : iClientId(_iClientId),
    iUniqueChannelId(_iUniqueChannelId),
    event(new CPVREpgInfoTag(*_event)),
    state(_state) {}

} EpgEventStateChange;

void CAddonCallbacksPVR::UpdateEpgEvent(const EpgEventStateChange &ch, bool bQueued)
{
  const CPVRChannelPtr channel(CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(ch.iUniqueChannelId, ch.iClientId));
  if (channel)
  {
    const CPVREpgPtr epg(channel->GetEPG());
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

void CAddonCallbacksPVR::cb_epg_event_state_change(void* addonData, EPG_TAG* tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState)
{
  CPVRClient *client = GetPVRClient(addonData);
  if (!client || !tag)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid handler data", __FUNCTION__);
    return;
  }

  static CCriticalSection queueMutex;
  static std::vector<EpgEventStateChange> queuedChanges;

  // during Kodi startup, addons may push updates very early, even before EPGs are ready to use.
  if (CServiceBroker::GetPVRManager().EpgsCreated())
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

xbmc_codec_t CAddonCallbacksPVR::cb_get_codec_by_name(const void* addonData, const char* strCodecName)
{
  (void)addonData;
  return CCodecIds::GetInstance().GetCodecByName(strCodecName);
}

} /* namespace PVR */
} /* namespace KodiAPI */
