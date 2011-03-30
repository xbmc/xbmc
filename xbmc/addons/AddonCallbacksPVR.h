#pragma once
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

#include "AddonCallbacks.h"
#include "include/xbmc_pvr_types.h"

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
   * @param handle The handle that initiated this action.
   * @param group The entry to transfer.
   */
  static void PVRTransferChannelGroup(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL_GROUP *group);

  /*!
   * @brief Transfer a channel group member from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param addonData A pointer to the add-on.
   * @param handle The handle that initiated this action.
   * @param member The entry to transfer.
   */
  static void PVRTransferChannelGroupMember(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);

  /*!
   * @brief Transfer an EPG entry from the add-on to XBMC.
   * @param addonData A pointer to the add-on.
   * @param handle The handle that initiated this action.
   * @param epgentry The entry to transfer.
   */
  static void PVRTransferEpgEntry(void *addonData, const PVR_HANDLE handle, const EPG_TAG *epgentry);

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC.
   * @param addonData A pointer to the add-on.
   * @param handle The handle that initiated this action.
   * @param channel The entry to transfer.
   */
  static void PVRTransferChannelEntry(void *addonData, const PVR_HANDLE handle, const PVR_CHANNEL *channel);

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC.
   * @param addonData A pointer to the add-on.
   * @param handle The handle that initiated this action.
   * @param timer The entry to transfer.
   */
  static void PVRTransferTimerEntry(void *addonData, const PVR_HANDLE handle, const PVR_TIMER *timer);

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC.
   * @param addonData A pointer to the add-on.
   * @param handle The handle that initiated this action.
   * @param recording The entry to transfer.
   */
  static void PVRTransferRecordingEntry(void *addonData, const PVR_HANDLE handle, const PVR_RECORDING *recording);

  /*!
   * @brief Add a menu hook to this add-on table.
   * @param addonData A pointer to the add-on.
   * @param hook The hook to add.
   */
  static void PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook);

  /*!
   * @brief Notify XBMC that a recording has started or stoppped.
   * @param addonData A pointer to the add-on.
   * @param strName The name of the recording.
   * @param strFileName The filename of the recording.
   * @param bOnOff True if the recording started, false if it stopped.
   */
  static void PVRRecording(void *addonData, const char *strName, const char *strFileName, bool bOnOff);

  /*!
   * @brief Ask the PVRManager to refresh it's channels list.
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerChannelUpdate(void *addonData);

  /*!
   * @brief Ask the PVRManager to refresh it's timers list.
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerTimerUpdate(void *addonData);

  /*!
   * @brief Ask the PVRManager to refresh it's recordings list.
   * @param addonData A pointer to the add-on.
   */
  static void PVRTriggerRecordingUpdate(void *addonData);

  /*!
   * @brief Free an allocated demux packet.
   * @param addonData A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket);

  /*!
   * @brief Allocate a new demux packet.
   * @param addonData A pointer to the add-on.
   * @param iDataSize The packet size.
   * @return The allocated packet.
   */
  static DemuxPacket* PVRAllocateDemuxPacket(void *addonData, int iDataSize = 0);

private:
  CB_PVRLib    *m_callbacks; /*!< callback addresses */
  CAddon       *m_addon;     /*!< the addon */
};

}; /* namespace ADDON */
