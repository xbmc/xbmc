#include <stdio.h>
#include <stdlib.h>
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#define __cdecl
#define __declspec(x)
#include <time.h>
#endif
#endif

#include <stdarg.h>
#include "xbmc_pvr_lib++.h"
#include "addon_local.h"

using namespace std;

AddonCB *m_pvr_cb = NULL;

void PVR_register_me(ADDON_HANDLE hdl)
{
  m_pvr_cb = (AddonCB*) hdl;

  return;
}

void PVR_event_callback(const PVR_EVENT event, const char* msg)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.EventCallback(m_pvr_cb->userData, event, msg);
}

void PVR_reset_player()
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.ResetPlayer(m_pvr_cb->userData);
}

void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferEpgEntry(m_pvr_cb->userData, handle, epgentry);
}

void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferChannelEntry(m_pvr_cb->userData, handle, chan);
}

void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferTimerEntry(m_pvr_cb->userData, handle, timer);
}

void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferRecordingEntry(m_pvr_cb->userData, handle, recording);
}
