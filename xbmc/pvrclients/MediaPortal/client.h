#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "platform/util/StdString.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

#define DEFAULT_HOST                  "127.0.0.1"
#define DEFAULT_PORT                  9596
#define DEFAULT_FTA_ONLY              false
#define DEFAULT_RADIO                 true
#define DEFAULT_TIMEOUT               10
#define DEFAULT_HANDLE_MSG            false
#define DEFAULT_RESOLVE_RTSP_HOSTNAME true
#define DEFAULT_READ_GENRE            false
#define DEFAULT_SLEEP_RTSP_URL        0
#define DEFAULT_USE_REC_DIR           false
#define DEFAULT_REC_DIR               ""
#define DEFAULT_TVGROUP               ""
#define DEFAULT_RADIOGROUP            ""
#define DEFAULT_DIRECT_TS_FR          false

extern bool         g_bCreated;           ///< Shows that the Create function was successfully called
extern int          g_iClientID;          ///< The PVR client ID used by XBMC for this driver
extern std::string  g_szUserPath;         ///< The Path to the user directory inside user profile
extern std::string  g_szClientPath;       ///< The Path where this driver is located

/* Client Settings */
extern std::string  g_szHostname;
extern int          g_iPort;
extern int          g_iConnectTimeout;
extern int          g_iSleepOnRTSPurl;
extern bool         g_bOnlyFTA;
extern bool         g_bRadioEnabled;
extern bool         g_bHandleMessages;
extern bool         g_bResolveRTSPHostname;
extern bool         g_bReadGenre;
extern bool         g_bUseRecordingsDir;
extern bool         g_bDirectTSFileRead;
extern std::string  g_szRecordingsDir;
extern std::string  g_szTVGroup;
extern std::string  g_szRadioGroup;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;

extern int          g_iTVServerXBMCBuild;

#endif /* CLIENT_H */
