#pragma once
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

#include "AddonCallbacks.h"
#include "include/xbmc_pvr_types.h"

namespace PVR
{
  class CPVRClient;
}

namespace ADDON
{

/*!
 * Callbacks for a PVR add-on to XBMC.
 *
 * Also translates the addon's C structures to XBMC's C++ structures.
 */
class CAddonCallbacksPVR
{
public:
  CAddonCallbacksPVR(CAddon* addon);
  ~CAddonCallbacksPVR(void);

  /*!
   * @return The callback table.
   */
  CB_PVRLib *GetCallbacks() { return m_callbacks; }

  /*!
   * @brief Transfer a channel group from the add-on to XBMC. The group will be created if it doesn't exist.
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the channel groups list
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferChannelGroup(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry);

  /*!
   * @brief Transfer a channel group member entry from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the channel group members list
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferChannelGroupMember(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry);

  /*!
   * @brief Transfer an EPG tag from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the EPG data
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferEpgEntry(void* addonData, const ADDON_HANDLE handle, const EPG_TAG* entry);

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the channel list
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferChannelEntry(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL* entry);

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the timers list
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferTimerEntry(void* addonData, const ADDON_HANDLE handle, const PVR_TIMER* entry);

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the recordings list
   * @param entry The entry to transfer to XBMC
   */
  static void PVRTransferRecordingEntry(void* addonData, const ADDON_HANDLE handle, const PVR_RECORDING* entry);

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param addonData A pointer to the add-on.
   * @param hook The hook to add.
   */
  static void PVRAddMenuHook(void* addonData, PVR_MENUHOOK* hook);

  /*!
   * @brief Display a notification in XBMC that a recording started or stopped on the server
   * @param addonData A pointer to the add-on.
   * @param strName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOnOff True when recording started, false when it stopped
   */
  static void PVRRecording(void* addonData, const char* strName, const char* strFileName, bool bOnOff);

  /*!
   * @brief Request XBMC to update it's list of channels
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerChannelUpdate(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of timers
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerTimerUpdate(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of recordings
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerRecordingUpdate(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of channel groups
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerChannelGroupsUpdate(void* addonData);

  /*!
   * @brief Schedule an EPG update for the given channel channel
   * @param addonData A pointer to the add-on
   * @param iChannelUid The unique id of the channel for this add-on
   */
  static void PVRTriggerEpgUpdate(void* addonData, unsigned int iChannelUid);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void PVRFreeDemuxPacket(void* addonData, DemuxPacket* pPacket);

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* PVRAllocateDemuxPacket(void* addonData, int iDataSize = 0);

private:
  static PVR::CPVRClient* GetPVRClient(void* addonData);

  CB_PVRLib    *m_callbacks; /*!< callback addresses */
  CAddon       *m_addon;     /*!< the addon */
};

}; /* namespace ADDON */
