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


#include "system.h"
#ifdef HAS_DBUS
#include "network/IConnection.h"
#include "linux/DBusUtil.h"
#include <dbus/dbus.h>

class CConnmanConnection : public IConnection
{
public:
  CConnmanConnection(const char *serviceObject);
  virtual ~CConnmanConnection();

  virtual std::string     GetName()       const;
  virtual std::string     GetAddress()    const;
  virtual std::string     GetNetmask()    const;
  virtual std::string     GetGateway()    const;
  virtual std::string     GetNameServer() const;
  virtual std::string     GetMacAddress() const;
  virtual void            GetMacAddressRaw(char rawMac[6]) const;
  virtual std::string     GetInterfaceName() const;

  virtual ConnectionType  GetType()       const;
  virtual ConnectionState GetState()      const;
  virtual unsigned int    GetSpeed()      const;
  virtual IPConfigMethod  GetMethod()     const;
  virtual unsigned int    GetStrength()   const;
  virtual EncryptionType  GetEncryption() const;

  virtual bool            Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig);

  bool PumpNetworkEvents();
  static ConnectionState ParseConnectionState(const char *stateString);

private:
  void UpdateConnection();

  std::string     m_name;
  std::string     m_address;
  std::string     m_netmask;
  std::string     m_gateway;
  std::string     m_macaddress;

  ConnectionType  m_type;
  ConnectionState m_state;
  unsigned int    m_speed;
  std::string     m_method;
  unsigned int    m_strength;
  EncryptionType  m_encryption;
  std::string     m_passphrase;

  CVariant        m_properties;
  std::string     m_serviceObject;

  DBusConnection *m_connection;
  DBusError       m_error;
};
#endif
