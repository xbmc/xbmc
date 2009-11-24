#pragma once
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

#ifndef LIBPVR_PLUS_PLUS_H
#define LIBPVR_PLUS_PLUS_H

#include <vector>
#include <string>
#include "xbmc_pvr_types.h"

void PVR_register_me(ADDON_HANDLE hdl);
void PVR_event_callback(const PVR_EVENT event, const char* msg);
void PVR_reset_player();
void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan);
void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer);
void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
bool PVR_add_demux_stream(const PVRDEMUXHANDLE handle, const PVR_DEMUXSTREAMINFO *demux);
void PVR_delete_demux_stream(const PVRDEMUXHANDLE handle, int index);
void PVR_delete_demux_streams(const PVRDEMUXHANDLE handle);
void PVR_free_demux_packet(demux_packet* pPacket);
demux_packet* PVR_allocate_demux_packet(int iDataSize = 0);

#endif /* LIBPVR_PLUS_PLUS_H */
