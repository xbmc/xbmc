/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "network/Network.h"
#include "utils/stopwatch.h"
#include "threads/CriticalSection.h"

#include <IPTypes.h>
#include <string>
#include <vector>
#include <winrt/Windows.Networking.Connectivity.h>

class CNetworkWin10;

class CNetworkInterfaceWin10 : public CNetworkInterface
{
public:
  CNetworkInterfaceWin10(CNetworkWin10* network, const PIP_ADAPTER_ADDRESSES adapter, ::IUnknown* winRTadapter);
  ~CNetworkInterfaceWin10(void);

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

  virtual void GetSettings(NetworkAssignment& assignment, std::string& ipAddress
                         , std::string& networkMask, std::string& defaultGateway
                         , std::string& essId, std::string& key, EncMode& encryptionMode);
  virtual void SetSettings(NetworkAssignment& assignment, std::string& ipAddress
                         , std::string& networkMask, std::string& defaultGateway
                         , std::string& essId, std::string& key, EncMode& encryptionMode);

  // Returns the list of access points in the area
  virtual std::vector<NetworkAccessPoint> GetAccessPoints(void);

private:
  CNetworkWin10* m_network;

  std::string m_adaptername;
  PIP_ADAPTER_ADDRESSES m_adapterAddr;
  winrt::Windows::Networking::Connectivity::NetworkAdapter m_winRT = nullptr;
  winrt::Windows::Networking::Connectivity::ConnectionProfile m_profile = nullptr;
};


class CNetworkWin10 : public CNetworkBase
{
public:
    CNetworkWin10(CSettings &settings);
    virtual ~CNetworkWin10(void);

    std::vector<CNetworkInterface*>& GetInterfaceList(void) override;
    CNetworkInterface* GetFirstConnectedInterface() override;
    std::vector<std::string> GetNameServers(void) override;
    void SetNameServers(const std::vector<std::string>& nameServers) override;

    bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;

    friend class CNetworkInterfaceWin10;
private:
    int GetSocket() { return m_sock; }
    void queryInterfaceList();
    void CleanInterfaceList();

    std::vector<CNetworkInterface*> m_interfaces;
    int m_sock;
    CStopWatch m_netrefreshTimer;
    CCriticalSection m_critSection;
    PIP_ADAPTER_ADDRESSES m_adapterAddresses;
};

using CNetwork = CNetworkWin10;

