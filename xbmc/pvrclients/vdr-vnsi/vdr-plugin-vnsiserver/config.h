/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef _STREAMERNG_CONFIG_H_
#define _STREAMERNG_CONFIG_H_

#include <string.h>
#include <stdint.h>

#include <vdr/config.h>

#if CONSOLEDEBUG
  #define LOGCONSOLE(x...) do{ fprintf(stderr, "VERBOSE %s - ", __FUNCTION__); fprintf(stderr, x); fprintf(stderr, "\n"); } while(0)
#else
  #define LOGCONSOLE(x...)
#endif

#define ALLOWED_HOSTS_FILE  "allowed_hosts.conf"
#define NO_SIGNAL_FILE      "noSignal.mpg"
#define FRONTEND_DEVICE     "/dev/dvb/adapter%d/frontend%d"

#define LISTEN_PORT       34890
#define LISTEN_PORT_S    "34890"
#define DISCOVERY_PORT    34890

enum eSuspendMode
{
  smOffer,
  smAlways,
  smNever,
  sm_Count
};


class cVNSIServerConfig
{
public:
  cVNSIServerConfig();

  void readNoSignalStream();

  // Remote server settings
  int  listen_port;         // Port of remote server
  int  SuspendMode;

  cString ConfigDirectory;

  uint8_t m_noSignalStreamData[ 6 + 0xffff ];
  long    m_noSignalStreamSize;
};

// Global instance
extern cVNSIServerConfig VNSIServerConfig;

#endif /* _STREAMERNG_CONFIG_H_ */
