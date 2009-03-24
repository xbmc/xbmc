/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "Network.h"
#include "../Application.h"
#include "../lib/libscrobbler/scrobbler.h"
#include "RssReader.h"
#include "Zeroconf.h"

using namespace std;

CNetwork::CNetwork()
{
   g_application.getApplicationMessenger().NetworkMessage(SERVICES_UP, 0);
}

CNetwork::~CNetwork()
{
   g_application.getApplicationMessenger().NetworkMessage(SERVICES_DOWN, 0);
}

int CNetwork::ParseHex(char *str, unsigned char *addr)
{
   int len = 0;

   while (*str)
   {
      int tmp;
      if (str[1] == 0)
         return -1;
      if (sscanf(str, "%02x", (unsigned int *)&tmp) != 1)
         return -1;
      addr[len] = tmp;
      len++;
      str += 2;
   }

   return len;
}

CNetworkInterface* CNetwork::GetFirstConnectedInterface()
{
   vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->IsConnected())
         return iface;
      ++iter;
   }

   return NULL;
}

bool CNetwork::IsAvailable(bool wait /*= false*/)
{
  if (wait)
  {
    // NOTE: Not implemented in linuxport branch as 99.9% of the time
    //       we have the network setup already.  Trunk code has a busy
    //       wait for 5 seconds here.
  }

  vector<CNetworkInterface*>& ifaces = GetInterfaceList();
  return (ifaces.size() != 0);
}

bool CNetwork::IsConnected()
{
   return GetFirstConnectedInterface() != NULL;
}

CNetworkInterface* CNetwork::GetInterfaceByName(CStdString& name)
{
   vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->GetName().Equals(name))
         return iface;
      ++iter;
   }

   return NULL;
}

void CNetwork::NetworkMessage(EMESSAGE message, DWORD dwParam)
{
  switch( message )
  {
    case SERVICES_UP:
    {
      CLog::Log(LOGDEBUG, "%s - Starting network services",__FUNCTION__);
#ifdef HAS_TIME_SERVER
      g_application.StartTimeServer();
#endif
#ifdef HAS_WEB_SERVER
      g_application.StartWebServer();
#endif
#ifdef HAS_FTP_SERVER
      g_application.StartFtpServer();
#endif
#ifdef HAS_UPNP
      g_application.StartUPnP();
#endif
#ifdef HAS_EVENT_SERVER
      g_application.StartEventServer();
#endif
#ifdef HAS_DBUS_SERVER
      g_application.StartDbusServer();
#endif
#ifdef HAS_ZEROCONF
      CZeroconf::GetInstance()->Start();
#endif
      CScrobbler::GetInstance()->Init();
      g_rssManager.Start();
    }
    break;
    case SERVICES_DOWN:
    {
      CLog::Log(LOGDEBUG, "%s - Stopping network services",__FUNCTION__);
#ifdef HAS_TIME_SERVER
      g_application.StopTimeServer();
#endif
#ifdef HAS_WEB_SERVER
      g_application.StopWebServer();
#endif
#ifdef HAS_FTP_SERVER
      g_application.StopFtpServer();
#endif
#ifdef HAS_UPNP
      g_application.StopUPnP();
#endif
#ifdef HAS_EVENT_SERVER
      g_application.StopEventServer();
#endif
#ifdef HAS_DBUS_SERVER
      g_application.StopDbusServer();
#endif
#ifdef HAS_ZEROCONF
      CZeroconf::GetInstance()->Stop();
#endif      
      CScrobbler::GetInstance()->Term();
      // smb.Deinit(); if any file is open over samba this will break.

      g_rssManager.Stop();
    }
    break;
  }
}

