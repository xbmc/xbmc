#pragma once

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

  bool IsConnected();
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

