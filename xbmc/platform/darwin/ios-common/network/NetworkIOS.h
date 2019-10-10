/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"

#include <string>
#include <vector>

class CNetworkIOS;

class CNetworkInterfaceIOS : public CNetworkInterface
{
public:
  CNetworkInterfaceIOS(CNetworkIOS* network, std::string interfaceName);
  ~CNetworkInterfaceIOS() override;

  bool IsEnabled() const override;
  bool IsConnected() const override;

  std::string GetMacAddress() const override;
  void GetMacAddressRaw(char rawMac[6]) const override;

  bool GetHostMacAddress(unsigned long host, std::string& mac) const override;

  std::string GetCurrentIPAddress() const override;
  std::string GetCurrentNetmask() const override;
  std::string GetCurrentDefaultGateway() const override;

  std::string GetInterfaceName() const;

private:
  std::string m_interfaceName;
  CNetworkIOS* m_network;
};

class CNetworkIOS : public CNetworkBase
{
public:
  CNetworkIOS();
  ~CNetworkIOS() override;

  // Return the list of interfaces
  std::vector<CNetworkInterface*>& GetInterfaceList() override;
  CNetworkInterface* GetFirstConnectedInterface() override;

  // Ping remote host
  bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;

  // Get/set the nameserver(s)
  std::vector<std::string> GetNameServers() override;

  friend class CNetworkInterfaceIOS;

private:
  int GetSocket() { return m_sock; }
  void queryInterfaceList();
  std::vector<CNetworkInterfaceIOS*> m_interfaces;
  int m_sock;
};

using CNetwork = CNetworkIOS;
