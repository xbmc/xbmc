/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SMBWSDiscoveryListener.h"

#include "ServiceBroker.h"
#include "filesystem/CurlFile.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/posix/filesystem/SMBWSDiscovery.h"

#include <array>
#include <chrono>
#include <mutex>
#include <stdio.h>
#include <string>
#include <utility>

#include <arpa/inet.h>
#include <fmt/format.h>
#include <sys/select.h>
#include <unistd.h>

using namespace WSDiscovery;

namespace WSDiscovery
{

// Templates for SOAPXML messages for WS-Discovery messages
static const std::string soap_msg_templ =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<soap:Envelope "
    "xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
    "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
    "xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
    "xmlns:wsx=\"http://schemas.xmlsoap.org/ws/2004/09/mex\" "
    "xmlns:wsdp=\"http://schemas.xmlsoap.org/ws/2006/02/devprof\" "
    "xmlns:un0=\"http://schemas.microsoft.com/windows/pnpx/2005/10\" "
    "xmlns:pub=\"http://schemas.microsoft.com/windows/pub/2005/07\">\n"
    "<soap:Header>\n"
    "<wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>\n"
    "<wsa:Action>{}</wsa:Action>\n"
    "<wsa:MessageID>urn:uuid:{}</wsa:MessageID>\n"
    "<wsd:AppSequence InstanceId=\"{}\" MessageNumber=\"{}\" />\n"
    "{}"
    "</soap:Header>\n"
    "{}"
    "</soap:Envelope>\n";

static const std::string hello_body = "<soap:Body>\n"
                                      "<wsd:Hello>\n"
                                      "<wsa:EndpointReference>\n"
                                      "<wsa:Address>urn:uuid:{}</wsa:Address>\n"
                                      "</wsa:EndpointReference>\n"
                                      "<wsd:Types>wsdp:Device pub:Computer</wsd:Types>\n"
                                      "<wsd:MetadataVersion>1</wsd:MetadataVersion>\n"
                                      "</wsd:Hello>\n"
                                      "</soap:Body>\n";

static const std::string bye_body = "<soap:Body>\n"
                                    "<wsd:Bye>\n"
                                    "<wsa:EndpointReference>\n"
                                    "<wsa:Address>urn:uuid:{}</wsa:Address>\n"
                                    "</wsa:EndpointReference>\n"
                                    "<wsd:Types>wsdp:Device pub:Computer</wsd:Types>\n"
                                    "<wsd:MetadataVersion>2</wsd:MetadataVersion>\n"
                                    "</wsd:Bye>\n"
                                    "</soap:Body>\n";

static const std::string probe_body = "<soap:Body>\n"
                                      "<wsd:Probe>\n"
                                      "<wsd:Types>wsdp:Device</wsd:Types>\n"
                                      "</wsd:Probe>\n"
                                      "</soap:Body>\n";

static const std::string resolve_body = "<soap:Body>\n"
                                        "<wsd:Resolve>\n"
                                        "<wsa:EndpointReference>\n"
                                        "<wsa:Address>"
                                        "{}"
                                        "</wsa:Address>\n"
                                        "</wsa:EndpointReference>\n"
                                        "</wsd:Resolve>\n"
                                        "</soap:Body>\n";

// These are the only actions we concern ourselves with for our WS-D implementation
static const std::string WSD_ACT_HELLO = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello";
static const std::string WSD_ACT_BYE = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye";
static const std::string WSD_ACT_PROBEMATCH =
    "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
static const std::string WSD_ACT_PROBE = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
static const std::string WSD_ACT_RESOLVE =
    "http://schemas.xmlsoap.org/ws/2005/04/discovery/Resolve";
static const std::string WSD_ACT_RESOLVEMATCHES =
    "http://schemas.xmlsoap.org/ws/2005/04/discovery/ResolveMatches";


// These are xml start/finish tags we need info from
static const std::array<std::pair<std::string, std::string>, 2> action_tag{
    {{"<wsa:Action>", "</wsa:Action>"},
     {"<wsa:Action SOAP-ENV:mustUnderstand=\"true\">", "</wsa:Action>"}}};

static const std::array<std::pair<std::string, std::string>, 2> msgid_tag{
    {{"<wsa:MessageID>", "</wsa:MessageID>"},
     {"<wsa:MessageID SOAP-ENV:mustUnderstand=\"true\">", "</wsa:MessageID>"}}};

static const std::array<std::pair<std::string, std::string>, 1> xaddrs_tag{
    {{"<wsd:XAddrs>", "</wsd:XAddrs>"}}};

static const std::array<std::pair<std::string, std::string>, 1> address_tag{
    {{"<wsa:Address>", "</wsa:Address>"}}};

static const std::array<std::pair<std::string, std::string>, 1> types_tag{
    {{"<wsd:Types>", "</wsd:Types>"}}};

static const std::string get_msg =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<soap:Envelope "
    "xmlns:pnpx=\"http://schemas.microsoft.com/windows/pnpx/2005/10\" "
    "xmlns:pub=\"http://schemas.microsoft.com/windows/pub/2005/07\" "
    "xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
    "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
    "xmlns:wsd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
    "xmlns:wsdp=\"http://schemas.xmlsoap.org/ws/2006/02/devprof\" "
    "xmlns:wsx=\"http://schemas.xmlsoap.org/ws/2004/09/mex\"> "
    "<soap:Header> "
    "<wsa:To>{}</wsa:To> "
    "<wsa:Action>http://schemas.xmlsoap.org/ws/2004/09/transfer/Get</wsa:Action> "
    "<wsa:MessageID>urn:uuid:{}</wsa:MessageID> "
    "<wsa:ReplyTo> "
    "<wsa:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</wsa:Address> "
    "</wsa:ReplyTo> "
    "<wsa:From> "
    "<wsa:Address>urn:uuid:{}</wsa:Address> "
    "</wsa:From> "
    "</soap:Header> "
    "<soap:Body /> "
    "</soap:Envelope>";

static const std::array<std::pair<std::string, std::string>, 1> computer_tag{
    {{"<pub:Computer>", "</pub:Computer>"}}};

CWSDiscoveryListenerUDP::CWSDiscoveryListenerUDP()
  : CThread("WSDiscoveryListenerUDP"), wsd_instance_address(StringUtils::CreateUUID())
{
}

CWSDiscoveryListenerUDP::~CWSDiscoveryListenerUDP()
{
}

void CWSDiscoveryListenerUDP::Stop()
{
  StopThread(true);
  CLog::Log(LOGINFO, "CWSDiscoveryListenerUDP::Stop - Stopped");
}

void CWSDiscoveryListenerUDP::Start()
{
  if (IsRunning())
    return;

  // Clear existing data. We dont know how long service has been down, so we may not
  // have correct services that may have sent BYE actions
  m_vecWSDInfo.clear();
  CWSDiscoveryPosix& WSInstance =
      dynamic_cast<CWSDiscoveryPosix&>(CServiceBroker::GetWSDiscovery());
  WSInstance.SetItems(m_vecWSDInfo);

  // Create thread and set low priority
  Create();
  SetPriority(ThreadPriority::LOWEST);
  CLog::Log(LOGINFO, "CWSDiscoveryListenerUDP::Start - Started");
}

void CWSDiscoveryListenerUDP::Process()
{
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::Process - Socket Creation failed");
    return;
  }

  // set socket reuse
  int enable = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable)) < 0)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY, "CWSDiscoveryListenerUDP::Process - Reuse Option failed");
    Cleanup(true);
    return;
  }

  // set up destination address
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(wsdUDP);

  // bind to receive address
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY, "CWSDiscoveryListenerUDP::Process - Bind failed");
    Cleanup(true);
    return;
  }

  // use setsockopt() to request join a multicast group on all interfaces
  // maybe use firstconnected?
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(WDSIPv4MultiGroup);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::Process - Multicast Option failed");
    Cleanup(true);
    return;
  }

  // Disable receiving broadcast messages on loopback
  // So we dont receive messages we send.
  int disable = 0;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&disable, sizeof(disable)) < 0)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::Process - Loopback Disable Option failed");
    Cleanup(true);
    return;
  }

  std::string bufferoutput;
  fd_set rset;
  int nready;
  struct timeval timeout;

  FD_ZERO(&rset);

  // Send HELLO to the world
  AddCommand(WSD_ACT_HELLO);
  DispatchCommand();

  AddCommand(WSD_ACT_PROBE);
  DispatchCommand();

  while (!m_bStop)
  {
    FD_SET(fd, &rset);
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    nready = select((fd + 1), &rset, NULL, NULL, &timeout);

    if (nready < 0)
      break;

    if (m_bStop)
      break;

    // if udp socket is readable receive the message.
    if (FD_ISSET(fd, &rset))
    {
      bufferoutput = "";
      char msgbuf[UDPBUFFSIZE];
      socklen_t addrlen = sizeof(addr);
      int nbytes = recvfrom(fd, msgbuf, UDPBUFFSIZE, 0, (struct sockaddr*)&addr, &addrlen);
      msgbuf[nbytes] = '\0';
      // turn msgbuf into std::string
      bufferoutput.append(msgbuf, nbytes);

      ParseBuffer(bufferoutput);
    }
    // Action any commands queued
    while (DispatchCommand())
    {
    }
  }

  // Be a nice citizen and send BYE to the world
  AddCommand(WSD_ACT_BYE);
  DispatchCommand();
  Cleanup(false);
  return;
}

void CWSDiscoveryListenerUDP::Cleanup(bool aborted)
{
  close(fd);

  if (aborted)
  {
    // Thread is stopping due to a failure state
    // Update service setting to be disabled
    auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return;

    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (!settings)
      return;

    if (settings->GetBool(CSettings::SETTING_SERVICES_WSDISCOVERY))
    {
      settings->SetBool(CSettings::SETTING_SERVICES_WSDISCOVERY, false);
      settings->Save();
    }
  }
}

bool CWSDiscoveryListenerUDP::DispatchCommand()
{
  Command sendCommand;
  {
    std::unique_lock<CCriticalSection> lock(crit_commandqueue);
    if (m_commandbuffer.size() <= 0)
      return false;

    auto it = m_commandbuffer.begin();
    sendCommand = *it;
    m_commandbuffer.erase(it);
  }

  int ret;

  // As its udp, send multiple messages due to unreliability
  // Windows seems to send 4-6 times for reference
  for (int i = 0; i < retries; i++)
  {
    do
    {
      ret = sendto(fd, sendCommand.commandMsg.c_str(), sendCommand.commandMsg.size(), 0,
                   (struct sockaddr*)&sendCommand.address, sizeof(sendCommand.address));
    } while (ret == -1 && !m_bStop);
    std::chrono::seconds sec(1);
    CThread::Sleep(sec);
  }

  CLog::Log(LOGDEBUG, LOGWSDISCOVERY, "CWSDiscoveryListenerUDP::DispatchCommand - Command sent");

  return true;
}

void CWSDiscoveryListenerUDP::AddCommand(const std::string& message,
                                         const std::string& extraparameter /* = "" */)
{

  std::unique_lock<CCriticalSection> lock(crit_commandqueue);

  std::string msg{};

  if (!buildSoapMessage(message, msg, extraparameter))
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::AddCommand - Invalid Soap Message");
    return;
  }

  // ToDo: IPv6
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(wsdUDP);

  addr.sin_addr.s_addr = inet_addr(WDSIPv4MultiGroup);
  memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

  Command newCommand{{addr}, {msg}};

  m_commandbuffer.push_back(newCommand);
}

void CWSDiscoveryListenerUDP::ParseBuffer(const std::string& buffer)
{
  wsd_req_info info;

  // MUST have an action tag
  std::string action = wsd_tag_find(buffer, action_tag);
  if (action.empty())
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::ParseBuffer - No Action tag found");
    return;
  }

  info.action = action;

  // Only handle actions we need when received
  if (!((action == WSD_ACT_HELLO) || (action == WSD_ACT_BYE) ||
        (action == WSD_ACT_RESOLVEMATCHES) || (action == WSD_ACT_PROBEMATCH)))
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::ParseBuffer - Action not supported");
    PrintWSDInfo(info);
    return;
  }

  // MUST have a msgid tag
  std::string msgid = wsd_tag_find(buffer, msgid_tag);
  if (msgid.empty())
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::ParseBuffer - No msgid tag found");
    return;
  }

  info.msgid = msgid;

  std::string types = wsd_tag_find(buffer, types_tag);
  info.types = types;
  std::string address = wsd_tag_find(buffer, address_tag);
  info.address = address;
  std::string xaddrs = wsd_tag_find(buffer, xaddrs_tag);
  info.xaddrs = xaddrs;

  if (xaddrs.empty() && (action != WSD_ACT_BYE))
  {
    // Do a resolve against address to hopefully receive an xaddrs field=
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::ParseBuffer - Resolve Action dispatched");
    AddCommand(WSD_ACT_RESOLVE, address);
    // Discard this message
    PrintWSDInfo(info);
    return;
  }

  const std::string delim1 = "://";
  const std::string delim2 = ":";
  size_t found = info.xaddrs.find(delim1);
  if (found != std::string::npos)
  {
    std::string tmpxaddrs = info.xaddrs.substr(found + delim1.size());
    if (tmpxaddrs[0] != '[')
    {
      found = std::min(tmpxaddrs.find(delim2), tmpxaddrs.find('/'));
      info.xaddrs_host = tmpxaddrs.substr(0, found);
    }
  }

  {
    std::unique_lock<CCriticalSection> lock(crit_wsdqueue);
    auto searchitem = std::find_if(m_vecWSDInfo.begin(), m_vecWSDInfo.end(),
                                   [info](const wsd_req_info& item) { return item == info; });

    if (searchitem == m_vecWSDInfo.end())
    {
      CWSDiscoveryPosix& WSInstance =
          dynamic_cast<CWSDiscoveryPosix&>(CServiceBroker::GetWSDiscovery());
      if (info.action != WSD_ACT_BYE)
      {
        // Acceptable request received, add to our server list
        CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
                  "CWSDiscoveryListenerUDP::ParseBuffer - Actionable message");
        UnicastGet(info);
        m_vecWSDInfo.emplace_back(info);
        WSInstance.SetItems(m_vecWSDInfo);
        PrintWSDInfo(info);
        return;
      }
      else
      {
        // WSD_ACT_BYE does not include an xaddrs tag
        // We only want to match the address when receiving a WSD_ACT_BYE message to
        // remove from our maintained list
        auto searchbye = std::find_if(
            m_vecWSDInfo.begin(), m_vecWSDInfo.end(),
            [info, this](const wsd_req_info& item) { return equalsAddress(item, info); });
        if (searchbye != m_vecWSDInfo.end())
        {
          CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
                    "CWSDiscoveryListenerUDP::ParseBuffer - BYE action -"
                    " removing entry");
          m_vecWSDInfo.erase(searchbye);
          PrintWSDInfo(info);
          WSInstance.SetItems(m_vecWSDInfo);
          return;
        }
      }
    }
  }

  // Only duplicate items get this far, silently drop
  return;
}

bool CWSDiscoveryListenerUDP::buildSoapMessage(const std::string& action,
                                               std::string& msg,
                                               const std::string& extraparameter)
{
  auto msg_uuid = StringUtils::CreateUUID();
  std::string body;
  std::string relatesTo; // Not implemented, may not be needed for our limited usage
  int messagenumber = 1;

  CWSDiscoveryPosix& WSInstance =
      dynamic_cast<CWSDiscoveryPosix&>(CServiceBroker::GetWSDiscovery());
  long long wsd_instance_id = WSInstance.GetInstanceID();

  if (action == WSD_ACT_HELLO)
  {
    body = fmt::format(hello_body, wsd_instance_address);
  }
  else if (action == WSD_ACT_BYE)
  {
    body = fmt::format(bye_body, wsd_instance_address);
  }
  else if (action == WSD_ACT_PROBE)
  {
    body = probe_body;
  }
  else if (action == WSD_ACT_RESOLVE)
  {
    body = fmt::format(resolve_body, extraparameter);
  }
  if (body.empty())
  {
    // May lead to excessive logspam
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::buildSoapMessage unimplemented WSD_ACTION");
    return false;
  }

  // Todo: how are fmt exception/failures handled with libfmt?
  msg = fmt::format(soap_msg_templ, action, msg_uuid, wsd_instance_id, messagenumber, relatesTo,
                    body);

  return true;
}

template<std::size_t SIZE>
const std::string CWSDiscoveryListenerUDP::wsd_tag_find(
    const std::string& xml, const std::array<std::pair<std::string, std::string>, SIZE>& tag)
{
  for (const auto& tagpair : tag)
  {
    std::size_t found1 = xml.find(tagpair.first);
    if (found1 != std::string::npos)
    {
      std::size_t found2 = xml.find(tagpair.second);
      if (found2 != std::string::npos)
      {
        return xml.substr((found1 + tagpair.first.size()),
                          (found2 - (found1 + tagpair.first.size())));
      }
    }
  }
  return "";
}

bool CWSDiscoveryListenerUDP::equalsAddress(const wsd_req_info& lhs, const wsd_req_info& rhs) const
{
  return lhs.address == rhs.address;
}

void CWSDiscoveryListenerUDP::PrintWSDInfo(const wsd_req_info& info)
{
  CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
            "CWSDiscoveryUtils::printstruct - message contents\n"
            "\tAction: {}\n"
            "\tMsgID: {}\n"
            "\tAddress: {}\n"
            "\tTypes: {}\n"
            "\tXAddrs: {}\n"
            "\tComputer: {}\n",
            info.action, info.msgid, info.address, info.types, info.xaddrs, info.computer);
}

void CWSDiscoveryListenerUDP::UnicastGet(wsd_req_info& info)
{
  if (INADDR_NONE == inet_addr(info.xaddrs_host.c_str()))
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY, "CWSDiscoveryListenerUDP::UnicastGet - No IP address in {}",
              info.xaddrs);
    return;
  }

  std::string msg =
      fmt::format(get_msg, info.address, StringUtils::CreateUUID(), wsd_instance_address);
  XFILE::CCurlFile file;
  file.SetAcceptEncoding("identity");
  file.SetMimeType("application/soap+xml");
  file.SetUserAgent("wsd");
  file.SetRequestHeader("Connection", "Close");

  std::string html;
  if (!file.Post(info.xaddrs, msg, html))
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::UnicastGet - Could not fetch from {}", info.xaddrs);
    return;
  }
  info.computer = wsd_tag_find(html, computer_tag);
  if (info.computer.empty())
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CWSDiscoveryListenerUDP::UnicastGet - No computer tag found");
    return;
  }
}

} // namespace WSDiscovery
