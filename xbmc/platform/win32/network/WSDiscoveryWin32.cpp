/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WSDiscoveryWin32.h"

#include "URL.h"
#include "filesystem/CurlFile.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <algorithm>
#include <mutex>
#include <string_view>

#include <windns.h>
#pragma comment(lib, "dnsapi.lib")

#include <ws2tcpip.h>

using KODI::PLATFORM::WINDOWS::FromW;
using KODI::PLATFORM::WINDOWS::ToW;
using namespace WSDiscovery;

namespace
{
// Same WS-Transfer Get request as the POSIX implementation (SMBWSDiscoveryListener.cpp)
constexpr std::string_view WSD_GET_MSG =
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

// The name the server announces itself with, e.g. "MEDIAMASTER" from
// <pub:Computer>MEDIAMASTER/Workgroup:WORKGROUP</pub:Computer>
std::wstring QueryWSDComputerName(const WSDServer& server)
{
  if (server.endpoint.empty() || !server.xaddr.starts_with(L"http://"))
    return {};

  const std::string msg = StringUtils::Format(WSD_GET_MSG, FromW(server.endpoint),
                                              StringUtils::CreateUUID(), StringUtils::CreateUUID());
  XFILE::CCurlFile file;
  file.SetTimeout(2);
  file.SetTransferTimeout(5);
  file.SetAcceptEncoding("identity");
  file.SetMimeType("application/soap+xml");
  file.SetUserAgent("wsd");
  file.SetRequestHeader("Connection", "Close");

  std::string response;
  if (!file.Post(FromW(server.xaddr) + "|redirect-limit=0", msg, response))
    return {};

  constexpr std::string_view computerTag = "<pub:Computer>";
  const size_t start = response.find(computerTag);
  if (start == std::string::npos)
    return {};

  const size_t nameStart = start + computerTag.size();
  const size_t nameEnd = response.find_first_of("/<", nameStart);
  if (nameEnd == std::string::npos)
    return {};

  std::string name = response.substr(nameStart, nameEnd - nameStart);
  const std::wstring hostName = ToW(StringUtils::Trim(name));

  // SMB paths are opened by name, so the name must lead back to this server
  ADDRINFOW hints{};
  hints.ai_family = AF_INET;
  ADDRINFOW* addresses = nullptr;
  bool matches = false;
  if (GetAddrInfoW(hostName.c_str(), nullptr, &hints, &addresses) == 0)
  {
    // the local machine announces itself on loopback, which its name never resolves to
    matches = server.ip == L"127.0.0.1";
    for (const ADDRINFOW* address = addresses; address && !matches; address = address->ai_next)
    {
      wchar_t ip[INET_ADDRSTRLEN]{};
      matches =
          InetNtopW(AF_INET, &reinterpret_cast<const sockaddr_in*>(address->ai_addr)->sin_addr, ip,
                    INET_ADDRSTRLEN) &&
          server.ip == ip;
    }
    FreeAddrInfoW(addresses);
  }
  if (!matches)
  {
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "[WS-Discovery]: Announced name '{}' doesn't resolve to '{}'", FromW(hostName),
              FromW(server.ip));
    return {};
  }

  return hostName;
}

std::wstring QueryReverseDNS(const std::wstring& serverIP)
{
  const std::vector<std::string> ip = StringUtils::Split(FromW(serverIP), '.');
  if (ip.size() != 4)
    return {};

  const std::string reverse =
      StringUtils::Format("{}.{}.{}.{}.IN-ADDR.ARPA", ip[3], ip[2], ip[1], ip[0]);

  // No multicast: when unicast DNS fails, mDNS would answer with names such as
  // "host-4.local" that Avahi generates after a name conflict and that change on restart.
  PDNS_RECORD pDnsRecord = nullptr;
  std::wstring hostName;

  if (DnsQuery_W(ToW(reverse).c_str(), DNS_TYPE_PTR, DNS_QUERY_STANDARD | DNS_QUERY_NO_MULTICAST,
                 nullptr, &pDnsRecord, nullptr) == ERROR_SUCCESS)
  {
    for (PDNS_RECORD record = pDnsRecord; record; record = record->pNext)
    {
      if (record->wType == DNS_TYPE_PTR && record->Data.PTR.pNameHost)
      {
        hostName = record->Data.PTR.pNameHost;
        break;
      }
    }
  }
  else
  {
    CLog::LogF(LOGWARNING, "DnsQuery_W for '{}' failed", reverse);
  }

  DnsRecordListFree(pDnsRecord, DnsFreeRecordList);

  return hostName;
}
} // namespace

namespace WSDiscovery
{

HRESULT CClientNotificationSink::Create(CClientNotificationSink** sink)
{
  CClientNotificationSink* tempSink = nullptr;

  if (!sink)
    return E_POINTER;

  tempSink = new CClientNotificationSink();

  if (!tempSink)
    return E_OUTOFMEMORY;

  *sink = tempSink;
  tempSink = nullptr;

  return S_OK;
}

CClientNotificationSink::CClientNotificationSink() : m_cRef(1)
{
}

CClientNotificationSink::~CClientNotificationSink()
{
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::Add(IWSDiscoveredService* service)
{
  if (!service)
    return E_INVALIDARG;

  std::unique_lock lock(m_criticalSection);

  WSD_NAME_LIST* list = nullptr;
  service->GetTypes(&list);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (list && address)
  {
    const std::wstring addr(address);
    std::wstring type(L"Unspecified");
    WSD_NAME_LIST* pList = list; // first element of list

    do
    {
      if (pList->Element && pList->Element->LocalName)
        type = std::wstring(pList->Element->LocalName);
      if (pList->Next)
        pList = pList->Next; // next element of list
      else
        pList = nullptr; // end of list
    } while (type != L"Computer" && pList != nullptr);

    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "[WS-Discovery]: HELLO packet received: device type = '{}', device address = '{}'",
              FromW(type), FromW(addr));

    // filter Printers and other devices that are not "Computers"
    if (type == L"Computer")
    {
      WSDServer server;
      server.ip = addr.substr(0, addr.find(L":", 0));

      WSD_ENDPOINT_REFERENCE* endpoint = nullptr;
      if (SUCCEEDED(service->GetEndpointReference(&endpoint)) && endpoint && endpoint->Address)
        server.endpoint = endpoint->Address;

      // only ask the announcing server itself for its name
      WSD_URI_LIST* xaddrs = nullptr;
      if (SUCCEEDED(service->GetXAddrs(&xaddrs)))
      {
        for (const WSD_URI_LIST* xaddr = xaddrs; xaddr; xaddr = xaddr->Next)
        {
          if (!xaddr->Element)
            continue;

          const std::wstring uri(xaddr->Element);
          const CURL url(FromW(uri));
          if (url.IsProtocol("http") && url.GetHostName() == FromW(server.ip))
            server.xaddr = uri;
        }
      }

      // a multi-homed server announces the same endpoint from each of its addresses
      auto it = std::ranges::find_if(m_servers,
                                     [&server](const WSDServer& known) {
                                       return server.endpoint.empty()
                                                  ? known.ip == server.ip
                                                  : known.endpoint == server.endpoint;
                                     });

      if (it == m_servers.end())
      {
        CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
                  "[WS-Discovery]: IP '{}' ({}) has been inserted into the server list.",
                  FromW(server.ip), FromW(server.endpoint));
        m_servers.push_back(std::move(server));
      }
      else if (it->ip != server.ip || it->xaddr != server.xaddr)
      {
        CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
                  "[WS-Discovery]: IP '{}' ({}) has replaced '{}' in the server list.",
                  FromW(server.ip), FromW(server.endpoint), FromW(it->ip));
        *it = std::move(server);
      }
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::Remove(IWSDiscoveredService* service)
{
  if (!service)
    return E_INVALIDARG;

  std::unique_lock lock(m_criticalSection);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (address)
  {
    const std::wstring addr(address);

    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "[WS-Discovery]: BYE packet received: device address = '{}'", FromW(addr));

    const std::wstring ip = addr.substr(0, addr.find(L":", 0));

    std::wstring endpointAddress;
    WSD_ENDPOINT_REFERENCE* endpoint = nullptr;
    if (SUCCEEDED(service->GetEndpointReference(&endpoint)) && endpoint && endpoint->Address)
      endpointAddress = endpoint->Address;

    auto it = std::ranges::find_if(
        m_servers, [&](const WSDServer& known)
        { return endpointAddress.empty() ? known.ip == ip : known.endpoint == endpointAddress; });

    // removes server from list
    if (it != m_servers.end())
    {
      m_servers.erase(it);
      CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
                "[WS-Discovery]: IP '{}' has been removed from the server list.", FromW(ip));
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchFailed(HRESULT hr, LPCWSTR tag)
{
  std::unique_lock lock(m_criticalSection);

  // This must not happen. At least localhost (127.0.0.1) has to be found
  CLog::Log(LOGWARNING,
            "[WS-Discovery]: The initial search for servers has failed. No servers found.");

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchComplete(LPCWSTR tag)
{
  std::unique_lock lock(m_criticalSection);

  std::string list;

  for (const auto& server : m_servers)
    list.append('\n' + FromW(server.ip));

  CLog::Log(LOGDEBUG,
            "[WS-Discovery]: The initial servers search has completed successfully with {} "
            "server(s) found:{}",
            m_servers.size(), list);

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::QueryInterface(REFIID riid, void** object)
{
  if (!object)
    return E_POINTER;

  *object = nullptr;

  if (__uuidof(IWSDiscoveryProviderNotify) == riid)
    *object = static_cast<IWSDiscoveryProviderNotify*>(this);
  else if (__uuidof(IUnknown) == riid)
    *object = static_cast<IUnknown*>(this);
  else
    return E_NOINTERFACE;

  ((LPUNKNOWN)*object)->AddRef();

  return S_OK;
}

bool CClientNotificationSink::ThereAreServers()
{
  std::unique_lock lock(m_criticalSection);

  return !m_servers.empty();
}

std::vector<WSDServer> CClientNotificationSink::GetServers()
{
  std::unique_lock lock(m_criticalSection);

  return m_servers;
}

ULONG STDMETHODCALLTYPE CClientNotificationSink::AddRef()
{
  ULONG newRefCount = InterlockedIncrement(&m_cRef);

  return newRefCount;
}

ULONG STDMETHODCALLTYPE CClientNotificationSink::Release()
{
  ULONG newRefCount = InterlockedDecrement(&m_cRef);

  if (!newRefCount)
    delete this;

  return newRefCount;
}

//==================================================================================

std::unique_ptr<IWSDiscovery> IWSDiscovery::GetInstance()
{
  return std::make_unique<WSDiscovery::CWSDiscoveryWindows>();
}

CWSDiscoveryWindows::~CWSDiscoveryWindows()
{
  StopServices();
}

bool CWSDiscoveryWindows::StartServices()
{
  if (m_initialized)
    return true;

  if (S_OK == WSDCreateDiscoveryProvider(nullptr, &m_provider))
  {
    m_provider->SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4);

    if (S_OK == CClientNotificationSink::Create(&m_sink))
    {
      if (S_OK == m_provider->Attach(m_sink))
      {
        if (S_OK == m_provider->SearchByType(nullptr, nullptr, nullptr, nullptr))
        {
          m_initialized = true;
          CLog::Log(LOGINFO, "[WS-Discovery]: Daemon started successfully.");
          return true;
        }
      }
    }
  }

  // if get here something has gone wrong
  CLog::Log(LOGERROR, "[WS-Discovery]: Daemon initialization has failed.");

  StopServices();

  return false;
}

bool CWSDiscoveryWindows::StopServices()
{
  if (m_initialized)
  {
    CLog::Log(LOGINFO, "[WS-Discovery]: terminating");
    m_initialized = false;
  }
  if (m_provider)
  {
    m_provider->Detach();
    m_provider->Release();
    m_provider = nullptr;
  }
  if (m_sink)
  {
    m_sink->Release();
    m_sink = nullptr;
  }
  return true;
}

bool CWSDiscoveryWindows::IsRunning()
{
  return m_initialized;
}

bool CWSDiscoveryWindows::ThereAreServers()
{
  if (!m_sink)
    return false;

  return m_sink->ThereAreServers();
}

std::vector<WSDServer> CWSDiscoveryWindows::GetServers()
{
  if (!m_sink)
    return {};

  return m_sink->GetServers();
}

std::wstring CWSDiscoveryWindows::ResolveHostName(const WSDServer& server)
{
  std::wstring hostName = QueryWSDComputerName(server);

  if (hostName.empty())
    hostName = QueryReverseDNS(server.ip);

  return hostName.empty() ? server.ip : hostName;
}
} // namespace WSDiscovery
