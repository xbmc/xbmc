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

#include <string>
#include "StdString.h"
#include "pvrclient-vdr_os.h"
#include "../../addons/include/xbmc_addon_lib++.h"
#include "../../addons/include/xbmc_pvr_lib++.h"

#define DEFAULT_HOST        "127.0.0.1"
#define DEFAULT_PORT        2004
#define DEFAULT_FTA_ONLY    false
#define DEFAULT_RADIO       true
#define DEFAULT_CHARCONV    false
#define DEFAULT_TIMEOUT     3
#define DEFAULT_BADCHANNELS true
#define DEFAULT_HANDLE_MSG  true

#ifdef __cplusplus
extern "C" {
#endif

extern bool         m_bCreated;
extern std::string  m_sHostname;
extern int          m_iPort;
extern bool         m_bOnlyFTA;
extern bool         m_bRadioEnabled;
extern bool         m_bCharsetConv;
extern int          m_iConnectTimeout;
extern bool         m_bNoBadChannels;
extern bool         m_bHandleMessages;
extern int          g_clientID;

#ifdef __cplusplus
}
#endif

#endif /* CLIENT_H */
