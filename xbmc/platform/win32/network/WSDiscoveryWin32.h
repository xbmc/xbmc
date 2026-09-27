/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/IWSDiscovery.h"
#include "threads/CriticalSection.h"

#include <wsdapi.h>
#pragma comment(lib, "wsdapi.lib")

#include <string>
#include <vector>

namespace WSDiscovery
{
struct WSDServer
{
  std::wstring endpoint; // wsa:Address, e.g. urn:uuid:...
  std::wstring ip;
  std::wstring xaddr; // metadata URL, e.g. http://192.168.1.25:5357/<uuid>
};

class CClientNotificationSink : public IWSDiscoveryProviderNotify
{
public:
  CClientNotificationSink();
  ~CClientNotificationSink();

  static HRESULT Create(CClientNotificationSink** sink);

  HRESULT STDMETHODCALLTYPE Add(IWSDiscoveredService* service);
  HRESULT STDMETHODCALLTYPE Remove(IWSDiscoveredService* service);
  HRESULT STDMETHODCALLTYPE SearchFailed(HRESULT hr, LPCWSTR tag);
  HRESULT STDMETHODCALLTYPE SearchComplete(LPCWSTR tag);
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object);
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();

  bool ThereAreServers();
  std::vector<WSDServer> GetServers();

private:
  std::vector<WSDServer> m_servers;
  ULONG m_cRef;
  CCriticalSection m_criticalSection;
};

class CWSDiscoveryWindows : public WSDiscovery::IWSDiscovery
{
public:
  CWSDiscoveryWindows() = default;
  ~CWSDiscoveryWindows() override;

  bool StartServices() override;
  bool StopServices() override;
  bool IsRunning() override;

  bool ThereAreServers();
  std::vector<WSDServer> GetServers();

  static std::wstring ResolveHostName(const std::wstring& serverIP);

private:
  bool m_initialized = false;
  IWSDiscoveryProvider* m_provider = nullptr;
  CClientNotificationSink* m_sink = nullptr;
};
} // namespace WSDiscovery
