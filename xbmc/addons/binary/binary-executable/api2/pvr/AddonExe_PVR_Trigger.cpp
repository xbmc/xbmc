/*
 *      Copyright (C) 2016 Team KODI
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

#include "AddonExe_PVR_Trigger.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/PVR/AddonCallbacksPVR.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_PVR_Trigger::TriggerTimerUpdate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR::CAddonCallbacksPVR::trigger_channel_update(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Trigger::TriggerRecordingUpdate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR::CAddonCallbacksPVR::trigger_channel_update(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Trigger::TriggerChannelUpdate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR::CAddonCallbacksPVR::trigger_channel_update(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Trigger::TriggerEpgUpdate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int iChannelUid;
  req.pop(API_UNSIGNED_INT, &iChannelUid);
  PVR::CAddonCallbacksPVR::trigger_epg_update(addon, iChannelUid);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Trigger::TriggerChannelGroupsUpdate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR::CAddonCallbacksPVR::trigger_channel_update(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
