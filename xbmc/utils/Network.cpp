#include "stdafx.h"
#include "Network.h"
#include "../Application.h"
#include "../lib/libscrobbler/scrobbler.h"

int CNetwork::ParseHex(char *str, unsigned char *addr)
{
   int len = 0;

   while (*str) 
   {
      int tmp;
      if (str[1] == 0)
         return -1;
      if (sscanf(str, "%02x", &tmp) != 1)
         return -1;
      addr[len] = tmp;
      len++;
      str += 2;
   }
  
   return len;
}

CNetworkInterface* CNetwork::GetFirstConnectedInterface()
{
   std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->IsConnected())
         return iface;
      ++iter;
   }
   
   return NULL;
}
   
bool CNetwork::IsAvailable()
{
   std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   return (ifaces.size() != 0);
}
   
bool CNetwork::IsConnected()
{
   return GetFirstConnectedInterface() != NULL;
}

CNetworkInterface* CNetwork::GetInterfaceByName(CStdString& name)
{
   std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
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
#ifdef HAS_KAI
      if (m_gWindowManager.GetActiveWindow() != WINDOW_LOGIN_SCREEN)
        g_application.StartKai();
#endif
#ifdef HAS_UPNP
      g_application.StartUPnP();
#endif
      CScrobbler::GetInstance()->Init();
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
#ifndef HAS_UPNP
      g_application.StopUPnP();
#endif
      CScrobbler::GetInstance()->Term();
      // smb.Deinit(); if any file is open over samba this will break.
    }
    break;
  }
}

