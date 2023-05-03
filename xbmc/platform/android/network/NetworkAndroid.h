/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Network.h"
#include "threads/CriticalSection.h"

#include "platform/android/activity/JNIXBMCConnectivityManagerNetworkCallback.h"

#include <androidjni/LinkProperties.h>
#include <androidjni/Network.h>
#include <androidjni/NetworkInterface.h>

class CNetworkInterfaceAndroid : public CNetworkInterface
{
public:
  CNetworkInterfaceAndroid(const CJNINetwork& network,
                           const CJNILinkProperties& lp,
                           const CJNINetworkInterface& intf);
  std::vector<std::string> GetNameServers();

  // CNetworkInterface interface
public:
  bool IsEnabled() const override;
  bool IsConnected() const override;
  std::string GetMacAddress() const override;
  void GetMacAddressRaw(char rawMac[6]) const override;
  bool GetHostMacAddress(unsigned long host_ip, std::string& mac) const override;
  std::string GetCurrentIPAddress() const override;
  std::string GetCurrentNetmask() const override;
  std::string GetCurrentDefaultGateway() const override;

  std::string GetHostName();

protected:
  std::string m_name;
  CJNINetwork m_network;
  CJNILinkProperties m_lp;
  CJNINetworkInterface m_intf;
};

class CNetworkAndroid : public CNetworkBase, public jni::CJNIXBMCConnectivityManagerNetworkCallback
{
  friend class CXBMCApp;

public:
  CNetworkAndroid();
  ~CNetworkAndroid() override;

  // CNetwork interface
public:
  bool GetHostName(std::string& hostname) override;
  std::vector<CNetworkInterface*>& GetInterfaceList() override;
  CNetworkInterface* GetFirstConnectedInterface() override;
  std::vector<std::string> GetNameServers() override;

  // Ping remote host
  using CNetworkBase::PingHost;
  bool PingHost(unsigned long remote_ip, unsigned int timeout_ms = 2000) override;

protected:
  void RetrieveInterfaces();
  std::vector<CNetworkInterface*> m_interfaces;
  std::vector<CNetworkInterface*> m_oldInterfaces;
  CCriticalSection m_refreshMutex;

  std::unique_ptr<CNetworkInterface> m_defaultInterface;

public:
  void onAvailable(const CJNINetwork network) override;
  void onLost(const CJNINetwork network) override;
};
