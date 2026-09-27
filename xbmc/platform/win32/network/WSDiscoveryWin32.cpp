/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WSDiscoveryWin32.h"

#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <algorithm>
#include <mutex>

#include <windns.h>
#pragma comment(lib, "dnsapi.lib")

using KODI::PLATFORM::WINDOWS::FromW;
using KODI::PLATFORM::WINDOWS::ToW;
using namespace WSDiscovery;

namespace
{
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

std::wstring CWSDiscoveryWindows::ResolveHostName(const std::wstring& serverIP)
{
  const std::wstring hostName = QueryReverseDNS(serverIP);

  return hostName.empty() ? serverIP : hostName;
}
} // namespace WSDiscovery
