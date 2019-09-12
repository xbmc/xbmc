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

#include "settings/lib/ISettingCallback.h"

#include "PlatformDefs.h"

class CNetworkInterface
{
public:
   virtual ~CNetworkInterface() = default;

   virtual bool IsEnabled(void) const = 0;
   virtual bool IsConnected(void) const = 0;

   virtual std::string GetMacAddress(void) const = 0;
   virtual void GetMacAddressRaw(char rawMac[6]) const = 0;

   virtual bool GetHostMacAddress(unsigned long host, std::string& mac) const = 0;

   virtual std::string GetCurrentIPAddress() const = 0;
   virtual std::string GetCurrentNetmask() const = 0;
   virtual std::string GetCurrentDefaultGateway(void) const = 0;
};

class CSettings;
class CNetworkServices;
struct sockaddr;

class CNetworkBase
{
public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN
  };

   CNetworkBase();
   virtual ~CNetworkBase();

   // Get network services
   CNetworkServices& GetServices() { return *m_services; }

   // Return our hostname
   virtual bool GetHostName(std::string& hostname);

   // Return the list of interfaces
   virtual std::vector<CNetworkInterface*>& GetInterfaceList(void) = 0;

   // Return the first interface which is active
   virtual CNetworkInterface* GetFirstConnectedInterface(void);

   // Return true if there is a interface for the same network as address
   bool HasInterfaceForIP(unsigned long address);

   // Return true if there's at least one defined network interface
   bool IsAvailable(void);

   // Return true if there's at least one interface which is connected
   bool IsConnected(void);

   // Return true if the magic packet was send
   bool WakeOnLan(const char *mac);

   // Return true if host replies to ping
   bool PingHost(unsigned long host, unsigned short port, unsigned int timeout_ms = 2000, bool readability_check = false);
   virtual bool PingHost(unsigned long host, unsigned int timeout_ms = 2000) = 0;

   // Get/set the nameserver(s)
   virtual std::vector<std::string> GetNameServers(void) = 0;

   // callback from application controlled thread to handle any setup
   void NetworkMessage(EMESSAGE message, int param);

   static int ParseHex(char *str, unsigned char *addr);

   // Return true if given name or ip address corresponds to localhost
   bool IsLocalHost(const std::string& hostname);

   // Waits for the first network interface to become available
   void WaitForNet();

   /*!
    \brief  IPv6/IPv4 compatible conversion of host IP address
    \param  struct sockaddr
    \return Function converts binary structure sockaddr to std::string.
            It can read sockaddr_in and sockaddr_in6, cast as (sockaddr*).
            IPv4 address is returned in the format x.x.x.x (where x is 0-255),
            IPv6 address is returned in it's canonised form.
            On error (or no IPv6/v4 valid input) empty string is returned.
    */
   static std::string GetIpStr(const sockaddr* sa);

   /*!
    \brief  convert prefix length of IPv4 address to IP mask representation
    \param  prefix length
    \return
   */
   static std::string GetMaskByPrefixLength(uint8_t prefixLength);

  std::unique_ptr<CNetworkServices> m_services;
};

#if defined(TARGET_ANDROID)
#include "platform/android/network/NetworkAndroid.h"
#elif defined(HAS_LINUX_NETWORK)
#include "platform/posix/network/NetworkLinux.h"
#elif defined(HAS_WIN32_NETWORK)
#include "platform/win32/network/NetworkWin32.h"
#elif defined(HAS_WIN10_NETWORK)
#include "platform/win10/network/NetworkWin10.h"
#elif defined(HAS_IOS_NETWORK)
#include "platform/darwin/ios-common/network/NetworkIOS.h"
#else
using CNetwork = CNetworkBase;
#endif

//creates, binds and listens tcp sockets on the desired port. Set bindLocal to
//true to bind to localhost only.
std::vector<SOCKET> CreateTCPServerSocket(const int port, const bool bindLocal, const int backlog, const char *callerName);

