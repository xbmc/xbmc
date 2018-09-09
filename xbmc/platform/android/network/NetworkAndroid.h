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

#include <androidjni/Network.h>
#include <androidjni/NetworkInfo.h>
#include <androidjni/LinkProperties.h>
#include <androidjni/RouteInfo.h>
#include <androidjni/NetworkInterface.h>

class CNetworkAndroid;

class CNetworkInterfaceAndroid : public CNetworkInterface
{
public:
  CNetworkInterfaceAndroid(CJNINetwork network, CJNILinkProperties lp, CJNINetworkInterface intf);
  std::vector<std::string> GetNameServers();

  // CNetworkInterface interface
public:
  virtual const std::string& GetName() const override;
  virtual bool IsEnabled() const override;
  virtual bool IsConnected() const override;
  virtual bool IsWireless() const override;
  virtual std::string GetMacAddress() const override;
  virtual void GetMacAddressRaw(char rawMac[6]) const override;
  virtual bool GetHostMacAddress(unsigned long host_ip, std::string& mac) const override;
  virtual std::string GetCurrentIPAddress() const override;
  virtual std::string GetCurrentNetmask() const override;
  virtual std::string GetCurrentDefaultGateway() const override;
  virtual std::string GetCurrentWirelessEssId() const override;
  virtual std::vector<NetworkAccessPoint> GetAccessPoints() const override;
  virtual void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) const override;
  virtual void SetSettings(const NetworkAssignment& assignment, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway, const std::string& essId, const std::string& key, const EncMode& encryptionMode) override;

  std::string GetHostName();

protected:
  std::string m_name;
  CJNINetwork m_network;
  CJNILinkProperties m_lp;
  CJNINetworkInterface m_intf;
};


class CNetworkAndroid : public CNetworkBase
{
  friend class CXBMCApp;

public:
  CNetworkAndroid();
  ~CNetworkAndroid();

  // CNetwork interface
public:
  virtual bool GetHostName(std::string& hostname) override;
  virtual std::vector<CNetworkInterface*>& GetInterfaceList() override;
  virtual CNetworkInterface* GetFirstConnectedInterface() override;
  virtual std::vector<std::string> GetNameServers() override;
  virtual void SetNameServers(const std::vector<std::string>& nameServers) override;

  // Ping remote host
  using CNetworkBase::PingHost;
  virtual bool PingHost(unsigned long remote_ip, unsigned int timeout_ms = 2000) override;

protected:
  void RetrieveInterfaces();
  std::vector<CNetworkInterface*> m_interfaces;
  std::vector<CNetworkInterface*> m_oldInterfaces;
  CCriticalSection m_refreshMutex;
};

using CNetwork = CNetworkAndroid;
