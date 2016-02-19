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

#include "AddonExe_PVR_Transfer.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/PVR/AddonCallbacksPVR.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_PVR_Transfer::EpgEntry(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  EPG_TAG tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(EPG_TAG); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_epg_entry(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Transfer::ChannelEntry(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_CHANNEL tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(PVR_CHANNEL); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_channel_entry(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Transfer::TimerEntry(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_TIMER tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(PVR_TIMER); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_timer_entry(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Transfer::RecordingEntry(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_RECORDING tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(PVR_RECORDING); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_recording_entry(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Transfer::ChannelGroup(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_CHANNEL_GROUP tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(PVR_CHANNEL_GROUP); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_channel_group(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_Transfer::ChannelGroupMember(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_CHANNEL_GROUP_MEMBER tag;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  for (unsigned int i = 0; i < sizeof(PVR_CHANNEL_GROUP_MEMBER); ++i)
    req.pop(API_UINT8_T, &tag+i);
  PVR::CAddonCallbacksPVR::transfer_channel_group_member(addon, (ADDON_HANDLE)ptr, &tag);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
