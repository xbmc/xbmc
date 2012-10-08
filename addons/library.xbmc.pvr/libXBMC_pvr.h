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
#define PVR_HELPER_DLL "/library.xbmc.pvr/libXBMC_pvr-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#endif

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

class CHelper_libXBMC_pvr
{
public:
  CHelper_libXBMC_pvr()
  {
    m_libXBMC_pvr = NULL;
    m_Handle      = NULL;
  }

  ~CHelper_libXBMC_pvr()
  {
    if (m_libXBMC_pvr)
    {
      PVR_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libXBMC_pvr);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += PVR_HELPER_DLL;

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

  void TransferEpgEntry(const ADDON_HANDLE handle, const EPG_TAG *epgentry)
  {
    return PVR_transfer_epg_entry(m_Handle, m_Callbacks, handle, epgentry);
  }

  void TransferChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL *chan)
  {
    return PVR_transfer_channel_entry(m_Handle, m_Callbacks, handle, chan);
  }

  void TransferTimerEntry(const ADDON_HANDLE handle, const PVR_TIMER *timer)
  {
    return PVR_transfer_timer_entry(m_Handle, m_Callbacks, handle, timer);
  }

  void TransferRecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING *recording)
  {
    return PVR_transfer_recording_entry(m_Handle, m_Callbacks, handle, recording);
  }

  void AddMenuHook(PVR_MENUHOOK *hook)
  {
    return PVR_add_menu_hook(m_Handle, m_Callbacks, hook);
  }

  void Recording(const char *Name, const char *FileName, bool On)
  {
    return PVR_recording(m_Handle, m_Callbacks, Name, FileName, On);
  }

  void TriggerTimerUpdate()
  {
    return PVR_trigger_timer_update(m_Handle, m_Callbacks);
  }

  void TriggerRecordingUpdate()
  {
    return PVR_trigger_recording_update(m_Handle, m_Callbacks);
  }

  void TriggerChannelUpdate()
  {
    return PVR_trigger_channel_update(m_Handle, m_Callbacks);
  }

  void TriggerChannelGroupsUpdate()
  {
    return PVR_trigger_channel_groups_update(m_Handle, m_Callbacks);
  }

  void TransferChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
  {
    return PVR_transfer_channel_group(m_Handle, m_Callbacks, handle, group);
  }

  void TransferChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
  {
    return PVR_transfer_channel_group_member(m_Handle, m_Callbacks, handle, member);
  }

#ifdef USE_DEMUX
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return PVR_free_demux_packet(m_Handle, m_Callbacks, pPacket);
  }

  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return PVR_allocate_demux_packet(m_Handle, m_Callbacks, iDataSize);
  }
#endif

protected:
  void* (*PVR_register_me)(void *HANDLE);
  void (*PVR_unregister_me)(void* HANDLE, void* CB);
  void (*PVR_transfer_epg_entry)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const EPG_TAG *epgentry);
  void (*PVR_transfer_channel_entry)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL *chan);
  void (*PVR_transfer_timer_entry)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_TIMER *timer);
  void (*PVR_transfer_recording_entry)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_RECORDING *recording);
  void (*PVR_add_menu_hook)(void* HANDLE, void* CB, PVR_MENUHOOK *hook);
  void (*PVR_recording)(void* HANDLE, void* CB, const char *Name, const char *FileName, bool On);
  void (*PVR_trigger_channel_update)(void* HANDLE, void* CB);
  void (*PVR_trigger_channel_groups_update)(void* HANDLE, void* CB);
  void (*PVR_trigger_timer_update)(void* HANDLE, void* CB);
  void (*PVR_trigger_recording_update)(void* HANDLE, void* CB);
  void (*PVR_transfer_channel_group)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group);
  void (*PVR_transfer_channel_group_member)(void* HANDLE, void* CB, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);
#ifdef USE_DEMUX
  void (*PVR_free_demux_packet)(void* HANDLE, void* CB, DemuxPacket* pPacket);
  DemuxPacket* (*PVR_allocate_demux_packet)(void* HANDLE, void* CB, int iDataSize);
#endif

private:
  void *m_libXBMC_pvr;
  void *m_Handle;
  void *m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};
