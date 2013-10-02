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
#if defined(TARGET_WINDOWS)
#include "xbmc/network/IConnection.h"
#include "Iphlpapi.h"

class CWinConnection : public IConnection
{
public:
  CWinConnection(IP_ADAPTER_INFO adapter);
  virtual ~CWinConnection();

  virtual std::string     GetName() const;
  virtual std::string     GetAddress() const;
  virtual std::string     GetNetmask() const;
  virtual std::string     GetGateway() const;
  virtual std::string     GetNameServer() const;
  virtual std::string     GetMacAddress() const;
  virtual void            GetMacAddressRaw(char rawMac[6]) const;
  virtual std::string     GetInterfaceName() const;

  virtual ConnectionType  GetType() const;
  virtual unsigned int    GetSpeed() const;
  virtual ConnectionState GetState() const;
  virtual IPConfigMethod  GetMethod() const;
  virtual unsigned int    GetStrength() const;
  virtual EncryptionType  GetEncryption() const;
  virtual bool            Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig);
private:
  void GetNameServerInternal();
  IP_ADAPTER_INFO m_adapter;
  std::string m_nameserver;
};
#endif
