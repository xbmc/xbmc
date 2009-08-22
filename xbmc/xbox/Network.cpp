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
#include "Application.h"
#include "FileSystem/FileSmb.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "RssReader.h"
#ifdef _LINUX
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif
#include <string>

// global network variable
CNetwork g_network;

bool CNetwork::Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{
  m_inited = true;
  return true;
}

void CNetwork::NetworkDown()
{
  memset(&m_networkinfo, 0, sizeof(m_networkinfo));
  m_lastlink = 0;
  m_laststate = 0;
  m_networkup = false;
#ifndef _LINUX
  g_application.getApplicationMessenger().NetworkMessage(SERVICES_DOWN, 0);
#endif
  m_inited = false;
}

void CNetwork::NetworkUp()
{
#ifdef _LINUX
  char buf[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int sock;
  int nInterfaces;
  int i;

  // get a socket handle
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock)
  {
    // get list of interfaces
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if(ioctl(sock, SIOCGIFCONF, &ifc) >= 0)
    {  
      ifr = ifc.ifc_req;
      nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
      for(i = 0; i < nInterfaces; i++)
      {
        struct ifreq *iface = &ifr[i];
        
        strncpy(m_networkinfo.ip, inet_ntoa(((struct sockaddr_in *)&iface->ifr_addr)->sin_addr), sizeof(m_networkinfo.ip));
        strncpy(m_networkinfo.subnet, inet_ntoa(((struct sockaddr_in *)&iface->ifr_netmask)->sin_addr), sizeof(m_networkinfo.subnet));
        if (ioctl(sock, SIOCGIFNETMASK, iface) >= 0)
        {
          strncpy(m_networkinfo.subnet, inet_ntoa(((struct sockaddr_in *)&iface->ifr_netmask)->sin_addr), sizeof(m_networkinfo.subnet));
        }

        if(ioctl(sock, SIOCGIFFLAGS, iface) >= 0)
        {
          if ((iface->ifr_flags & IFF_UP) && (!(iface->ifr_flags & IFF_LOOPBACK)))
          {
            if (strlen(m_networkinfo.ip)>0 && strlen(m_networkinfo.subnet)>0 && strcmp(m_networkinfo.ip, "127.0.0.1")!=0)
            {
              std::string cmd = "ip route list | grep default | grep " + string(iface->ifr_name) + string(" | sed -e 's/default via //g' | sed -e 's/ .*//g'");
              FILE* pipe = popen(cmd.c_str(), "r");
              if (pipe)
              {
                char buffer[512];
                memset(buffer, 0, sizeof(buffer)*sizeof(char));
                fread(buffer, sizeof(buffer)*sizeof(char), 1, pipe);
                pclose(pipe);
                if (strlen(buffer)>0)                  
                {
                  char gateway[512];
                  sscanf(buffer, "%s", gateway);
                  strncpy(m_networkinfo.gateway, gateway, sizeof(m_networkinfo.gateway));
                }
              }
              res_init();
              if (MAXNS>0)
                strncpy(m_networkinfo.DNS1, inet_ntoa(((struct sockaddr_in *)&_res.nsaddr_list[0])->sin_addr), sizeof(m_networkinfo.DNS1));
              if (MAXNS>1)
              strncpy(m_networkinfo.DNS2, inet_ntoa(((struct sockaddr_in *)&_res.nsaddr_list[1])->sin_addr), sizeof(m_networkinfo.DNS2));
              break; 
            }
          }
        }
      }
    }
    close(sock);
  }
#endif
  
  m_networkup = true;
  
  g_application.getApplicationMessenger().NetworkMessage(SERVICES_UP, 0);
}

/* update network state, call repetedly while return value is XNET_GET_XNADDR_PENDING */
DWORD CNetwork::UpdateState()
{
  return 0;
}

bool CNetwork::IsConnected()
{
  return true;
}

bool CNetwork::WaitForSetup(DWORD timeout)
{
  DWORD timestamp = GetTickCount() + timeout;

  if( !IsConnected() )
    return false;

  NetworkUp();
  return true;
}

CNetwork::CNetwork(void)
{
  memset(&m_networkinfo, 0, sizeof(m_networkinfo));      
  m_lastlink = 0;
  m_laststate = 0;
  m_networkup = false;
  m_inited = false;
}

CNetwork::~CNetwork(void)
{
  Deinitialize();
}

void CNetwork::Deinitialize()
{
  if( m_networkup )
    NetworkDown();
}

void CNetwork::LogState()
{
    DWORD dwLink = m_lastlink;
    DWORD dwState = m_laststate;

    CLog::Log(LOGINFO,  "%s - ip: %s", __FUNCTION__, m_networkinfo.ip);
    CLog::Log(LOGINFO,  "%s - subnet: %s", __FUNCTION__, m_networkinfo.subnet);
    CLog::Log(LOGINFO,  "%s - gateway: %s", __FUNCTION__, m_networkinfo.gateway);
  //  CLog::Log(LOGINFO,  __FUNCTION__" - DHCPSERVER: %s", m_networkinfo.dhcpserver);
    CLog::Log(LOGINFO,  "%s - dns: %s, %s", m_networkinfo.DNS1, __FUNCTION__, m_networkinfo.DNS2);

}

bool CNetwork::IsAvailable(bool wait)
{
  /* if network isn't up, wait for it to setup */
  if( !m_networkup && wait )
    WaitForSetup(5000);

  return true;
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
#ifdef HAS_KAI
      if (m_gWindowManager.GetActiveWindow() != WINDOW_LOGIN_SCREEN)
        g_application.StartKai();
#endif
#ifdef HAS_UPNP
      g_application.StartUPnP();
#endif
      CLastfmScrobbler::GetInstance()->Init();
      CLibrefmScrobbler::GetInstance()->Init();
      g_application.StartEventServer();
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
#ifdef HAS_KAI
      g_application.StopKai();   
#endif
#ifdef HAS_UPNP
      g_application.StopUPnP();
#endif
      CLastfmScrobbler::GetInstance()->Term();
      CLibrefmScrobbler::GetInstance()->Term();
      g_application.StopEventServer();
      // smb.Deinit(); if any file is open over samba this will break.

      g_rssManager.Stop();
    }
    break;
  }
}
