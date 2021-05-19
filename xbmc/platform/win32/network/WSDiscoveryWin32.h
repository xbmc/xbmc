/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <wsdapi.h>
#pragma comment(lib, "wsdapi.lib")

#include <vector>


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

  bool ThereAreServers() { return m_serversIPs.size() > 0; }
  std::vector<std::wstring> GetServersIPs() { return m_serversIPs; }

private:
  std::vector<std::wstring> m_serversIPs;
  ULONG m_cRef;
  CCriticalSection m_criticalSection;
};

class CWSDiscoverySupport
{
public:
  CWSDiscoverySupport();
  ~CWSDiscoverySupport();

  static std::shared_ptr<CWSDiscoverySupport> Get();

  bool Initialize();
  void Terminate();

  bool IsInitialized() { return m_initialized; }
  bool ThereAreServers();
  std::vector<std::wstring> GetServersIPs();

  static std::wstring ResolveHostName(const std::wstring& serverIP);

private:
  bool m_initialized = false;
  IWSDiscoveryProvider* m_provider = nullptr;
  CClientNotificationSink* m_sink = nullptr;
};
