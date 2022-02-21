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

#include <string>
#include <vector>

#include <IPTypes.h>
#include <winrt/Windows.Networking.Connectivity.h>

class CNetworkWin10;

class CNetworkInterfaceWin10 : public CNetworkInterface
{
public:
  CNetworkInterfaceWin10(const PIP_ADAPTER_ADDRESSES adapter);
  ~CNetworkInterfaceWin10(void);

  virtual bool IsEnabled(void) const;
  virtual bool IsConnected(void) const;

  virtual std::string GetMacAddress(void) const;
  virtual void GetMacAddressRaw(char rawMac[6]) const;

  virtual bool GetHostMacAddress(unsigned long host, std::string& mac) const;

  virtual std::string GetCurrentIPAddress() const;
  virtual std::string GetCurrentNetmask() const;
  virtual std::string GetCurrentDefaultGateway(void) const;

private:
  PIP_ADAPTER_ADDRESSES m_adapterAddr;
};


class CNetworkWin10 : public CNetworkBase
{
public:
    CNetworkWin10();
    virtual ~CNetworkWin10(void);

    std::vector<CNetworkInterface*>& GetInterfaceList(void) override;
    CNetworkInterface* GetFirstConnectedInterface() override;
    std::vector<std::string> GetNameServers(void) override;

    bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) override;

    friend class CNetworkInterfaceWin10;

private:
    int GetSocket() { return m_sock; }
    void queryInterfaceList();
    void CleanInterfaceList();

    std::vector<CNetworkInterface*> m_interfaces;
    int m_sock;
    CCriticalSection m_critSection;
    std::vector<uint8_t> m_adapterAddresses;
};

