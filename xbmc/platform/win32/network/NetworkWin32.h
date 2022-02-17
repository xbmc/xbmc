/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"
#include "threads/CriticalSection.h"
#include "utils/stopwatch.h"

#include <string>
#include <vector>

#include <ws2ipdef.h> /* Microsoft can't write standalone headers */
#include <Iphlpapi.h> /* Microsoft can't write standalone headers */

class CNetworkWin32;

class CNetworkInterfaceWin32 : public CNetworkInterface
{
public:
   CNetworkInterfaceWin32(const IP_ADAPTER_ADDRESSES& adapter);
   ~CNetworkInterfaceWin32(void) override;

   bool IsEnabled(void) const override;
   bool IsConnected(void) const override;

   std::string GetMacAddress(void) const override;
   void GetMacAddressRaw(char rawMac[6]) const override;

   bool GetHostMacAddress(unsigned long host, std::string& mac) const override;
   bool GetHostMacAddress(const struct sockaddr& host, std::string& mac) const;

   std::string GetCurrentIPAddress() const override;
   std::string GetCurrentNetmask() const override;
   std::string GetCurrentDefaultGateway(void) const override;

private:
   IP_ADAPTER_ADDRESSES m_adapter;
};

class CNetworkWin32 : public CNetworkBase
{
public:
   CNetworkWin32();
   ~CNetworkWin32(void) override;

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void) override;

   // Ping remote host
   using CNetworkBase::PingHost;
   bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;
   bool PingHost(const struct sockaddr& host, unsigned int timeout_ms = 2000);

   // Get/set the nameserver(s)
   std::vector<std::string> GetNameServers(void) override;

   friend class CNetworkInterfaceWin32;

private:
   int GetSocket() { return m_sock; }
   void queryInterfaceList();
   void CleanInterfaceList();
   std::vector<CNetworkInterface*> m_interfaces;
   int m_sock;
   CStopWatch m_netrefreshTimer;
   CCriticalSection m_critSection;
   std::vector<uint8_t> m_adapterAddresses;
};

using CNetwork = CNetworkWin32;

