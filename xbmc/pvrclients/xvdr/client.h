#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include "addons/library.xbmc.addon/libXBMC_addon.h"
#include "addons/library.xbmc.gui/libXBMC_gui.h"
#include "addons/library.xbmc.pvr/libXBMC_pvr.h"

#define DEFAULT_HOST           "127.0.0.1"
#define DEFAULT_PORT           34891
#define DEFAULT_CHARCONV       false
#define DEFAULT_HANDLE_MSG     true
#define DEFAULT_PRIORITY       99
#define DEFAULT_TIMEOUT        3
#define DEFAULT_AUTOGROUPS     false
#define DEFAULT_COMPRESSION    6
#define DEFAULT_AUDIOTYPE      0
#define DEFAULT_UPDATECHANNELS 3

extern bool         m_bCreated;
extern std::string  g_szHostname;         ///< hostname or ip-address of the server
extern int          g_iConnectTimeout;    ///< Network connection / read timeout in seconds
extern int          g_iPriority;          ///< The Priority this client have in response to other clients
extern bool         g_bCharsetConv;       ///< Convert VDR's incoming strings to UTF8 character set
extern bool         g_bHandleMessages;    ///< Send VDR's OSD status messages to XBMC OSD
extern int          g_iCompression;       ///< Protocol Compression Level
extern int          g_iAudioType;         ///< Preferred audio stream type
extern int          g_iUpdateChannels;    ///< Channel scan method

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_gui   *GUI;
extern CHelper_libXBMC_pvr   *PVR;
