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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "libXBMC_pvr.h"
#include "AddonHelpers_local.h"

AddonCB *m_pvr_cb = NULL;

void PVR_register_me(ADDON_HANDLE hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_pvr-ERROR: PVR_register_me is called with NULL handle !!!\n");
  else
    m_pvr_cb = (AddonCB*) hdl;
  return;
}

void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferEpgEntry(m_pvr_cb->addonData, handle, epgentry);
}

void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferChannelEntry(m_pvr_cb->addonData, handle, chan);
}

void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferTimerEntry(m_pvr_cb->addonData, handle, timer);
}

void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.TransferRecordingEntry(m_pvr_cb->addonData, handle, recording);
}

void PVR_add_menu_hook(PVR_MENUHOOK *hook)
{
  if (m_pvr_cb == NULL)
    return;

  m_pvr_cb->PVR.AddMenuHook(m_pvr_cb->addonData, hook);
}
