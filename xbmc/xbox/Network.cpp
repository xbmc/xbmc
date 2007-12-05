#include "stdafx.h"

#include "Network.h"
#ifdef HAS_XBOX_NETWORK
#include "Undocumented.h"
#endif
#include "../Application.h"
#include "../FileSystem/FileSmb.h"
#include "../lib/libscrobbler/scrobbler.h"
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

#ifdef _XBOX
static char* inet_ntoa (struct in_addr in)
{
  static char _inetaddress[32];
  sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
  return _inetaddress;
}
#endif

#ifdef HAS_XBOX_NETWORK
/* translator function wich will take our network info and make TXNetConfigParams of it */
/* returns true if config is different from default */
static bool TranslateConfig( const struct network_info& networkinfo, TXNetConfigParams &params )
{    
  bool bDirty = false;

  if ( !networkinfo.DHCP )
  {
    bool bXboxVersion2 = (params.V2_Tag == 0x58425632 );  // "XBV2"

    if (bXboxVersion2)
    {
      if (params.V2_IP.s_addr != inet_addr(networkinfo.ip))
      {
        params.V2_IP.s_addr = inet_addr(networkinfo.ip);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_IP.s_addr != inet_addr(networkinfo.ip))
      {
        params.V1_IP.s_addr = inet_addr(networkinfo.ip);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_Subnetmask.s_addr != inet_addr(networkinfo.subnet))
      {
        params.V2_Subnetmask.s_addr = inet_addr(networkinfo.subnet);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_Subnetmask.s_addr != inet_addr(networkinfo.subnet))
      {
        params.V1_Subnetmask.s_addr = inet_addr(networkinfo.subnet);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_Defaultgateway.s_addr != inet_addr(networkinfo.gateway))
      {
        params.V2_Defaultgateway.s_addr = inet_addr(networkinfo.gateway);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_Defaultgateway.s_addr != inet_addr(networkinfo.gateway))
      {
        params.V1_Defaultgateway.s_addr = inet_addr(networkinfo.gateway);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_DNS1.s_addr != inet_addr(networkinfo.DNS1))
      {
        params.V2_DNS1.s_addr = inet_addr(networkinfo.DNS1);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_DNS1.s_addr != inet_addr(networkinfo.DNS1))
      {
        params.V1_DNS1.s_addr = inet_addr(networkinfo.DNS1);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_DNS2.s_addr != inet_addr(networkinfo.DNS2))
      {
        params.V2_DNS2.s_addr = inet_addr(networkinfo.DNS2);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_DNS2.s_addr != inet_addr(networkinfo.DNS2))
      {
        params.V1_DNS2.s_addr = inet_addr(networkinfo.DNS2);
        bDirty = true;
      }
    }

    if (params.Flag != (0x04 | 0x08) )
    {
      params.Flag = 0x04 | 0x08;
      bDirty = true;
    }
  }
  else
  {

    if( params.Flag != 0 )
    {
      params.Flag = 0;
      bDirty = true;
    }

#if 0
    TXNetConfigParams oldconfig;
    memcpy(&oldconfig, &params, sizeof(TXNetConfigParams));

    oldconfig.Flag = 0;

    unsigned char *raw = (unsigned char*)&params;
    
    //memset( raw, 0, sizeof(params)); /* shouldn't be needed, xbox should still remember what ip's where set statically */

    /**     Set DHCP-flags from a known DHCP mode  (maybe some day we will fix this)  **/
    /* i'm guessing these are some dhcp options */
    /* debug dash doesn't set them like this thou */
    /* i have a feeling we don't even need to touch these */
    raw[40] = 33;  raw[41] = 223; raw[42] = 196; raw[43] = 67;    //  param.Data_28
    raw[44] = 6;   raw[45] = 145; raw[46] = 157; raw[47] = 118;   //  param.Data_2c
    raw[48] = 182; raw[49] = 239; raw[50] = 68;  raw[51] = 197;   //  param.Data_30
    raw[52] = 133; raw[53] = 150; raw[54] = 118; raw[55] = 211;   //  param.Data_34
    raw[56] = 38;  raw[57] = 87;  raw[58] = 222; raw[59] = 119;   //  param.Data_38
    
    /* clears static ip flag */
    raw[64] = 0; // first part of params.Flag.. wonder if params.Flag really should be a DWORD
    
    //raw[72] = 0; raw[73] = 0; raw[74] = 0; raw[75] = 0; /* this would have cleared the v2 ip, just silly */
    
    /* no idea what this is, could be additional dhcp options, but's hard to tell */
    raw[340] = 160; raw[341] = 93; raw[342] = 131; raw[343] = 191; raw[344] = 46;

    /* if something was changed, update with this */
    if( memcmp(&oldconfig, &params, sizeof(TXNetConfigParams)) != 0 ) 
      bDirty = true;
#endif
  }

  return bDirty;
}
#endif

bool CNetwork::Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{
#ifdef HAS_XBOX_NETWORK
  XNetStartupParams xnsp = {};
  WSADATA WsaData = {};
  TXNetConfigParams params = {};
  DWORD dwState = 0;
  bool dashconfig = false;

  memset(&m_networkinfo , 0, sizeof(m_networkinfo ));

  /* load current params */
  XNetLoadConfigParams( &params );

  if (iAssignment == NETWORK_DHCP)
  {
    m_networkinfo.DHCP = true;    
    dashconfig = !TranslateConfig(m_networkinfo, params);
    CLog::Log(LOGNOTICE, "use DHCP");    
  }
  else if (iAssignment == NETWORK_STATIC)
  {
    m_networkinfo.DHCP = false;
    strcpy(m_networkinfo.ip, szLocalAddress);
    strcpy(m_networkinfo.subnet, szLocalSubnet);
    strcpy(m_networkinfo.gateway, szLocalGateway);
    strcpy(m_networkinfo.DNS1, szNameServer);

    dashconfig = !TranslateConfig(m_networkinfo, params);
    CLog::Log(LOGNOTICE, "use static ip");
  }
  else
  {
    dashconfig = true;
    CLog::Log(LOGWARNING, "use dashboard");
  }

  /* configure addresses */  
  if( !dashconfig )
  { 
    /* override dashboard setting with this, if it was different */      
    XNetSaveConfigParams( &params );    
  }


  /* okey now startup the settings we wish to use */
  xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

  // Bypass security so that we may connect to 'untrusted' hosts
  xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
  // create more memory for networking
  xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
  xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
  xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
  xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
  xnsp.cfgSockMaxSockets = 64; // default = 64
  xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
  xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

  dwState = XNetStartup(&xnsp);
  if( dwState != 0 )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - XNetStartup failed with error %d", dwState);
    return false;
  }

  if( !dashconfig )
  {
    dwState = XNetConfig( &params, 0 );
    if( dwState != 0 )
    {
      CLog::Log(LOGERROR, __FUNCTION__" - XNetConfig failed with error %d", dwState);
      return false;
    }      
  }

  /* startup winsock */  
  dwState = WSAStartup( MAKEWORD(2, 2), &WsaData );
  if( NO_ERROR != dwState )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - WSAStartup failed with error %d", dwState);
    return false;
  }

#endif
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
#ifdef HAS_XBOX_NETWORK
  
  /* get the current status */
  TXNetConfigStatus status;
  XNetGetConfigStatus(&status);

  /* fill local network info */
  strcpy(m_networkinfo.ip, inet_ntoa(status.ip));
  strcpy(m_networkinfo.subnet, inet_ntoa(status.subnet));
  strcpy(m_networkinfo.gateway, inet_ntoa(status.gateway));
  strcpy(m_networkinfo.dhcpserver, "");
  strcpy(m_networkinfo.DNS1, inet_ntoa(status.dns1));
  strcpy(m_networkinfo.DNS2, inet_ntoa(status.dns2));

  m_networkinfo.DHCP = !(status.dhcp == 0);
#endif

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
#ifdef HAS_XBOX_NETWORK

  if (!m_inited)
    return XNET_GET_XNADDR_NONE;

  XNADDR xna;
  DWORD dwState = XNetGetTitleXnAddr(&xna);
  DWORD dwLink = XNetGetEthernetLinkStatus();

  if( m_lastlink != dwLink || m_laststate != dwState )
  {
    if( m_networkup )
      NetworkDown();

    m_lastlink = dwLink;
    m_laststate = dwState;

    if (dwState & XNET_GET_XNADDR_DHCP || dwState & XNET_GET_XNADDR_STATIC)
      NetworkUp();
    
    LogState();
  }

  return dwState;
#else
  return 0;
#endif
}

bool CNetwork::IsConnected()
{
#ifdef HAS_XBOX_NETWORK
  if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
    return false;
#endif

  return true;
}

bool CNetwork::WaitForSetup(DWORD timeout)
{
  DWORD timestamp = GetTickCount() + timeout;

  if( !IsConnected() )
    return false;

#ifdef HAS_XBOX_NETWORK
  do
  {
    if( UpdateState() != XNET_GET_XNADDR_PENDING && m_inited)
      return true;
    
    Sleep(100);
  } while( GetTickCount() < timestamp );

  return false;
#else
  NetworkUp();
  return true;
#endif
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

#ifdef HAS_XBOX_NETWORK
  WSACleanup();
  XNetCleanup();
#endif
}

void CNetwork::LogState()
{
    DWORD dwLink = m_lastlink;
    DWORD dwState = m_laststate;

#ifdef HAS_XBOX_NETWORK
    if ( dwLink & XNET_ETHERNET_LINK_FULL_DUPLEX )
      CLog::Log(LOGINFO, __FUNCTION__" - Link: full duplex");

    if ( dwLink & XNET_ETHERNET_LINK_HALF_DUPLEX )
      CLog::Log(LOGINFO, __FUNCTION__" - Link: half duplex");

    if ( dwLink & XNET_ETHERNET_LINK_100MBPS )
      CLog::Log(LOGINFO, __FUNCTION__" - Link: 100 mbps");

    if ( dwLink & XNET_ETHERNET_LINK_10MBPS )
      CLog::Log(LOGINFO, __FUNCTION__" - Link: 10bmps");

    if ( dwState & XNET_GET_XNADDR_DNS )
      CLog::Log(LOGINFO, __FUNCTION__" - State: dns");

    if ( dwState & XNET_GET_XNADDR_ETHERNET )
      CLog::Log(LOGINFO, __FUNCTION__" - State: ethernet");

    if ( dwState & XNET_GET_XNADDR_NONE )
      CLog::Log(LOGINFO, __FUNCTION__" - State: none");

    if ( dwState & XNET_GET_XNADDR_ONLINE )
      CLog::Log(LOGINFO, __FUNCTION__" - State: online");

    if ( dwState & XNET_GET_XNADDR_PENDING )
      CLog::Log(LOGINFO, __FUNCTION__" - State: pending");

    if ( dwState & XNET_GET_XNADDR_TROUBLESHOOT )
      CLog::Log(LOGINFO, __FUNCTION__" - State: error");

    if ( dwState & XNET_GET_XNADDR_PPPOE )
      CLog::Log(LOGINFO, __FUNCTION__" - State: ppoe");

    if ( dwState & XNET_GET_XNADDR_STATIC )
      CLog::Log(LOGINFO, __FUNCTION__" - State: static");

    if ( dwState & XNET_GET_XNADDR_DHCP )
      CLog::Log(LOGINFO, __FUNCTION__" - State: dhcp");
#endif
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

#ifdef HAS_XBOX_NETWORK
  return m_networkup;
#else
  return true;
#endif
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
