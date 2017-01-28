#ifndef NETWORK_LINUX_H_
#define NETWORK_LINUX_H_

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <forward_list>
#include <cstdio>
#include "network/Network.h"

class CNetworkLinux;
class CNetworkLinuxUpdateThread;

class CNetworkInterfaceLinux : public CNetworkInterface
{
  friend class CNetworkLinux;

public:
   CNetworkInterfaceLinux(CNetworkLinux* network, unsigned int ifa_flags, struct sockaddr *address,
                          struct sockaddr *netmask, std::string interfaceName, char interfaceMacAddrRaw[6]);
   ~CNetworkInterfaceLinux(void);

   virtual std::string& GetName(void);

   virtual bool IsEnabled(void);
   virtual bool IsConnected(void);
   virtual bool IsWireless(void);

   virtual std::string GetMacAddress(void);
   virtual void GetMacAddressRaw(char rawMac[6]);

   virtual bool GetHostMacAddress(unsigned long host, std::string& mac);

   virtual std::string GetCurrentIPAddress();
   virtual std::string GetCurrentNetmask();
   virtual std::string GetCurrentDefaultGateway(void);
   virtual std::string GetCurrentWirelessEssId(void);

   virtual void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);
   virtual void SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);

   bool isIPv6() { return m_address->sa_family == AF_INET6; }
   bool isIPv4() { return m_address->sa_family == AF_INET;  }

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void);

   bool Exists(const struct sockaddr *address, const struct sockaddr *netmask, const std::string &name)
   {
    return CNetwork::CompareAddresses(m_address, address) && CNetwork::CompareAddresses(m_netmask, netmask) && name == m_interfaceName;
   }

protected:
   void SetRemoved(bool removed = true) { m_removed = removed; }
   bool IsRemoved(void) { return m_removed; }

private:
   void WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);
   unsigned int    m_interfaceFlags;   /* Flags from SIOCGIFFLAGS */
  struct sockaddr* m_address;
  struct sockaddr* m_netmask;
   bool            m_removed;
   std::string     m_interfaceName;
   std::string     m_interfaceMacAdr;
   char           m_interfaceMacAddrRaw[6];
   CNetworkLinux* m_network;
};

class CNetworkLinux : public CNetwork
{
   friend class CNetworkInterfaceLinux;

public:
   CNetworkLinux(void);
   virtual ~CNetworkLinux(void);

   virtual std::forward_list<CNetworkInterface*>& GetInterfaceList(void);
   virtual CNetworkInterface* GetFirstConnectedInterface(void);

   virtual bool SupportsIPv6() { return true; }

   virtual bool PingHostImpl(const std::string &target, unsigned int timeout_ms = 2000);

   // Get/set the nameserver(s)
   // Current code is safe for any stack configuration, but APi
   // used provides IPv4 nameservers only (those specified in system
   // via IPv4 address). empty otherwise.
   // TODO: find a method to get list of all defined nameservers
   virtual std::vector<std::string> GetNameServers(void);
   virtual void SetNameServers(const std::vector<std::string>& nameServers);

   bool ForceRereadInterfaces() { return queryInterfaceList(); }
private:
   CNetworkInterfaceLinux *Exists(const struct sockaddr *addr, const struct sockaddr *mask, const std::string &name);
   void InterfacesClear(void);
   void DeleteRemoved(void);

   int GetSocket() { return m_sock; }
   void GetMacAddress(struct ifaddrs *tif, char *mac);
   bool queryInterfaceList();
   std::forward_list<CNetworkInterface*> m_interfaces;
   int m_sock;

   static bool IsRemoved(const CNetworkInterface *i) { return ((CNetworkInterfaceLinux*)i)->IsRemoved(); }
};

#endif

void WatcherProcess(void *caller);
