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
#include "threads/Thread.h"

class CNetworkLinux;
class CNetworkLinuxUpdateThread;

class CNetworkInterfaceLinux : public CNetworkInterface
{
  friend class CNetworkLinux;

public:
   CNetworkInterfaceLinux(CNetworkLinux* network, bool sa_ipv6,
                unsigned int ifa_flags, std::string ifa_addr, std::string ifa_netmask,
                std::string interfaceName, char interfaceMacAddrRaw[6]);
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

   bool isIPv6() { return m_interfaceIpv6; }
   bool isIPv4() { return !m_interfaceIpv6; }

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void);

   bool Exists(const std::string &addr, const std::string &mask, const std::string &name)
   {
     return (addr == m_interfaceAddr && mask == m_interfaceNetmask && name == m_interfaceName);
   }

protected:
   void SetRemoved(bool removed = true) { m_removed = removed; }
   bool IsRemoved(void) { return m_removed; }

private:
   void WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode);
   bool            m_interfaceIpv6;
   unsigned int    m_interfaceFlags;   /* Flags from SIOCGIFFLAGS */
   std::string     m_interfaceAddr;    /* Address of interface */
   std::string     m_interfaceNetmask; /* Netmask of interface */
   bool            m_removed;
   std::string     m_interfaceName;
   std::string     m_interfaceMacAdr;
   char           m_interfaceMacAddrRaw[6];
   CNetworkLinux* m_network;
};

class CNetworkLinux : public CNetwork
{
   friend class CNetworkLinuxUpdateThread;
   friend class CNetworkInterfaceLinux;

public:
   CNetworkLinux(void);
   virtual ~CNetworkLinux(void);

   // Return the list of interfaces
   virtual std::forward_list<CNetworkInterface*>& GetInterfaceList(void);
   // list of available interfaces is always sorted: by AF_FAMILY, dependence of their type
   // on other types and alphabetically at last.
   // AF_PACKET comes first, then AF_INET, AF_INET6. physical interfaces (eg network cards)
   // come first. bridges later before virtual itnerfaces, then tunnels ...
   // this assures that ppp0 won't be picked up (presented as GetFirstConnectedInterface())
   // at the expense of eth0 for instance / thus providing nonsense info to services expecting
   // interface with MAC address.
   virtual CNetworkInterface* GetFirstConnectedInterface(void);        

   virtual bool SupportsIPv6() { return true; }

   // Ping remote host
   virtual bool PingHostImpl(const std::string &target, unsigned int timeout_ms = 2000);

   // Get/set the nameserver(s)
   // Current code is safe for any stack configuration, but APi
   // used provides IPv4 nameservers only (those specified in system
   // via IPv4 address). empty otherwise.
   // TODO: find a method to get list of all defined nameservers
   virtual std::vector<std::string> GetNameServers(void);
   virtual void SetNameServers(const std::vector<std::string>& nameServers);

private:
   CNetworkInterfaceLinux *Exists(const std::string &addr, const std::string &mask, const std::string &name);
   void InterfacesClear(void);
   void DeleteRemoved(void);

   int GetSocket() { return m_sock; }
   void GetMacAddress(struct ifaddrs *tif, char *mac);
   bool queryInterfaceList();
   std::forward_list<CNetworkInterface*> m_interfaces;
   int m_sock;

   CNetworkLinuxUpdateThread      *m_updThread;

   static bool IsRemoved(const CNetworkInterface *i) { return ((CNetworkInterfaceLinux*)i)->IsRemoved(); }
};

class CNetworkLinuxUpdateThread : public CThread
{
public:
   CNetworkLinuxUpdateThread(CNetworkLinux *owner);
   virtual ~CNetworkLinuxUpdateThread(void) {};

protected:
   void Process(void);
   CNetworkLinux *m_owner;
};

#endif

