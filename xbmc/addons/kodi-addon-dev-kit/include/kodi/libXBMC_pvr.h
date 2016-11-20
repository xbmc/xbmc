#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"

#define DVD_TIME_BASE 1000000

//! @todo original definition is in DVDClock.h
#define DVD_NOPTS_VALUE 0xFFF0000000000000

namespace KodiAPI
{
namespace V1
{
namespace PVR
{

typedef struct CB_PVRLib
{
  void (*TransferEpgEntry)(void *userData, const ADDON_HANDLE handle, const EPG_TAG *epgentry);
  void (*TransferChannelEntry)(void *userData, const ADDON_HANDLE handle, const PVR_CHANNEL *chan);
  void (*TransferTimerEntry)(void *userData, const ADDON_HANDLE handle, const PVR_TIMER *timer);
  void (*TransferRecordingEntry)(void *userData, const ADDON_HANDLE handle, const PVR_RECORDING *recording);
  void (*AddMenuHook)(void *addonData, PVR_MENUHOOK *hook);
  void (*Recording)(void *addonData, const char *Name, const char *FileName, bool On);
  void (*TriggerChannelUpdate)(void *addonData);
  void (*TriggerTimerUpdate)(void *addonData);
  void (*TriggerRecordingUpdate)(void *addonData);
  void (*TriggerChannelGroupsUpdate)(void *addonData);
  void (*TriggerEpgUpdate)(void *addonData, unsigned int iChannelUid);

  void (*TransferChannelGroup)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group);
  void (*TransferChannelGroupMember)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);

  void (*FreeDemuxPacket)(void *addonData, DemuxPacket* pPacket);
  DemuxPacket* (*AllocateDemuxPacket)(void *addonData, int iDataSize);
  
  void (*ConnectionStateChange)(void* addonData, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage);
  void (*EpgEventStateChange)(void* addonData, EPG_TAG* tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState);
} CB_PVRLib;

} /* namespace PVR */
} /* namespace V1 */
} /* namespace KodiAPI */

class CHelper_libXBMC_pvr
{
public:
  CHelper_libXBMC_pvr(void)
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libXBMC_pvr(void)
  {
    if (m_Handle && m_Callbacks)
    {
      m_Handle->PVRLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (KodiAPI::V1::PVR::CB_PVRLib*)m_Handle->PVRLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me can't get callback table from Kodi !!!\n");
  
    return m_Callbacks != NULL;
  }

  /*!
   * @brief Transfer an EPG tag from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the EPG data
   * @param entry The entry to transfer to XBMC
   */
  void TransferEpgEntry(const ADDON_HANDLE handle, const EPG_TAG* entry)
  {
    return m_Callbacks->TransferEpgEntry(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the channel list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry)
  {
    return m_Callbacks->TransferChannelEntry(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the timers list
   * @param entry The entry to transfer to XBMC
   */
  void TransferTimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry)
  {
    return m_Callbacks->TransferTimerEntry(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the recordings list
   * @param entry The entry to transfer to XBMC
   */
  void TransferRecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry)
  {
    return m_Callbacks->TransferRecordingEntry(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Transfer a channel group from the add-on to XBMC. The group will be created if it doesn't exist.
   * @param handle The handle parameter that XBMC used when requesting the channel groups list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry)
  {
    return m_Callbacks->TransferChannelGroup(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Transfer a channel group member entry from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param handle The handle parameter that XBMC used when requesting the channel group members list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry)
  {
    return m_Callbacks->TransferChannelGroupMember(m_Handle->addonData, handle, entry);
  }

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param hook The hook to add
   */
  void AddMenuHook(PVR_MENUHOOK* hook)
  {
    return m_Callbacks->AddMenuHook(m_Handle->addonData, hook);
  }

  /*!
   * @brief Display a notification in XBMC that a recording started or stopped on the server
   * @param strRecordingName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOn True when recording started, false when it stopped
   */
  void Recording(const char* strRecordingName, const char* strFileName, bool bOn)
  {
    return m_Callbacks->Recording(m_Handle->addonData, strRecordingName, strFileName, bOn);
  }

  /*!
   * @brief Request XBMC to update it's list of timers
   */
  void TriggerTimerUpdate(void)
  {
    return m_Callbacks->TriggerTimerUpdate(m_Handle->addonData);
  }

  /*!
   * @brief Request XBMC to update it's list of recordings
   */
  void TriggerRecordingUpdate(void)
  {
    return m_Callbacks->TriggerRecordingUpdate(m_Handle->addonData);
  }

  /*!
   * @brief Request XBMC to update it's list of channels
   */
  void TriggerChannelUpdate(void)
  {
    return m_Callbacks->TriggerChannelUpdate(m_Handle->addonData);
  }

  /*!
   * @brief Schedule an EPG update for the given channel channel
   * @param iChannelUid The unique id of the channel for this add-on
   */
  void TriggerEpgUpdate(unsigned int iChannelUid)
  {
    return m_Callbacks->TriggerEpgUpdate(m_Handle->addonData, iChannelUid);
  }

  /*!
   * @brief Request XBMC to update it's list of channel groups
   */
  void TriggerChannelGroupsUpdate(void)
  {
    return m_Callbacks->TriggerChannelGroupsUpdate(m_Handle->addonData);
  }

#ifdef USE_DEMUX
  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return m_Callbacks->FreeDemuxPacket(m_Handle->addonData, pPacket);
  }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet
   */
  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return m_Callbacks->AllocateDemuxPacket(m_Handle->addonData, iDataSize);
  }
#endif

  /*!
   * @brief Notify a state change for a PVR backend connection
   * @param strConnectionString The connection string reported by the backend that can be displayed in the UI.
   * @param newState The new state.
   * @param strMessage A localized addon-defined string representing the new state, that can be displayed
   *        in the UI or NULL if the Kodi-defined default string for the new state shall be displayed.
   */
  void ConnectionStateChange(const char *strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage)
  {
    return m_Callbacks->ConnectionStateChange(m_Handle->addonData, strConnectionString, newState, strMessage);
  }

  /*!
   * @brief Notify a state change for an EPG event
   * @param tag The EPG event.
   * @param iUniqueChannelId The unique id of the channel for the EPG event
   * @param newState The new state. For EPG_EVENT_CREATED and EPG_EVENT_UPDATED, tag must be filled with all available
   *        event data, not just a delta. For EPG_EVENT_DELETED, it is sufficient to fill EPG_TAG.iUniqueBroadcastId
   */
  void EpgEventStateChange(EPG_TAG *tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState)
  {
    return m_Callbacks->EpgEventStateChange(m_Handle->addonData, tag, iUniqueChannelId, newState);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::V1::PVR::CB_PVRLib *m_Callbacks;
};
