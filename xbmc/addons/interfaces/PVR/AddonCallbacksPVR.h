#pragma once
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

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_epg_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_pvr.h"

namespace PVR
{
  class CPVRClient;
}

namespace ADDON
{
  class CAddon;
}

namespace KodiAPI
{
namespace PVR
{

struct EpgEventStateChange;

/*!
 * Callbacks for a PVR add-on to XBMC.
 *
 * Also translates the addon's C structures to XBMC's C++ structures.
 */
class CAddonCallbacksPVR
{
public:
  CAddonCallbacksPVR(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksPVR(void);

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
  static void cb_transfer_channel_group(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry);

  /*!
   * @brief Transfer a channel group member entry from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the channel group members list
   * @param entry The entry to transfer to XBMC
   */
  static void cb_transfer_channel_group_member(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry);

  /*!
   * @brief Transfer an EPG tag from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the EPG data
   * @param entry The entry to transfer to XBMC
   */
  static void cb_transfer_epg_entry(void* addonData, const ADDON_HANDLE handle, const EPG_TAG* entry);

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the channel list
   * @param entry The entry to transfer to XBMC
   */
  static void cb_transfer_channel_entry(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL* entry);

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the timers list
   * @param entry The entry to transfer to XBMC
   */
  static void cb_transfer_timer_entry(void* addonData, const ADDON_HANDLE handle, const PVR_TIMER* entry);

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC
   * @param addonData A pointer to the add-on.
   * @param handle The handle parameter that XBMC used when requesting the recordings list
   * @param entry The entry to transfer to XBMC
   */
  static void cb_transfer_recording_entry(void* addonData, const ADDON_HANDLE handle, const PVR_RECORDING* entry);

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param addonData A pointer to the add-on.
   * @param hook The hook to add.
   */
  static void cb_add_menu_hook(void* addonData, PVR_MENUHOOK* hook);

  /*!
   * @brief Display a notification in XBMC that a recording started or stopped on the server
   * @param addonData A pointer to the add-on.
   * @param strName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOnOff True when recording started, false when it stopped
   */
  static void cb_recording(void* addonData, const char* strName, const char* strFileName, bool bOnOff);

  /*!
   * @brief Request XBMC to update it's list of channels
   * @param addonData A pointer to the add-on.
   */
  static void cb_trigger_channel_update(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of timers
   * @param addonData A pointer to the add-on.
   */
  static void cb_trigger_timer_update(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of recordings
   * @param addonData A pointer to the add-on.
   */
  static void cb_trigger_recording_update(void* addonData);

  /*!
   * @brief Request XBMC to update it's list of channel groups
   * @param addonData A pointer to the add-on.
   */
  static void cb_trigger_channel_groups_update(void* addonData);

  /*!
   * @brief Schedule an EPG update for the given channel channel
   * @param addonData A pointer to the add-on
   * @param iChannelUid The unique id of the channel for this add-on
   */
  static void cb_trigger_epg_update(void* addonData, unsigned int iChannelUid);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void cb_free_demux_packet(void* addonData, DemuxPacket* pPacket);

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* cb_allocate_demux_packet(void* addonData, int iDataSize = 0);

  /*!
   * @brief Notify a state change for a PVR backend connection
   * @param addonData A pointer to the add-on.
   * @param strConnectionString The connection string reported by the backend that can be displayed in the UI.
   * @param newState The new state.
   * @param strMessage A localized addon-defined string representing the new state, that can be displayed
   *        in the UI or NULL if the Kodi-defined default string for the new state shall be displayed.
   */
  static void cb_connection_state_change(void* addonData, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage);

  /*!
   * @brief Notify a state change for an EPG event
   * @param addonData A pointer to the add-on.
   * @param tag The EPG event.
   * @param iUniqueChannelId The unique id of the channel for the EPG event
   * @param newState The new state.
   * @param newState The new state. For EPG_EVENT_CREATED and EPG_EVENT_UPDATED, tag must be filled with all available
   *        event data, not just a delta. For EPG_EVENT_DELETED, it is sufficient to fill EPG_TAG.iUniqueBroadcastId
   */
  static void cb_epg_event_state_change(void* addonData, EPG_TAG* tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState);

  /*! @todo remove the use complete from them, or add as generl function?!
   * Returns the ffmpeg codec id from given ffmpeg codec string name
   */
  static xbmc_codec_t cb_get_codec_by_name(const void* addonData, const char* strCodecName);

private:
  static ::PVR::CPVRClient* GetPVRClient(void* addonData);
  static void UpdateEpgEvent(const EpgEventStateChange &ch, bool bQueued);

  ADDON::CAddon* m_addon; /*!< the addon */
  CB_PVRLib    *m_callbacks; /*!< callback addresses */
};

} /* namespace PVR */
} /* namespace KodiAPI */
