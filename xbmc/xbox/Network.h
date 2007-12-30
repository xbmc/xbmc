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

  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN
  };

  CNetwork(void);
  ~CNetwork(void);

  /* initializes network settings */
  bool Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer);
  void Deinitialize();

  /* waits for network to finish init */
  bool WaitForSetup(DWORD timeout);

  bool IsEthernetConnected();
  bool IsAvailable(bool wait = true);
  bool IsInited() { return m_inited; }

  /* updates and returns current network state */
  /* will return pending if network is not up and running */
  /* should really be called once in a while should network */
  /* be unplugged */
  DWORD UpdateState();
  void LogState();
  
  /* callback from application controlled thread to handle any setup */
  void NetworkMessage(EMESSAGE message, DWORD dwParam);

  struct network_info m_networkinfo;
protected:
  bool m_networkup;  /* true if network is available */
  bool m_inited;     /* true if initalized() has been called */
  DWORD m_laststate; /* will hold the last state, to notice changes */
  DWORD m_lastlink;  /* will hold the last link, to notice changes */

private:

  void NetworkDown();
  void NetworkUp();
};


extern CNetwork g_network;