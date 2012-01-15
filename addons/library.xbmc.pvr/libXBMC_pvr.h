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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#define PVR_HELPER_DLL "/library.xbmc.pvr/libXBMC_pvr-" ADDON_HELPER_ARCH "-" ADDON_HELPER_PLATFORM ADDON_HELPER_EXT
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
      PVR_unregister_me();
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

    PVR_register_me         = (int (*)(void *HANDLE))
      dlsym(m_libXBMC_pvr, "PVR_register_me");
    if (PVR_register_me == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_unregister_me       = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_unregister_me");
    if (PVR_unregister_me == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferEpgEntry        = (void (*)(const PVR_HANDLE handle, const EPG_TAG *epgentry))
      dlsym(m_libXBMC_pvr, "PVR_transfer_epg_entry");
    if (TransferEpgEntry == NULL)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelEntry    = (void (*)(const PVR_HANDLE handle, const PVR_CHANNEL *chan))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_entry");
    if (TransferChannelEntry == NULL)   { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferTimerEntry      = (void (*)(const PVR_HANDLE handle, const PVR_TIMER *timer))
      dlsym(m_libXBMC_pvr, "PVR_transfer_timer_entry");
    if (TransferTimerEntry == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferRecordingEntry  = (void (*)(const PVR_HANDLE handle, const PVR_RECORDING *recording))
      dlsym(m_libXBMC_pvr, "PVR_transfer_recording_entry");
    if (TransferRecordingEntry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AddMenuHook             = (void (*)(PVR_MENUHOOK *hook))
      dlsym(m_libXBMC_pvr, "PVR_add_menu_hook");
    if (AddMenuHook == NULL)            { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Recording               = (void (*)(const char *Name, const char *FileName, bool On))
      dlsym(m_libXBMC_pvr, "PVR_recording");
    if (Recording == NULL)              { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TriggerTimerUpdate      = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_trigger_timer_update");
    if (TriggerTimerUpdate == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TriggerRecordingUpdate  = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_trigger_recording_update");
    if (TriggerRecordingUpdate == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TriggerChannelUpdate  = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_trigger_channel_update");
    if (TriggerChannelUpdate == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TriggerChannelGroupsUpdate  = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_trigger_channel_groups_update");
    if (TriggerChannelGroupsUpdate == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelGroup  = (void (*)(const PVR_HANDLE handle, const PVR_CHANNEL_GROUP *group))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group");
    if (TransferChannelGroup == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelGroupMember  = (void (*)(const PVR_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group_member");
    if (TransferChannelGroupMember == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

#ifdef USE_DEMUX
    FreeDemuxPacket         = (void (*)(DemuxPacket* pPacket))
      dlsym(m_libXBMC_pvr, "PVR_free_demux_packet");
    if (FreeDemuxPacket == NULL)        { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AllocateDemuxPacket     = (DemuxPacket* (*)(int iDataSize))
      dlsym(m_libXBMC_pvr, "PVR_allocate_demux_packet");
    if (AllocateDemuxPacket == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }
#endif

    return PVR_register_me(m_Handle) > 0;
  }

  void (*TransferEpgEntry)(const PVR_HANDLE handle, const EPG_TAG *epgentry);
  void (*TransferChannelEntry)(const PVR_HANDLE handle, const PVR_CHANNEL *chan);
  void (*TransferTimerEntry)(const PVR_HANDLE handle, const PVR_TIMER *timer);
  void (*TransferRecordingEntry)(const PVR_HANDLE handle, const PVR_RECORDING *recording);
  void (*AddMenuHook)(PVR_MENUHOOK *hook);
  void (*Recording)(const char *Name, const char *FileName, bool On);
  void (*TriggerTimerUpdate)();
  void (*TriggerRecordingUpdate)();
  void (*TriggerChannelUpdate)();
  void (*TriggerChannelGroupsUpdate)();
  void (*TransferChannelGroup)(const PVR_HANDLE handle, const PVR_CHANNEL_GROUP *group);
  void (*TransferChannelGroupMember)(const PVR_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);
#ifdef USE_DEMUX
  void (*FreeDemuxPacket)(DemuxPacket* pPacket);
  DemuxPacket* (*AllocateDemuxPacket)(int iDataSize);
#endif

protected:
  int (*PVR_register_me)(void *HANDLE);
  void (*PVR_unregister_me)();

private:
  void *m_libXBMC_pvr;
  void *m_Handle;
  struct cb_array
  {
    const char* libPath;
  };
};
