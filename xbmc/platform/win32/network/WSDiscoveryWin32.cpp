/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WSDiscoveryWin32.h"

#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <windns.h>
#pragma comment(lib, "dnsapi.lib")

using KODI::PLATFORM::WINDOWS::FromW;


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

  CSingleLock lock(m_criticalSection);

  WSD_NAME_LIST* list = nullptr;
  service->GetTypes(&list);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (list && address)
  {
    std::wstring type(list->Next->Element->LocalName);
    std::wstring addr(address);

    CLog::Log(LOGINFO,
              "[WS-Discovery]: HELLO packet received: device type = '{}', device address = '{}'",
              FromW(type), FromW(addr));

    // filter Printers and other devices that are not "Computers"
    if (type == L"Computer")
    {
      std::wstring addr(address);
      const std::wstring ip = addr.substr(0, addr.find(L":", 0));

      auto it = std::find(m_serversIPs.begin(), m_serversIPs.end(), ip);

      // inserts server IP if not exist in list
      if (it == m_serversIPs.end())
      {
        m_serversIPs.push_back(ip);
        CLog::Log(LOGINFO, "[WS-Discovery]: IP '{}' has been inserted into the server list.",
                  FromW(ip));
      }
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::Remove(IWSDiscoveredService* service)
{
  if (!service)
    return E_INVALIDARG;

  CSingleLock lock(m_criticalSection);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (address)
  {
    std::wstring addr(address);

    CLog::Log(LOGINFO, "[WS-Discovery]: BYE packet received: device address = '{}'", FromW(addr));

    const std::wstring ip = addr.substr(0, addr.find(L":", 0));
    auto it = std::find(m_serversIPs.begin(), m_serversIPs.end(), ip);

    // removes server IP from list
    if (it != m_serversIPs.end())
    {
      m_serversIPs.erase(it);
      CLog::Log(LOGINFO, "[WS-Discovery]: IP '{}' has been removed from the server list.",
                FromW(ip));
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchFailed(HRESULT hr, LPCWSTR tag)
{
  CSingleLock lock(m_criticalSection);

  CLog::Log(LOGINFO,
            "[WS-Discovery]: The initial search for servers has failed. No servers found.");

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchComplete(LPCWSTR tag)
{
  CSingleLock lock(m_criticalSection);

  CLog::Log(LOGINFO,
            "[WS-Discovery]: The initial servers search has completed successfully with {} "
            "server(s) found:",
            m_serversIPs.size());

  for (const auto& ip : GetServersIPs())
    CLog::Log(LOGINFO, "    {}", FromW(ip));

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

std::shared_ptr<CWSDiscoverySupport> CWSDiscoverySupport::Get()
{
  static std::shared_ptr<CWSDiscoverySupport> sWSD(new CWSDiscoverySupport);
  return sWSD;
}

CWSDiscoverySupport::CWSDiscoverySupport()
{
  Initialize();
}

CWSDiscoverySupport::~CWSDiscoverySupport()
{
  Terminate();
}

bool CWSDiscoverySupport::Initialize()
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
          CLog::Log(LOGINFO, "[WS-Discovery]: Daemon initialized successfully. Now are listening "
                             "HELLO and BYE packets from LAN.");
          return true;
        }
      }
    }
  }

  // if get here something has gone wrong
  CLog::Log(LOGERROR, "[WS-Discovery]: Daemon initialization has failed.");

  Terminate();

  return false;
}

void CWSDiscoverySupport::Terminate()
{
  if (m_initialized)
  {
    CLog::Log(LOGINFO, "[WS-Discovery]: terminate...");
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
}

std::wstring CWSDiscoverySupport::ResolveHostName(std::wstring server_ip)
{
  std::wstring hostname = server_ip;
  std::vector<std::string> ip = StringUtils::Split(FromW(server_ip), '.', 4);
  std::string reverse = StringUtils::Format("{}.{}.{}.{}.IN-ADDR.ARPA", ip[3], ip[2], ip[1], ip[0]);

  PDNS_RECORD pDnsRecord = nullptr;

  if (!DnsQuery_W(KODI::PLATFORM::WINDOWS::ToW(reverse).c_str(), DNS_TYPE_PTR, DNS_QUERY_STANDARD,
                  nullptr, &pDnsRecord, nullptr) &&
      pDnsRecord)
  {
    hostname = pDnsRecord->Data.PTR.pNameHost;
  }

  DnsRecordListFree(pDnsRecord, freetype);

  return hostname;
}
