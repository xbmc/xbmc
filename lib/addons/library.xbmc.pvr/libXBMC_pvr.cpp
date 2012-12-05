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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define USE_DEMUX // enable including of the demuxer packet structure

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "../../../addons/library.xbmc.pvr/libXBMC_pvr.h"
#include "addons/AddonCallbacks.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxPacket.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

extern "C"
{

DLLEXPORT void* PVR_register_me(void *hdl)
{
  CB_PVRLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me is called with NULL handle !!!\n");
  else
  {
    cb = ((AddonCB*)hdl)->PVRLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me can't get callback table from XBMC !!!\n");
  }
  return cb;
}

DLLEXPORT void PVR_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->PVRLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_PVRLib*)cb);
}

DLLEXPORT void PVR_transfer_epg_entry(void *hdl, void* cb, const ADDON_HANDLE handle, const EPG_TAG *epgentry)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferEpgEntry(((AddonCB*)hdl)->addonData, handle, epgentry);
}

DLLEXPORT void PVR_transfer_channel_entry(void *hdl, void* cb, const ADDON_HANDLE handle, const PVR_CHANNEL *chan)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferChannelEntry(((AddonCB*)hdl)->addonData, handle, chan);
}

DLLEXPORT void PVR_transfer_timer_entry(void *hdl, void* cb, const ADDON_HANDLE handle, const PVR_TIMER *timer)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferTimerEntry(((AddonCB*)hdl)->addonData, handle, timer);
}

DLLEXPORT void PVR_transfer_recording_entry(void *hdl, void* cb, const ADDON_HANDLE handle, const PVR_RECORDING *recording)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferRecordingEntry(((AddonCB*)hdl)->addonData, handle, recording);
}

DLLEXPORT void PVR_add_menu_hook(void *hdl, void* cb, PVR_MENUHOOK *hook)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->AddMenuHook(((AddonCB*)hdl)->addonData, hook);
}

DLLEXPORT void PVR_recording(void *hdl, void* cb, const char *Name, const char *FileName, bool On)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->Recording(((AddonCB*)hdl)->addonData, Name, FileName, On);
}

DLLEXPORT void PVR_trigger_channel_update(void *hdl, void* cb)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TriggerChannelUpdate(((AddonCB*)hdl)->addonData);
}

DLLEXPORT void PVR_trigger_channel_groups_update(void *hdl, void* cb)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TriggerChannelGroupsUpdate(((AddonCB*)hdl)->addonData);
}

DLLEXPORT void PVR_trigger_timer_update(void *hdl, void* cb)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TriggerTimerUpdate(((AddonCB*)hdl)->addonData);
}

DLLEXPORT void PVR_trigger_recording_update(void *hdl, void* cb)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TriggerRecordingUpdate(((AddonCB*)hdl)->addonData);
}

DLLEXPORT void PVR_trigger_epg_update(void* hdl, void* cb, unsigned int iChannelUid)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TriggerEpgUpdate(((AddonCB*)hdl)->addonData, iChannelUid);
}

DLLEXPORT void PVR_free_demux_packet(void *hdl, void* cb, DemuxPacket* pPacket)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->FreeDemuxPacket(((AddonCB*)hdl)->addonData, pPacket);
}

DLLEXPORT DemuxPacket* PVR_allocate_demux_packet(void *hdl, void* cb, int iDataSize)
{
  if (cb == NULL)
    return NULL;

  return ((CB_PVRLib*)cb)->AllocateDemuxPacket(((AddonCB*)hdl)->addonData, iDataSize);
}

DLLEXPORT void PVR_transfer_channel_group(void *hdl, void* cb, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferChannelGroup(((AddonCB*)hdl)->addonData, handle, group);
}

DLLEXPORT void PVR_transfer_channel_group_member(void *hdl, void* cb, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member)
{
  if (cb == NULL)
    return;

  ((CB_PVRLib*)cb)->TransferChannelGroupMember(((AddonCB*)hdl)->addonData, handle, member);
}

};
