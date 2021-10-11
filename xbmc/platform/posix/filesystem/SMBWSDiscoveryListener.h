/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <string>
#include <vector>

#include <netinet/in.h>

namespace WSDiscovery
{
struct wsd_req_info;
}

namespace WSDiscovery
{
class CWSDiscoveryListenerUDP : public CThread
{
public:
  CWSDiscoveryListenerUDP();
  ~CWSDiscoveryListenerUDP();

  void Start();
  void Stop();

protected:
  // CThread
  void Process() override;

private:
  struct Command
  {
    struct sockaddr_in address;
    std::string commandMsg;
  };

  /*
   * Dispatch UDP command from command Queue
   * return    (bool) true if message dispatched
   */
  bool DispatchCommand();

  /*
   * Build SOAPXML command and add to command Queue
   * in        (string) action type
   * in        (string) extra data field (currently used solely for resolve addresses)
   * return    (void)
   */
  void AddCommand(const std::string& message, const std::string& extraparameter = "");

  /*
   * Process received broadcast messages and add accepted items to 
   * in        (string) SOAPXML Message
   * return    (void)
   */
  void ParseBuffer(const std::string& buffer);

  /*
   * Generates an XML SOAP message given a particular action type
   * in        (string) action type
   * in/out    (string) created message
   * in        (string) extra data field (currently used for resolve addresses)
   * return    (bool) true if full message crafted
   */
  bool buildSoapMessage(const std::string& action,
                        std::string& msg,
                        const std::string& extraparameter);

  // Closes socket and handles setting state for WS-Discovery
  void Cleanup(bool aborted);

  /*
   * Use unicast Get to request computer name
   * in/out    (wsd_req_info&) host info to be updated
   * return    (void)
   */
  void UnicastGet(wsd_req_info& info);

private:
  template<std::size_t SIZE>
  const std::string wsd_tag_find(const std::string& xml,
                                 const std::array<std::pair<std::string, std::string>, SIZE>& tag);

  // Debug print WSD packet data
  void PrintWSDInfo(const WSDiscovery::wsd_req_info& info);

  // compare WSD entry address
  const bool equalsAddress(const WSDiscovery::wsd_req_info& lhs,
                           const WSDiscovery::wsd_req_info& rhs);

  // Socket FD for send/recv
  int fd;

  std::vector<Command> m_commandbuffer;

  CCriticalSection crit_commandqueue;
  CCriticalSection crit_wsdqueue;

  std::vector<WSDiscovery::wsd_req_info> m_vecWSDInfo;

  // GUID that should remain constant for an instance
  const std::string wsd_instance_address;

  // Number of sends for UDP messages
  const int retries = 4;

  // Max udp packet size (+ UDP header + IP header overhead = 65535)
  const int UDPBUFFSIZE = 65507;

  // Port for unicast/multicast WSD traffic
  const int wsdUDP = 3702;

  // ipv4 multicast group WSD - https://specs.xmlsoap.org/ws/2005/04/discovery/ws-discovery.pdf
  const char* WDSIPv4MultiGroup = "239.255.255.250";

  // ToDo: ipv6 broadcast address
  // const char* WDSIPv6MultiGroup = "FF02::C"
};
} // namespace WSDiscovery
