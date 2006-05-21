#include "../stdafx.h"

#include "network.h"
#include "Undocumented.h"

// global network variable
CNetwork g_network;

static char* inet_ntoa (struct in_addr in)
{
  static char _inetaddress[32];
  sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
  return _inetaddress;
}


void CNetwork::OverrideDash( struct network_info& networkinfo )
{
  TXNetConfigParams params;
  XNetLoadConfigParams( &params );
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
    TXNetConfigParams oldconfig;
    memcpy(&oldconfig, &params, sizeof(TXNetConfigParams));

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

    CLog::Log(LOGINFO, "requesting DHCP");
  }

  /* if we changed anything, we should write data to persistant memory */
  /* maybe we could avoid this by calling XNetConfig after XNetStartup */
  if( bDirty )
    XNetSaveConfigParams( &params );
}

bool CNetwork::Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{

  WSACleanup();
  XNetCleanup();

  memset(&m_networkinfo , 0, sizeof(m_networkinfo ));
  if (iAssignment == NETWORK_DHCP)
  {
    CLog::Log(LOGNOTICE, "use DHCP");
    m_networkinfo.DHCP = true;
    OverrideDash( m_networkinfo );
  }
  else if (iAssignment == NETWORK_STATIC)
  {
    CLog::Log(LOGNOTICE, "use static ip");
    m_networkinfo.DHCP = false;

    strcpy(m_networkinfo.ip, szLocalAddress);
    strcpy(m_networkinfo.subnet, szLocalSubnet);
    strcpy(m_networkinfo.gateway, szLocalGateway);
    strcpy(m_networkinfo.DNS1, szNameServer);
    OverrideDash( m_networkinfo );
  }
  else
  {
    CLog::Log(LOGWARNING, "use dashboard");
  }

  { /* okey now startup the settings we wish to use */
    XNetStartupParams xnsp;
    memset(&xnsp, 0, sizeof(xnsp));
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

    DWORD dwState = XNetStartup(&xnsp);
    if( dwState != 0 )
    {
      CLog::Log(LOGERROR, __FUNCTION__" - XNetStartup failed with error %d", dwState);
      return false;
    }
  }

  /* startup winsock */
  WSADATA WsaData;
  DWORD dwState = WSAStartup( MAKEWORD(2, 2), &WsaData );
  if( NO_ERROR != dwState )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - WSAStartup failed with error %d", dwState);
    return false;
  }

  return true;
}

/* update network state, call repetedly while return value is XNET_GET_XNADDR_PENDING */
DWORD CNetwork::UpdateState()
{
  XNADDR xna;
  DWORD dwState = XNetGetTitleXnAddr(&xna);
  DWORD dwLink = XNetGetEthernetLinkStatus();

  if( !(dwLink & XNET_ETHERNET_LINK_ACTIVE) )
  {
    if( m_networkup )
    {
      memset(&m_networkinfo, 0, sizeof(m_networkinfo));
      m_lastlink = 0;
      m_laststate = 0;
      m_networkup = false;
    }
    return XNET_GET_XNADDR_NONE;
  }

  if( dwState == XNET_GET_XNADDR_PENDING )
  {
    /* reset current network settings, as we might have lost */
    /* connection */
    if( m_networkup )
    {
      memset(&m_networkinfo, 0, sizeof(m_networkinfo));      
      m_lastlink = 0;
      m_laststate = 0;
      m_networkup = false;
    }

    return XNET_GET_XNADDR_PENDING;
  }


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


  if( m_lastlink != dwState )
  {
    /* check that what type of network is attached */
    if ( dwLink & XNET_ETHERNET_LINK_ACTIVE )
    {

      if ( dwLink & XNET_ETHERNET_LINK_FULL_DUPLEX )
        CLog::Log(LOGINFO, __FUNCTION__" - Link: full duplex");

      if ( dwLink & XNET_ETHERNET_LINK_HALF_DUPLEX )
        CLog::Log(LOGINFO, __FUNCTION__" - Link: half duplex");

      if ( dwLink & XNET_ETHERNET_LINK_100MBPS )
        CLog::Log(LOGINFO, __FUNCTION__" - Link: 100 mbps");

      if ( dwLink & XNET_ETHERNET_LINK_10MBPS )
        CLog::Log(LOGINFO, __FUNCTION__" - Link: 10bmps");
    }
  }

  if( m_laststate != dwState )
  {
    if ( dwState & XNET_GET_XNADDR_STATIC )
    {
      CLog::Log(LOGINFO, __FUNCTION__" - State: static");

      m_networkinfo.DHCP = false;
      m_networkup = true;
    }

    if ( dwState & XNET_GET_XNADDR_DHCP )
    {
      CLog::Log(LOGINFO, __FUNCTION__" - State: dhcp");

      m_networkinfo.DHCP = true;
      m_networkup = true;
    }

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


    CLog::Log(LOGINFO,  __FUNCTION__" - Interface: IP         %s", m_networkinfo.ip);
    CLog::Log(LOGINFO,  __FUNCTION__" - Interface: SUBNET     %s", m_networkinfo.subnet);
    CLog::Log(LOGINFO,  __FUNCTION__" - Interface: GATEWAY    %s", m_networkinfo.gateway);
  //  CLog::Log(LOGINFO,  __FUNCTION__" - DHCPSERVER: %s", m_networkinfo.dhcpserver);
    CLog::Log(LOGINFO,  __FUNCTION__" - Interface: DNS1       %s", m_networkinfo.DNS1);
    CLog::Log(LOGINFO,  __FUNCTION__" - Interface: DNS2       %s", m_networkinfo.DNS2);

  }
  return dwState;
}

bool CNetwork::IsEthernetConnected()
{
  if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
    return false;

  return true;
}

bool CNetwork::WaitForSetup(DWORD timeout)
{
  DWORD timestamp = GetTickCount() + timeout;

  if( !IsEthernetConnected() )
    return false;

  do
  {
    if( UpdateState() != XNET_GET_XNADDR_PENDING )
      return true;
    
    Sleep(100);
  } while( GetTickCount() < timestamp );

  return false;
}

CNetwork::CNetwork(void)
{
  memset(&m_networkinfo, 0, sizeof(m_networkinfo));      
  m_lastlink = 0;
  m_laststate = 0;
  m_networkup = false;
}

CNetwork::~CNetwork(void)
{
}
