/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWasm.h"

#include "platform/wasm/WasmWebApis.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdio>
#include <cstring>

#include <emscripten.h>

using namespace KODI::PLATFORM::WASM;
using namespace std::chrono_literals;

namespace
{
// Each snapshot is a synchronous round trip to the browser main thread, and
// IsConnected() is polled by several services.
constexpr auto REFRESH_INTERVAL = 3s;

std::string DescribeLink(const TizenNetworkInfo& info)
{
  using ConnectionType = TizenNetworkInfo::ConnectionType;

  std::string desc;
  switch (info.connectionType)
  {
    case ConnectionType::DISCONNECTED:
      return "disconnected";
    case ConnectionType::WIFI:
      desc = StringUtils::Format("Wi-Fi '{}' (signal {}/5", info.wifiSsid, info.wifiSignalLevel);
      if (info.wifiFrequencyMHz > 0)
        desc += StringUtils::Format(", {} MHz", info.wifiFrequencyMHz);
      desc += ")";
      break;
    case ConnectionType::CELLULAR:
      desc = "cellular";
      break;
    case ConnectionType::ETHERNET:
      desc = "Ethernet";
      break;
    case ConnectionType::UNKNOWN:
      desc = "unknown link";
      break;
  }

  if (!info.ipMode.empty())
    desc += ", " + info.ipMode;
  desc += info.gatewayConnected ? ", gateway reachable" : ", gateway unreachable";
  return desc;
}
} // namespace

TizenNetworkInfo CNetworkInterfaceWasm::GetInfo() const
{
  std::unique_lock lock(m_mutex);
  const auto now = std::chrono::steady_clock::now();
  if (m_refreshed != std::chrono::steady_clock::time_point{} &&
      now - m_refreshed < REFRESH_INTERVAL)
    return m_info;

  lock.unlock();
  TizenNetworkInfo info = CTizenWebApis::GetNetworkInfo();
  lock.lock();

  m_info = info;
  m_refreshed = now;

  if (info.available)
  {
    const std::string state = DescribeLink(info);
    if (state != m_loggedState)
    {
      m_loggedState = state;
      CLog::Log(LOGINFO, "Tizen network: {}, IP {}, gateway {}, DNS {} {}", state, info.ipAddress,
                info.gateway, info.dns1, info.dns2);
    }
  }

  return info;
}

bool CNetworkInterfaceWasm::IsConnected() const
{
  const TizenNetworkInfo info = GetInfo();
  if (info.available)
    return info.connectionType != TizenNetworkInfo::ConnectionType::DISCONNECTED &&
           !info.ipAddress.empty();

  return EM_ASM_INT({
    return (typeof navigator !== "undefined" && typeof navigator.onLine === "boolean")
             ? (navigator.onLine ? 1 : 0)
             : 1;
  }) != 0;
}

std::string CNetworkInterfaceWasm::GetMacAddress() const
{
  const TizenNetworkInfo info = GetInfo();
  if (info.available)
    return info.macAddress;

  const std::string macAddress = CWasmWebApis::GetDeviceInfo().macAddress;
  return macAddress.empty() ? "00:00:00:00:00:00" : macAddress;
}

void CNetworkInterfaceWasm::GetMacAddressRaw(char rawMac[6]) const
{
  std::memset(rawMac, 0, 6);

  unsigned int bytes[6];
  if (std::sscanf(GetMacAddress().c_str(), "%x:%x:%x:%x:%x:%x", &bytes[0], &bytes[1], &bytes[2],
                  &bytes[3], &bytes[4], &bytes[5]) != 6)
    return;

  for (int i = 0; i < 6; ++i)
    rawMac[i] = static_cast<char>(bytes[i]);
}

std::string CNetworkInterfaceWasm::GetCurrentIPAddress() const
{
  const TizenNetworkInfo info = GetInfo();
  return info.available ? info.ipAddress : "127.0.0.1";
}

std::string CNetworkInterfaceWasm::GetCurrentNetmask() const
{
  const TizenNetworkInfo info = GetInfo();
  return info.available ? info.netmask : "255.0.0.0";
}

std::string CNetworkInterfaceWasm::GetCurrentDefaultGateway() const
{
  const TizenNetworkInfo info = GetInfo();
  return info.available ? info.gateway : "127.0.0.1";
}

std::vector<std::string> CNetworkInterfaceWasm::GetNameServers() const
{
  const TizenNetworkInfo info = GetInfo();

  std::vector<std::string> servers;
  if (!info.dns1.empty())
    servers.push_back(info.dns1);
  if (!info.dns2.empty() && info.dns2 != info.dns1)
    servers.push_back(info.dns2);
  return servers;
}

std::string CNetworkInterfaceWasm::GetTVName() const
{
  return GetInfo().tvName;
}

CNetworkWasm::CNetworkWasm()
{
  m_interfaces.push_back(&m_iface);
}

CNetworkWasm::~CNetworkWasm() = default;

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkWasm>();
}

bool CNetworkWasm::GetHostName(std::string& hostname)
{
  const std::string tvName = m_iface.GetTVName();
  if (!tvName.empty())
  {
    hostname = tvName;
    return true;
  }

  return CNetworkBase::GetHostName(hostname);
}

std::vector<CNetworkInterface*>& CNetworkWasm::GetInterfaceList()
{
  return m_interfaces;
}

bool CNetworkWasm::PingHost(unsigned long /*host*/, unsigned int /*timeout_ms*/)
{
  return false;
}

std::vector<std::string> CNetworkWasm::GetNameServers()
{
  return m_iface.GetNameServers();
}
