/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"

#include <string>
#include <vector>

class CNetworkInterfaceWasm : public CNetworkInterface
{
public:
  CNetworkInterfaceWasm() = default;
  ~CNetworkInterfaceWasm() override = default;

  bool IsEnabled() const override { return true; }
  bool IsConnected() const override;

  std::string GetMacAddress() const override { return "00:00:00:00:00:00"; }
  void GetMacAddressRaw(char rawMac[6]) const override;

  bool GetHostMacAddress(unsigned long host, std::string& mac) const override { return false; }

  // The browser doesn't expose the real interface addresses. Returning
  // plausible RFC1918 values keeps CNetworkBase::HasInterfaceForIP() happy
  // for any local-network queries the upper layers may attempt.
  std::string GetCurrentIPAddress() const override { return "127.0.0.1"; }
  std::string GetCurrentNetmask() const override { return "255.0.0.0"; }
  std::string GetCurrentDefaultGateway() const override { return "127.0.0.1"; }
};

class CNetworkWasm : public CNetworkBase
{
public:
  CNetworkWasm();
  ~CNetworkWasm() override;

  std::vector<CNetworkInterface*>& GetInterfaceList() override;
  bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;
  std::vector<std::string> GetNameServers() override;

private:
  CNetworkInterfaceWasm m_iface;
  std::vector<CNetworkInterface*> m_interfaces;
};
