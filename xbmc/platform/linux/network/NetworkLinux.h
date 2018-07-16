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
#include <cstdio>
#include "network/Network.h"

class CNetworkLinux;

class CNetworkInterfaceLinux : public CNetworkInterface
{
public:
   CNetworkInterfaceLinux(CNetworkLinux* network, std::string interfaceName, char interfaceMacAddrRaw[6]);
   ~CNetworkInterfaceLinux(void) override;

   std::string& GetName(void) override;

   bool IsEnabled(void) override;
   bool IsConnected(void) override;
   bool IsWireless(void) override;

   std::string GetMacAddress(void) override;
   void GetMacAddressRaw(char rawMac[6]) override;

   bool GetHostMacAddress(unsigned long host, std::string& mac) override;

   std::string GetCurrentIPAddress() override;
   std::string GetCurrentNetmask() override;
   std::string GetCurrentDefaultGateway(void) override;
   std::string GetCurrentWirelessEssId(void) override;

   void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) override;
   void SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) override;

   // Returns the list of access points in the area
   std::vector<NetworkAccessPoint> GetAccessPoints(void) override;

private:
   void WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);
   std::string     m_interfaceName;
   std::string     m_interfaceMacAdr;
   char           m_interfaceMacAddrRaw[6];
   CNetworkLinux* m_network;
};

class CNetworkLinux : public CNetworkBase
{
public:
   CNetworkLinux(CSettings &settings);
   ~CNetworkLinux(void) override;

   // Return the list of interfaces
   std::vector<CNetworkInterface*>& GetInterfaceList(void) override;
   CNetworkInterface* GetFirstConnectedInterface(void) override;

   // Ping remote host
   bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;

   // Get/set the nameserver(s)
   std::vector<std::string> GetNameServers(void) override;
   void SetNameServers(const std::vector<std::string>& nameServers) override;

   friend class CNetworkInterfaceLinux;

private:
   int GetSocket() { return m_sock; }
   void GetMacAddress(const std::string& interfaceName, char macAddrRaw[6]);
   void queryInterfaceList();
   std::vector<CNetworkInterface*> m_interfaces;
   int m_sock;
};

using CNetwork = CNetworkLinux;

