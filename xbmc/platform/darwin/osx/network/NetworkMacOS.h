/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/network/NetworkPosix.h"

#include <string>
#include <vector>

class CNetworkInterfaceMacOS : public CNetworkInterfacePosix
{
public:
  CNetworkInterfaceMacOS(CNetworkPosix* network,
                         const std::string& interfaceName,
                         char interfaceMacAddrRaw[6]);
  ~CNetworkInterfaceMacOS() override = default;

  std::string GetCurrentDefaultGateway() const override;
  bool GetHostMacAddress(unsigned long host, std::string& mac) const override;
};

class CNetworkMacOS : public CNetworkPosix
{
public:
  CNetworkMacOS();
  ~CNetworkMacOS() override = default;

  bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;
  std::vector<std::string> GetNameServers() override;

private:
  void GetMacAddress(const std::string& interfaceName, char macAddrRaw[6]) override;
  void queryInterfaceList() override;
};
