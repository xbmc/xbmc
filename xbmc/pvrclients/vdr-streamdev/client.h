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

#ifndef CLIENT_H
#define CLIENT_H

#include "StdString.h"
#include "../../../addons/library.xbmc.addon/libXBMC_addon.h"
#include "../../../addons/library.xbmc.pvr/libXBMC_pvr.h"

#define DEFAULT_HOST          "127.0.0.1"
#define DEFAULT_PORT          2004
#define DEFAULT_FTA_ONLY      false
#define DEFAULT_RADIO         true
#define DEFAULT_CHARCONV      false
#define DEFAULT_BADCHANNELS   true
#define DEFAULT_HANDLE_MSG    true
#define DEFAULT_PRIORITY      99
#define DEFAULT_TIMEOUT       3
#define DEFAULT_USE_REC_DIR   false
#define DEFAULT_REC_DIR       ""

extern bool         g_bCreated;           ///< Shows that the Create function was successfully called
extern int          g_iClientID;          ///< The PVR client ID used by XBMC for this driver
extern CStdString   g_szUserPath;         ///< The Path to the user directory inside user profile
extern CStdString   g_szClientPath;       ///< The Path where this driver is located

/* Client Settings */
extern CStdString   g_szHostname;         ///< The Host name or IP of the VDR
extern int          g_iPort;              ///< The VTP Port of the VDR (default is 2004)
extern int          g_iPriority;          ///< The Priority this client have in response to other clients
extern int          g_iConnectTimeout;    ///< The Socket connection timeout
extern bool         g_bOnlyFTA;           ///< Send only Free-To-Air Channels inside Channel list to XBMC
extern bool         g_bRadioEnabled;      ///< Send also Radio channels list to XBMC
extern bool         g_bCharsetConv;       ///< Convert VDR's incoming strings to UTF8 character set
extern bool         g_bNoBadChannels;     ///< Ignore channels without a PID, APID and DPID
extern bool         g_bHandleMessages;    ///< Send VDR's OSD status messages to XBMC OSD
extern bool         g_bUseRecordingsDir;  ///< Use a normal directory if true for recordings
extern CStdString   g_szRecordingsDir;    ///< The path to the recordings directory
extern cHelper_libXBMC_addon *XBMC;
extern cHelper_libXBMC_pvr   *PVR;

#endif /* CLIENT_H */
