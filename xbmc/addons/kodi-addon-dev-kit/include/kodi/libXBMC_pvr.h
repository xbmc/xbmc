/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
      m_Callbacks = (AddonInstance_PVR*)m_Handle->PVRLib_RegisterMe(m_Handle->addonData);
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
    return m_Callbacks->toKodi->TransferEpgEntry(m_Callbacks->toKodi->kodiInstance, handle, entry);
  }

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the channel list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry)
  {
    return m_Callbacks->toKodi->TransferChannelEntry(m_Callbacks->toKodi->kodiInstance, handle,
                                                     entry);
  }

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the timers list
   * @param entry The entry to transfer to XBMC
   */
  void TransferTimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry)
  {
    return m_Callbacks->toKodi->TransferTimerEntry(m_Callbacks->toKodi->kodiInstance, handle,
                                                   entry);
  }

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the recordings list
   * @param entry The entry to transfer to XBMC
   */
  void TransferRecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry)
  {
    return m_Callbacks->toKodi->TransferRecordingEntry(m_Callbacks->toKodi->kodiInstance, handle,
                                                       entry);
  }

  /*!
   * @brief Transfer a channel group from the add-on to XBMC. The group will be created if it doesn't exist.
   * @param handle The handle parameter that XBMC used when requesting the channel groups list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry)
  {
    return m_Callbacks->toKodi->TransferChannelGroup(m_Callbacks->toKodi->kodiInstance, handle,
                                                     entry);
  }

  /*!
   * @brief Transfer a channel group member entry from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param handle The handle parameter that XBMC used when requesting the channel group members list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry)
  {
    return m_Callbacks->toKodi->TransferChannelGroupMember(m_Callbacks->toKodi->kodiInstance,
                                                           handle, entry);
  }

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param hook The hook to add
   */
  void AddMenuHook(PVR_MENUHOOK* hook)
  {
    return m_Callbacks->toKodi->AddMenuHook(m_Callbacks->toKodi->kodiInstance, hook);
  }

  /*!
   * @brief Display a notification in XBMC that a recording started or stopped on the server
   * @param strRecordingName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOn True when recording started, false when it stopped
   */
  void Recording(const char* strRecordingName, const char* strFileName, bool bOn)
  {
    return m_Callbacks->toKodi->Recording(m_Callbacks->toKodi->kodiInstance, strRecordingName,
                                          strFileName, bOn);
  }

  /*!
   * @brief Request XBMC to update it's list of timers
   */
  void TriggerTimerUpdate(void)
  {
    return m_Callbacks->toKodi->TriggerTimerUpdate(m_Callbacks->toKodi->kodiInstance);
  }

  /*!
   * @brief Request XBMC to update it's list of recordings
   */
  void TriggerRecordingUpdate(void)
  {
    return m_Callbacks->toKodi->TriggerRecordingUpdate(m_Callbacks->toKodi->kodiInstance);
  }

  /*!
   * @brief Request XBMC to update it's list of channels
   */
  void TriggerChannelUpdate(void)
  {
    return m_Callbacks->toKodi->TriggerChannelUpdate(m_Callbacks->toKodi->kodiInstance);
  }

  /*!
   * @brief Schedule an EPG update for the given channel channel
   * @param iChannelUid The unique id of the channel for this add-on
   */
  void TriggerEpgUpdate(unsigned int iChannelUid)
  {
    return m_Callbacks->toKodi->TriggerEpgUpdate(m_Callbacks->toKodi->kodiInstance, iChannelUid);
  }

  /*!
   * @brief Request XBMC to update it's list of channel groups
   */
  void TriggerChannelGroupsUpdate(void)
  {
    return m_Callbacks->toKodi->TriggerChannelGroupsUpdate(m_Callbacks->toKodi->kodiInstance);
  }

#ifdef USE_DEMUX
  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return m_Callbacks->toKodi->FreeDemuxPacket(m_Callbacks->toKodi->kodiInstance, pPacket);
  }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet
   */
  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return m_Callbacks->toKodi->AllocateDemuxPacket(m_Callbacks->toKodi->kodiInstance, iDataSize);
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
    return m_Callbacks->toKodi->ConnectionStateChange(m_Callbacks->toKodi->kodiInstance,
                                                      strConnectionString, newState, strMessage);
  }

  /*!
   * @brief Notify a state change for an EPG event
   * @param tag The EPG event.
   * @param newState The new state. For EPG_EVENT_CREATED and EPG_EVENT_UPDATED, tag must be filled with all available
   *        event data, not just a delta. For EPG_EVENT_DELETED, it is sufficient to fill EPG_TAG.iUniqueBroadcastId
   */
  void EpgEventStateChange(EPG_TAG *tag, EPG_EVENT_STATE newState)
  {
    return m_Callbacks->toKodi->EpgEventStateChange(m_Callbacks->toKodi->kodiInstance, tag,
                                                    newState);
  }

  /*!
   * @brief Get the codec id used by XBMC
   * @param strCodecName The name of the codec
   * @return The codec_id, or a codec_id with 0 values when not supported
   */
  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    return m_Callbacks->toKodi->GetCodecByName(m_Callbacks->toKodi->kodiInstance, strCodecName);
  }

private:
  AddonCB* m_Handle;
  AddonInstance_PVR *m_Callbacks;
};
