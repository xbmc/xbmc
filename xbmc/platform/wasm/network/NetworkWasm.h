/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"

#include "platform/wasm/TizenWebApis.h"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

/*!
 * \brief The single network interface of the wasm build.
 *
 * On a Samsung TV the addresses come from the Tizen Network API. Plain browsers
 * expose nothing beyond navigator.onLine, so loopback placeholders are reported
 * there; they keep CNetworkBase::HasInterfaceForIP() usable for local queries.
 */
class CNetworkInterfaceWasm : public CNetworkInterface
{
public:
  CNetworkInterfaceWasm() = default;
  ~CNetworkInterfaceWasm() override = default;

  bool IsEnabled() const override { return true; }
  bool IsConnected() const override;

  std::string GetMacAddress() const override;
  void GetMacAddressRaw(char rawMac[6]) const override;

  bool GetHostMacAddress(unsigned long host, std::string& mac) const override { return false; }

  std::string GetCurrentIPAddress() const override;
  std::string GetCurrentNetmask() const override;
  std::string GetCurrentDefaultGateway() const override;

  std::vector<std::string> GetNameServers() const;
  std::string GetTVName() const;

private:
  // Returns a copy so callers never hold the lock while proxying to the main thread.
  KODI::PLATFORM::WASM::TizenNetworkInfo GetInfo() const;

  mutable std::mutex m_mutex;
  mutable KODI::PLATFORM::WASM::TizenNetworkInfo m_info;
  mutable std::chrono::steady_clock::time_point m_refreshed;
  mutable std::string m_loggedState;
};

class CNetworkWasm : public CNetworkBase
{
public:
  CNetworkWasm();
  ~CNetworkWasm() override;

  bool GetHostName(std::string& hostname) override;
  std::vector<CNetworkInterface*>& GetInterfaceList() override;
  bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;
  std::vector<std::string> GetNameServers() override;

private:
  CNetworkInterfaceWasm m_iface;
  std::vector<CNetworkInterface*> m_interfaces;
};
