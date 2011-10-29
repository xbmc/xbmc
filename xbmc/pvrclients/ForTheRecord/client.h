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

#ifndef CLIENT_H
#define CLIENT_H

#include "utils/StdString.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

#define DEFAULT_HOST                  "127.0.0.1"
#define DEFAULT_PORT                  49943
#define DEFAULT_RADIO                 true
#define DEFAULT_TIMEOUT               10
#define DEFAULT_USER                  "Guest"
#define DEFAULT_PASS                  ""

extern bool         g_bCreated;           ///< Shows that the Create function was successfully called
extern int          g_iClientID;          ///< The PVR client ID used by XBMC for this driver
extern std::string  g_szUserPath;         ///< The Path to the user directory inside user profile
extern std::string  g_szClientPath;       ///< The Path where this driver is located

/* Client Settings */
extern std::string  g_szHostname;
extern int          g_iPort;
extern int          g_iConnectTimeout;
extern bool         g_bRadioEnabled;
extern std::string  g_szUser;
extern std::string  g_szPass;

extern std::string  g_szBaseURL;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr   *PVR;

#endif /* CLIENT_H */
