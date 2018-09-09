/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>
#include "network/Network.h"
#include "utils/stopwatch.h"
#include "threads/CriticalSection.h"

#include <ws2ipdef.h>
#include <Iphlpapi.h>

class CNetworkWin32;

class CNetworkInterfaceWin32 : public CNetworkInterface
{
public:
   CNetworkInterfaceWin32(CNetworkWin32* network, const IP_ADAPTER_ADDRESSES& adapter);
   ~CNetworkInterfaceWin32(void) override;

   const std::string& GetName(void) const override;

   bool IsEnabled(void) const override;
   bool IsConnected(void) const override;
   bool IsWireless(void) const override;

   std::string GetMacAddress(void) const override;
   void GetMacAddressRaw(char rawMac[6]) const override;

   bool GetHostMacAddress(unsigned long host, std::string& mac) const override;
   bool GetHostMacAddress(const struct sockaddr& host, std::string& mac) const;

   std::string GetCurrentIPAddress() const override;
   std::string GetCurrentNetmask() const override;
   std::string GetCurrentDefaultGateway(void) const override;
   std::string GetCurrentWirelessEssId(void) const override;

   void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) const override;
   void SetSettings(const NetworkAssignment& assignment, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway, const std::string& essId, const std::string& key, const EncMode& encryptionMode) override;

   // Returns the list of access points in the area
   std::vector<NetworkAccessPoint> GetAccessPoints(void) const override;

private:
   IP_ADAPTER_ADDRESSES m_adapter;
   CNetworkWin32* m_network;
   std::string m_adaptername;
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
   void SetNameServers(const std::vector<std::string>& nameServers) override;

   friend class CNetworkInterfaceWin32;

private:
   int GetSocket() { return m_sock; }
   void queryInterfaceList();
   void CleanInterfaceList();
   std::vector<CNetworkInterface*> m_interfaces;
   int m_sock;
   CStopWatch m_netrefreshTimer;
   CCriticalSection m_critSection;
};

using CNetwork = CNetworkWin32;

