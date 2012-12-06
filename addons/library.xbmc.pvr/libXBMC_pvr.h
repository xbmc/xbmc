#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xbmc_pvr_types.h"
#include "../library.xbmc.addon/libXBMC_addon.h"

#ifdef _WIN32
#define PVR_HELPER_DLL "\\library.xbmc.pvr\\libXBMC_pvr" ADDON_HELPER_EXT
#else
#define PVR_HELPER_DLL_NAME "libXBMC_pvr-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#define PVR_HELPER_DLL "/library.xbmc.pvr/" PVR_HELPER_DLL_NAME
#endif

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

class CHelper_libXBMC_pvr
{
public:
  CHelper_libXBMC_pvr(void)
  {
    m_libXBMC_pvr = NULL;
    m_Handle      = NULL;
  }

  ~CHelper_libXBMC_pvr(void)
  {
    if (m_libXBMC_pvr)
    {
      PVR_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libXBMC_pvr);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += PVR_HELPER_DLL;

#if defined(ANDROID)
      struct stat st;
      if(stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + PVR_HELPER_DLL_NAME;
      }
#endif

    m_libXBMC_pvr = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libXBMC_pvr == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    PVR_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libXBMC_pvr, "PVR_register_me");
    if (PVR_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_unregister_me = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_pvr, "PVR_unregister_me");
    if (PVR_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_epg_entry = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const EPG_TAG *epgentry))
      dlsym(m_libXBMC_pvr, "PVR_transfer_epg_entry");
    if (PVR_transfer_epg_entry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_channel_entry = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL *chan))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_entry");
    if (PVR_transfer_channel_entry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_timer_entry = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_TIMER *timer))
      dlsym(m_libXBMC_pvr, "PVR_transfer_timer_entry");
    if (PVR_transfer_timer_entry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_recording_entry = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_RECORDING *recording))
      dlsym(m_libXBMC_pvr, "PVR_transfer_recording_entry");
    if (PVR_transfer_recording_entry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_add_menu_hook = (void (*)(void* HANDLE, void* CB, PVR_MENUHOOK *hook))
      dlsym(m_libXBMC_pvr, "PVR_add_menu_hook");
    if (PVR_add_menu_hook == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_recording = (void (*)(void* HANDLE, void* CB, const char *Name, const char *FileName, bool On))
      dlsym(m_libXBMC_pvr, "PVR_recording");
    if (PVR_recording == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_trigger_timer_update = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_pvr, "PVR_trigger_timer_update");
    if (PVR_trigger_timer_update == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_trigger_recording_update = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_pvr, "PVR_trigger_recording_update");
    if (PVR_trigger_recording_update == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_trigger_channel_update = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_pvr, "PVR_trigger_channel_update");
    if (PVR_trigger_channel_update == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_trigger_channel_groups_update = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_pvr, "PVR_trigger_channel_groups_update");
    if (PVR_trigger_channel_groups_update == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_trigger_epg_update = (void (*)(void* HANDLE, void* CB, unsigned int iChannelUid))
      dlsym(m_libXBMC_pvr, "PVR_trigger_epg_update");
    if (PVR_trigger_epg_update == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_channel_group  = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group");
    if (PVR_transfer_channel_group == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_transfer_channel_group_member = (void (*)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group_member");
    if (PVR_transfer_channel_group_member == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

#ifdef USE_DEMUX
    PVR_free_demux_packet = (void (*)(void* HANDLE, void* CB, DemuxPacket* pPacket))
      dlsym(m_libXBMC_pvr, "PVR_free_demux_packet");
    if (PVR_free_demux_packet == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_allocate_demux_packet = (DemuxPacket* (*)(void* HANDLE, void* CB, int iDataSize))
      dlsym(m_libXBMC_pvr, "PVR_allocate_demux_packet");
    if (PVR_allocate_demux_packet == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }
#endif

    m_Callbacks = PVR_register_me(m_Handle);
    return m_Callbacks != NULL;
  }

  /*!
   * @brief Transfer an EPG tag from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the EPG data
   * @param entry The entry to transfer to XBMC
   */
  void TransferEpgEntry(const ADDON_HANDLE handle, const EPG_TAG* entry)
  {
    return PVR_transfer_epg_entry(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Transfer a channel entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the channel list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry)
  {
    return PVR_transfer_channel_entry(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Transfer a timer entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the timers list
   * @param entry The entry to transfer to XBMC
   */
  void TransferTimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry)
  {
    return PVR_transfer_timer_entry(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Transfer a recording entry from the add-on to XBMC
   * @param handle The handle parameter that XBMC used when requesting the recordings list
   * @param entry The entry to transfer to XBMC
   */
  void TransferRecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry)
  {
    return PVR_transfer_recording_entry(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Transfer a channel group from the add-on to XBMC. The group will be created if it doesn't exist.
   * @param handle The handle parameter that XBMC used when requesting the channel groups list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry)
  {
    return PVR_transfer_channel_group(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Transfer a channel group member entry from the add-on to XBMC. The channel will be added to the group if the group can be found.
   * @param handle The handle parameter that XBMC used when requesting the channel group members list
   * @param entry The entry to transfer to XBMC
   */
  void TransferChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry)
  {
    return PVR_transfer_channel_group_member(m_Handle, m_Callbacks, handle, entry);
  }

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param hook The hook to add
   */
  void AddMenuHook(PVR_MENUHOOK* hook)
  {
    return PVR_add_menu_hook(m_Handle, m_Callbacks, hook);
  }

  /*!
   * @brief Display a notification in XBMC that a recording started or stopped on the server
   * @param strRecordingName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOn True when recording started, false when it stopped
   */
  void Recording(const char* strRecordingName, const char* strFileName, bool bOn)
  {
    return PVR_recording(m_Handle, m_Callbacks, strRecordingName, strFileName, bOn);
  }

  /*!
   * @brief Request XBMC to update it's list of timers
   */
  void TriggerTimerUpdate(void)
  {
    return PVR_trigger_timer_update(m_Handle, m_Callbacks);
  }

  /*!
   * @brief Request XBMC to update it's list of recordings
   */
  void TriggerRecordingUpdate(void)
  {
    return PVR_trigger_recording_update(m_Handle, m_Callbacks);
  }

  /*!
   * @brief Request XBMC to update it's list of channels
   */
  void TriggerChannelUpdate(void)
  {
    return PVR_trigger_channel_update(m_Handle, m_Callbacks);
  }

  /*!
   * @brief Schedule an EPG update for the given channel channel
   * @param iChannelUid The unique id of the channel for this add-on
   */
  void TriggerEpgUpdate(unsigned int iChannelUid)
  {
    return PVR_trigger_epg_update(m_Handle, m_Callbacks, iChannelUid);
  }

  /*!
   * @brief Request XBMC to update it's list of channel groups
   */
  void TriggerChannelGroupsUpdate(void)
  {
    return PVR_trigger_channel_groups_update(m_Handle, m_Callbacks);
  }

#ifdef USE_DEMUX
  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return PVR_free_demux_packet(m_Handle, m_Callbacks, pPacket);
  }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet
   */
  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return PVR_allocate_demux_packet(m_Handle, m_Callbacks, iDataSize);
  }
#endif

protected:
  void* (*PVR_register_me)(void*);
  void (*PVR_unregister_me)(void*, void*);
  void (*PVR_transfer_epg_entry)(void*, void*, const ADDON_HANDLE, const EPG_TAG*);
  void (*PVR_transfer_channel_entry)(void*, void*, const ADDON_HANDLE, const PVR_CHANNEL*);
  void (*PVR_transfer_timer_entry)(void*, void*, const ADDON_HANDLE, const PVR_TIMER*);
  void (*PVR_transfer_recording_entry)(void*, void*, const ADDON_HANDLE, const PVR_RECORDING*);
  void (*PVR_add_menu_hook)(void*, void*, PVR_MENUHOOK*);
  void (*PVR_recording)(void*, void*, const char*, const char*, bool);
  void (*PVR_trigger_channel_update)(void*, void*);
  void (*PVR_trigger_channel_groups_update)(void*, void*);
  void (*PVR_trigger_timer_update)(void*, void*);
  void (*PVR_trigger_recording_update)(void* , void*);
  void (*PVR_trigger_epg_update)(void*, void*, unsigned int);
  void (*PVR_transfer_channel_group)(void*, void*, const ADDON_HANDLE, const PVR_CHANNEL_GROUP*);
  void (*PVR_transfer_channel_group_member)(void*, void*, const ADDON_HANDLE, const PVR_CHANNEL_GROUP_MEMBER*);
#ifdef USE_DEMUX
  void (*PVR_free_demux_packet)(void*, void*, DemuxPacket*);
  DemuxPacket* (*PVR_allocate_demux_packet)(void*, void*, int);
#endif

private:
  void* m_libXBMC_pvr;
  void* m_Handle;
  void* m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};
