#pragma once


struct network_info
{
  char ip[32];
  char gateway[32];
  char subnet[32];
  char DNS1[32];
  char DNS2[32];
  bool DHCP;
  char dhcpserver[32];
};

class CNetwork
{
public:  
  CNetwork(void);
  ~CNetwork(void);

  /* overrides dashboard set network information */
  /* this will actually change what is set in dashboard */
  void OverrideDash( struct network_info& networkinfo );

  /* initializes network settings */
  bool Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer);

  /* waits for network to finish init */
  bool WaitForSetup(DWORD timeout);

  bool IsEthernetConnected();
  bool IsAvailable() { return m_networkup; }

  /* updates and returns current network state */
  /* will return pending if network is not up and running */
  /* should really be called once in a while should network */
  /* be unplugged */
  DWORD UpdateState();


  struct network_info m_networkinfo;
protected:
  bool m_networkup;  /* true if network is available */
  DWORD m_laststate; /* will hold the last state, to notice changes */
  DWORD m_lastlink;  /* will hold the last link, to notice changes */
};

extern CNetwork g_network;