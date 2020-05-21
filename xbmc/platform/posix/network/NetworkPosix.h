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

class CNetworkPosix;

class CNetworkInterfacePosix : public CNetworkInterface
{
public:
  CNetworkInterfacePosix(CNetworkPosix* network,
                         std::string interfaceName,
                         char interfaceMacAddrRaw[6]);
  virtual ~CNetworkInterfacePosix() override = default;

  bool IsEnabled() const override;
  bool IsConnected() const override;
  std::string GetCurrentIPAddress() const override;
  std::string GetCurrentNetmask() const override;

  std::string GetMacAddress() const override;
  void GetMacAddressRaw(char rawMac[6]) const override;

protected:
  std::string m_interfaceName;
  CNetworkPosix* m_network;

private:
  std::string m_interfaceMacAdr;
  char m_interfaceMacAddrRaw[6];
};

class CNetworkPosix : public CNetworkBase
{
public:
  CNetworkPosix();
  virtual ~CNetworkPosix() override;

  std::vector<CNetworkInterface*>& GetInterfaceList() override;
  CNetworkInterface* GetFirstConnectedInterface() override;

  int GetSocket() { return m_sock; }

protected:
  std::vector<CNetworkInterface*> m_interfaces;

private:
  virtual void GetMacAddress(const std::string& interfaceName, char macAddrRaw[6]) = 0;
  virtual void queryInterfaceList() = 0;
  int m_sock;
};

#if defined(HAS_LINUX_NETWORK)
#include "platform/linux/network/NetworkLinux.h"
#elif defined(HAS_FREEBSD_NETWORK)
#include "platform/freebsd/network/NetworkFreebsd.h"
#elif defined(HAS_OSX_NETWORK)
#include "platform/darwin/osx/network/NetworkOsx.h"
#endif
