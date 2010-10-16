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

#define USE_DEMUX // enable including of the demuxer packet structure

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "../../../addons/library.xbmc.pvr/libXBMC_pvr.h"
#include "addons/AddonHelpers_local.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxPacket.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

AddonCB *m_Handle = NULL;
CB_PVRLib *m_cb   = NULL;

extern "C"
{

DLLEXPORT int PVR_register_me(void *hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me is called with NULL handle !!!\n");
  else
  {
    m_Handle = (AddonCB*) hdl;
    m_cb     = m_Handle->PVRLib_RegisterMe(m_Handle->addonData);
    if (!m_cb)
      fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me can't get callback table from XBMC !!!\n");
    else
      return 1;
  }
  return 0;
}

DLLEXPORT void PVR_unregister_me()
{
  if (m_Handle && m_cb)
    m_Handle->PVRLib_UnRegisterMe(m_Handle->addonData, m_cb);
}

DLLEXPORT void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferEpgEntry(m_Handle->addonData, handle, epgentry);
}

DLLEXPORT void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferChannelEntry(m_Handle->addonData, handle, chan);
}

DLLEXPORT void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferTimerEntry(m_Handle->addonData, handle, timer);
}

DLLEXPORT void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferRecordingEntry(m_Handle->addonData, handle, recording);
}

DLLEXPORT void PVR_add_menu_hook(PVR_MENUHOOK *hook)
{
  if (m_cb == NULL)
    return;

  m_cb->AddMenuHook(m_Handle->addonData, hook);
}

DLLEXPORT void PVR_recording(const char *Name, const char *FileName, bool On)
{
  if (m_cb == NULL)
    return;

  m_cb->Recording(m_Handle->addonData, Name, FileName, On);
}

DLLEXPORT void PVR_trigger_timer_update()
{
  if (m_cb == NULL)
    return;

  m_cb->TriggerTimerUpdate(m_Handle->addonData);
}

DLLEXPORT void PVR_trigger_recording_update()
{
  if (m_cb == NULL)
    return;

  m_cb->TriggerTimerUpdate(m_Handle->addonData);
}

DLLEXPORT void PVR_free_demux_packet(DemuxPacket* pPacket)
{
  if (m_cb == NULL)
    return;

  m_cb->FreeDemuxPacket(m_Handle->addonData, pPacket);
}

DLLEXPORT DemuxPacket* PVR_allocate_demux_packet(int iDataSize)
{
  if (m_cb == NULL)
    return NULL;

  return m_cb->AllocateDemuxPacket(m_Handle->addonData, iDataSize);
}

};
