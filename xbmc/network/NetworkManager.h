#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "INetworkManager.h"

class CStopWatch;

class CNetworkManager : public INetworkEventsCallback
{
public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN
  };

  CNetworkManager();
  virtual ~CNetworkManager();

  void            Initialize();

  bool            PumpNetworkEvents();

  std::string     GetDefaultConnectionName();
  std::string     GetDefaultConnectionAddress();
  std::string     GetDefaultConnectionNetmask();
  std::string     GetDefaultConnectionGateway();
  std::string     GetDefaultConnectionNameServer();
  std::string     GetDefaultConnectionMacAddress();
  void            GetDefaultConnectionMacAddressRaw(char rawMac[6]);
  std::string     GetDefaultConnectionInterfaceName();

  ConnectionType  GetDefaultConnectionType();
  ConnectionState GetDefaultConnectionState();
  IPConfigMethod  GetDefaultConnectionMethod();

  bool            IsConnected();
  bool            IsAvailable(bool wait = false);
  bool            CanManageConnections();

  ConnectionList  GetConnections();

  virtual void    OnConnectionStateChange(ConnectionState state);
  virtual void    OnConnectionChange(CConnectionPtr connection);
  virtual void    OnConnectionListChange(ConnectionList list);

  // callback from application controlled thread to handle any setup
  void NetworkMessage(EMESSAGE message, int param);

  void            StartServices();
  void            StopServices();
private:
  void            StopServices(bool wait);
  const char*     ConnectionStateToString(ConnectionState state);

  INetworkManager *m_instance;
  CConnectionPtr   m_defaultConnection;
  ConnectionList   m_connections;
  ConnectionState  m_state;
  CStopWatch      *m_timer;
};
