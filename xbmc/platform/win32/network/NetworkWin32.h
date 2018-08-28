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
#include "Iphlpapi.h"
#include "utils/stopwatch.h"
#include "threads/CriticalSection.h"

class CNetworkWin32;

class CNetworkInterfaceWin32 : public CNetworkInterface
{
public:
   CNetworkInterfaceWin32(CNetworkWin32* network, const IP_ADAPTER_INFO& adapter);
   ~CNetworkInterfaceWin32(void);

   virtual const std::string& GetName(void) const;

   virtual bool IsEnabled(void) const;
   virtual bool IsConnected(void) const;
   virtual bool IsWireless(void) const;

   virtual std::string GetMacAddress(void) const;
   virtual void GetMacAddressRaw(char rawMac[6]) const;

   virtual bool GetHostMacAddress(unsigned long host, std::string& mac) const;

   virtual std::string GetCurrentIPAddress() const;
   virtual std::string GetCurrentNetmask() const;
   virtual std::string GetCurrentDefaultGateway(void) const;
   virtual std::string GetCurrentWirelessEssId(void) const;

   virtual void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) const;
   virtual void SetSettings(const NetworkAssignment& assignment, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway, const std::string& essId, const std::string& key, const EncMode& encryptionMode);

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void) const;

private:
   void WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);
   IP_ADAPTER_INFO m_adapter;
   CNetworkWin32* m_network;
   std::string m_adaptername;
};

class CNetworkWin32 : public CNetworkBase
{
public:
   CNetworkWin32(CSettings &settings);
   virtual ~CNetworkWin32(void);

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void);

   // Ping remote host
   using CNetworkBase::PingHost;
   virtual bool PingHost(unsigned long host, unsigned int timeout_ms = 2000);

   // Get/set the nameserver(s)
   virtual std::vector<std::string> GetNameServers(void);
   virtual void SetNameServers(const std::vector<std::string>& nameServers);

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

