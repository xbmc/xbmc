#pragma once
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

#include "system.h"
#include "threads/Event.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "Application.h"
#include "interfaces/AnnouncementManager.h"

#include "settings/lib/ISettingCallback.h"
#include <sys/socket.h>

enum EncMode { ENC_NONE = 0, ENC_WEP = 1, ENC_WPA = 2, ENC_WPA2 = 3 };
enum NetworkAssignment { NETWORK_DASH = 0, NETWORK_DHCP = 1, NETWORK_STATIC = 2, NETWORK_DISABLED = 3 };

class NetworkAccessPoint
{
public:
    NetworkAccessPoint(const std::string &essId, const std::string &macAddress, int signalStrength, EncMode encryption, int channel = 0):
      m_essId(essId),
      m_macAddress(macAddress)
    {
      m_dBm            = signalStrength;
      m_encryptionMode = encryption;
      m_channel        = channel;
   }

   const std::string &getEssId() const { return m_essId; }
   const std::string &getMacAddress() const { return m_macAddress; }
   int getSignalStrength() const { return m_dBm; }
   EncMode getEncryptionMode() const { return m_encryptionMode; }
   int getChannel() const { return m_channel; }

   /*!
    \brief  Returns the quality, normalized as a percentage, of the network access point
    \return The quality as an integer between 0 and 100
    */
   int getQuality() const;

   /*!
    \brief  Translates a 802.11a+g frequency into the corresponding channel
    \param  frequency  The frequency of the channel in units of Hz
    \return The channel as an integer between 1 and 14 (802.11b+g) or
            between 36 and 165 (802.11a), or 0 if unknown.
    */
   static int FreqToChannel(float frequency);

private:
   std::string  m_essId;
   std::string  m_macAddress;
   int         m_dBm;
   EncMode     m_encryptionMode;
   int         m_channel;
};

class CNetworkInterface
{
public:
   virtual ~CNetworkInterface() {};

   virtual std::string& GetName(void) = 0;

   virtual bool IsEnabled(void) = 0;
   virtual bool IsConnected(void) = 0;
   virtual bool IsWireless(void) = 0;

   virtual std::string GetMacAddress(void) = 0;
   virtual void GetMacAddressRaw(char rawMac[6]) = 0;

   virtual bool GetHostMacAddress(unsigned long host, std::string& mac) = 0;
   bool GetHostMacAddress(const std::string &host, std::string& mac);

   virtual std::string GetCurrentIPAddress() = 0;
   virtual std::string GetCurrentNetmask() = 0;
   virtual std::string GetCurrentDefaultGateway(void) = 0;
   virtual std::string GetCurrentWirelessEssId(void) = 0;

   // Returns the list of access points in the area
   virtual std::vector<NetworkAccessPoint> GetAccessPoints(void) = 0;

   virtual void GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) = 0;
   virtual void SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode) = 0;

   // tells if interface itself is configured with IPv6 or IPv4 address
   virtual bool isIPv6() { return false; }
   virtual bool isIPv4() { return true; }
};

class CNetwork
{
public:
  class CNetworkUpdater : public CThread, public ANNOUNCEMENT::IAnnouncer
  {
  public:
    CNetworkUpdater(void (*watcher)(void *caller));
    virtual ~CNetworkUpdater(void);

    volatile bool *Stopping() { return &m_bStop; }
    void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  protected:
    void Process() { m_watcher(this); }

  private:
    void (*m_watcher)(void *caller);
  };

public:
  enum EMESSAGE
  {
    SERVICES_UP,
    SERVICES_DOWN,
    NETWORK_CHANGED
  };

   CNetwork();
   virtual ~CNetwork();

   /*!
    \brief  Returns local hostname
    */
   virtual bool GetHostName(std::string& hostname);

   /*!
    \brief  Return the list of interfaces
    */
   virtual std::forward_list<CNetworkInterface*>& GetInterfaceList(void) = 0;

   /*!
    \brief  Returns interface with specific name
          - in the current implementation, interface as the basic configutation item
            represents specific IP configuration
          - this means InterfaceName is not unique key. IP address is

    \return With respect above comment, it returns first valid configuration on interface
            holding searched interface name.
    */
   CNetworkInterface* GetInterfaceByName(const std::string& name);

   /*!
    \brief  Return the first interface which is active
            - list of available interfaces is always sorted: by AF_FAMILY, dependence of their type
              on other types and alphabetically at last. AF_PACKET comes first, then AF_INET, AF_INET6.
              physical interfaces (eg network cards) come first. Bridges later before virtual
              itnerfaces, then tunnels...
            - This assures that ppp0 won't be picked up (presented as
              GetFirstConnectedInterface()) at the expense of eth0 for instance / thus providing
              non-sense info to services expecting interface with MAC address.
    */
   virtual CNetworkInterface* GetFirstConnectedInterface(void);

    /*!
     \brief  Address family of GetFirstConnectedInterface() interface
     \sa     GetFirstConnectedInterface()
     \return with respect to comment in GetFirstConnectedInterface()
             - in case of returned AF_INET6 - host is configured with IPv6 stack only (we don't need
               iterate over interfaces list further, or trying socket() results to confirm this)
             - AF_INET - host is configured with IPv4 stack. IPv6 availability unknown, we would need
               to loop over the list.
    */
   int GetFirstConnectedFamily() { return (GetFirstConnectedInterface() && GetFirstConnectedInterface()->isIPv6() ? AF_INET6 : AF_INET); }

    /*!
     \brief Return true if there is a interface for the same network as address
    */
   bool HasInterfaceForIP(const std::string &address);

    /*!
     \brief Compat wrapper to
     \sa    HasInterfaceForIP(const std::string &address)
    */
   bool HasInterfaceForIP(unsigned long address);

   // Return true if there's at least one defined network interface
   bool IsAvailable(bool wait = false);

   // Return true if there's at least one interface which is connected
   bool IsConnected(void);

   /*!
    \return  Return true if the magic packet was send

      TODO: current implementation uses ARP for MAC resolution.
            as IPv6 has no ARP, it will provide expected result
            only if target host is accessible via IPv4.
            (anyhow it's use is safe regardless actual.
            configuration, returns false for IPv6 only stack)
    */
   bool WakeOnLan(const char *mac);

   /*!
    \brief  Ping remote host
    \param  IP host address, port (optional)
    \return Return true if host replies to ping, false otherwise
            Function is IPv6/v4 compatible
            - If port is not specified, system cmd ping/ping6 is called
            - If port is specified, host is contacted via sock connect()
    */
   bool PingHost(const std::string &ipaddr, unsigned short port, unsigned int timeout_ms = 2000, bool readability_check = false);

   /*!
    \brief  Ping remote host (compatibility wrapper)
    \sa     PingHost(const std::string &ipaddr, unsigned short port, unsigned int timeout_ms = 2000, bool readability_check = false)
    */
   bool PingHost(unsigned long ipaddr, unsigned short port, unsigned int timeout_ms = 2000, bool readability_check = false)
     { return PingHost(GetIpStr(ipaddr), port, timeout_ms, readability_check); }

   virtual bool PingHostImpl(const std::string &target, unsigned int timeout_ms = 2000) = 0;

   // Get/set the nameserver(s)
   virtual std::vector<std::string> GetNameServers(void) = 0;
   virtual void SetNameServers(const std::vector<std::string>& nameServers) = 0;

   // callback from application controlled thread to handle any setup
   void NetworkMessage(EMESSAGE message, int param);

   void StartServices();
   void StopServices(bool bWait);

   /*!
    \brief  Tests if parameter specifies valid IPv6 address (as specified by RFCs)
    \param  ipaddress to convert/test
    \param  *sockaddr non mandatory destination for conversion
    \param  port non mandatory port specification to connect to
    \return Function is dual purpose. With just one parameter (IP),
            it return true if that IP represents IPv6 address.
            If case of second / third supplied argument, it directly
            converts parameters into binary struct pointed by 2nd param.
            (structure needs to be pre-allocated)

    \sa     ConvIP
            For direct conversion into (sockaddr*) see ConvIP() variants.
            (it is universal wrapper to ConvIPv6/v4())
    */
   static bool ConvIPv6(const std::string &address, struct sockaddr_in6 *sa = NULL, unsigned short port = 0);

   /*!
    \brief  Tests if parameter specifies valid IPv4 address (as specified by RFCs)
    \param  ipaddress stored as std::string
    \param  sockaddr* non mandatory
    \param  port non mandatory
    \return Function is dual purpose. With just one parameter (IP),
            it return true if that IP represents IPv4 address.
            If case of second / third supplied argument, it directly
            converts parameters into binary struct pointed by 2nd param.
            (structure needs to be pre-allocated)

    \sa     ConvIP
            For direct conversion into (sockaddr*) see ConvIP() variants.
            (it is universal wrapper to ConvIPv6/v4())
    */
   static bool ConvIPv4(const std::string &address, struct sockaddr_in  *sa = NULL, unsigned short port = 0);

   /*!
    \brief  Converts host IP address into binary sockaddr
            (wrapper to ConvIP(std::string&, ushort))
    */
   static struct sockaddr_storage *ConvIP(unsigned long address, unsigned short port = 0) { return ConvIP(GetIpStr(address), port); }

   /*!
    \brief  Converts host IP address into binary sockaddr
    \param  IP address as std::string, port specification (optional)
    \return Function converts host IP to structure directly used
            in network API functions. It allocates sockaddr_storage
            (via malloc) and returns pointer to it (on error NULL
            is returned). af_family, s_addr, port are filled in.
            Struct sockaddr_storage can be cast to needed structure
            (sockaddr_in, sockaddr_in6, sockaddr, ...)
            You have to relese memory with free(p) after use.
    */
   static struct sockaddr_storage *ConvIP(const std::string &address, unsigned short port = 0);

   static int ParseHex(char *str, unsigned char *addr);

   /*!
    \brief  IPv6/IPv4 compatible conversion of host IP address
    \param  struct sockaddr
    \return Function converts binary structure sockaddr to std::string.
            It can read sockaddr_in and sockaddr_in6, cast as (sockaddr*).
            IPv4 address is returned in the format x.x.x.x (where x is 0-255),
            IPv6 address is returned in it's canonised form.
            On error (or no IPv6/v4 valid input) empty string is returned.
    */
   static std::string GetIpStr(const struct sockaddr *sa);

   /*!
    \brief  Converts IPv4 address stored in unsigned long to std::string
            Function converts from host byte order to network
            byte order first
    \param  IPaddress unsigned long
    \return Result is returned as std::string
    */
   static std::string GetIpStr(unsigned long address);

   /*!
    \brief  Canonisation of IPv6 address
    \param  Any valid IPv6 address representation (std::string)
    \return In respect to RFC 2373, provided string in any legal
            representations will be canonised to it's shortest
            possible form e.g.
            12AB:0000:0000:CD30:0000:0000:0000:0000 -> 12AB:0:0:CD30::
    */
   static std::string CanonizeIPv6(const std::string &address);

  /*!
   \brief  computes the prefix length for a (IPv4/IPv6) netmask
   \param  struct sockaddr
   \return The prefix length of the netmask
           For IPv4 it can be between 0 and 32
           For IPv6 it can be between 0 and 128
   */
  static uint8_t PrefixLength(const struct sockaddr *netmask);

  /*!
   \brief  compares two ip addresses (IPv4/IPv6)
   \param  ip address 1
   \param  ip address 2
   \return if the two addresses are the same
   */
  static bool CompareAddresses(const struct sockaddr *sa, const struct sockaddr *sb);

   /*!
    \brief fully IPv4/IPv6 compatible
           - IPv6 part is limited to addr/mask match only (IPv4 way)

    \param ipaddr1 to match
    \param ipaddr2 to match agains
    \param   mask2 to match agains

    \return TRUE if: ipaddr1 & mask2 == ipaddr2 & mask2 , FALSE otherwise

            TODO: beside addr/match matching IPv6 introduced NetworkDiscoveryProtocol(NDP)
                  currently not implemented.
    */
   static bool AddrMatch(const std::string &addr, const std::string &match_ip, const std::string &match_mask);

   /*!
    \brief Per platform implementation.
           - CNetwork class has both IPv6/IPv4 support, but doesn't require each derived
             class to support IPv6 as well
           - By default we assume IPv6 support not existend (or IPv6 stack unconfigured)
           - CNetwork class functions check IPv6 support automatically based on this call
             and avoid IPv6 depending code automatically.
             This makes the calls safe for the caller without prior checks or deep knowledge
             of CNetwork internals

           - for calls dealing with IPv6 stack - and no IPv6 available - calls return safely
             returning string.empty() / FALSE / -1 or NULL

    \return  Static functions providing only type<>type conversions or formal valididy checking
             are independent on actual IPv6 stack availability
    */
   virtual bool SupportsIPv6(void) { return false; }

   /*!
    \brief Return true if given name or ip address corresponds to localhost
    */
   bool IsLocalHost(const std::string& hostname);

   // Waits for the first network interface to become available
   void WaitForNet();

   /*!
    \brief Registers function as platform. network settings change watcher. Changes on net ifaces
           should be reported by sending message TMSG_NETWORKMESSAGE (CNetwork::NETWORK_CHANGED).
    */
   void RegisterWatcher(void (*watcher)(void *caller)) { m_updThread = new CNetworkUpdater(watcher); m_updThread->Create(false); }
   CNetworkUpdater *m_updThread;

   virtual bool ForceRereadInterfaces() = 0;

protected:
   CCriticalSection m_lockInterfaces;

private:
   CEvent  m_signalNetworkChange;
   bool    m_bStop;
};


#ifdef HAS_LINUX_NETWORK
#include "linux/NetworkLinux.h"
#else
#include "windows/NetworkWin32.h"
#endif

//creates, binds and listens a tcp socket on the desired port. Set bindLocal to
//true to bind to localhost only. The socket will listen over ipv6 if possible
//and fall back to ipv4 if ipv6 is not available on the platform.
int CreateTCPServerSocket(const int port, const bool bindLocal, const int backlog, const char *callerName);

