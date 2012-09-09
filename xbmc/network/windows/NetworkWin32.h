#ifndef NETWORK_LINUX_H_
#define NETWORK_LINUX_H_

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <vector>
#include "utils/StdString.h"
#include "network/Network.h"
#include "Iphlpapi.h"
#include "utils/stopwatch.h"
#include "threads/CriticalSection.h"

class CNetworkWin32;

class CNetworkInterfaceWin32 : public CNetworkInterface
{
public:
   CNetworkInterfaceWin32(CNetworkWin32* network, IP_ADAPTER_INFO adapter);
   ~CNetworkInterfaceWin32(void);

   virtual CStdString& GetName(void);

   virtual bool IsEnabled(void);
   virtual bool IsConnected(void);
   virtual bool IsWireless(void);

   virtual CStdString GetMacAddress(void);
   virtual void GetMacAddressRaw(char rawMac[6]);

   virtual CStdString GetCurrentIPAddress();
   virtual CStdString GetCurrentNetmask();
   virtual CStdString GetCurrentDefaultGateway(void);
   virtual CStdString GetCurrentWirelessEssId(void);

   virtual void GetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode);
   virtual void SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode);

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void);

private:
   void WriteSettings(FILE* fw, NetworkAssignment assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode);
   IP_ADAPTER_INFO m_adapter;
   CNetworkWin32* m_network;
   CStdString m_adaptername;
};

class CNetworkWin32 : public CNetwork
{
public:
   CNetworkWin32(void);
   virtual ~CNetworkWin32(void);

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void);

   // Get/set the nameserver(s)
   virtual std::vector<CStdString> GetNameServers(void);
   virtual void SetNameServers(std::vector<CStdString> nameServers);

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

#endif
