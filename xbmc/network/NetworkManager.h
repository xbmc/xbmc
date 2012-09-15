#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "INetworkManager.h"

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

  // Return true if the magic packet was send
  bool WakeOnLan(const char *mac);

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
};
