/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWasm.h"

#include <cstring>

#include <emscripten.h>

bool CNetworkInterfaceWasm::IsConnected() const
{
  return EM_ASM_INT({
    return (typeof navigator !== "undefined" && typeof navigator.onLine === "boolean")
             ? (navigator.onLine ? 1 : 0)
             : 1;
  }) != 0;
}

void CNetworkInterfaceWasm::GetMacAddressRaw(char rawMac[6]) const
{
  std::memset(rawMac, 0, 6);
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
  return {};
}
