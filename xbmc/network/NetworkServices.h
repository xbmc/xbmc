/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"

class CSettings;
#ifdef HAS_WEB_SERVER
class CWebServer;
class CHTTPImageHandler;
class CHTTPImageTransformationHandler;
class CHTTPVfsHandler;
class CHTTPJsonRpcHandler;
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
class CHTTPPythonHandler;
#endif
class CHTTPWebinterfaceHandler;
class CHTTPWebinterfaceAddonsHandler;
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER

class CNetworkServices : public ISettingCallback
{
public:
  CNetworkServices();
  ~CNetworkServices() override;

  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;

  void Start();
  void Stop(bool bWait);

  enum ESERVERS
  {
    ES_WEBSERVER = 1,
    ES_AIRPLAYSERVER,
    ES_JSONRPCSERVER,
    ES_UPNPRENDERER,
    ES_UPNPSERVER,
    ES_EVENTSERVER,
    ES_ZEROCONF,
    ES_WSDISCOVERY,
  };

  bool StartServer(enum ESERVERS server, bool start);

  bool StartWebserver();
  bool IsWebserverRunning();
  bool StopWebserver();

  bool StartAirPlayServer();
  bool IsAirPlayServerRunning();
  bool StopAirPlayServer(bool bWait);
  bool StartAirTunesServer();
  bool IsAirTunesServerRunning();
  bool StopAirTunesServer(bool bWait);

  bool StartJSONRPCServer();
  bool IsJSONRPCServerRunning();
  bool StopJSONRPCServer(bool bWait);

  bool StartEventServer();
  bool IsEventServerRunning();
  bool StopEventServer(bool bWait, bool promptuser);
  bool RefreshEventServer();

  bool StartUPnP();
  bool StopUPnP(bool bWait);
  bool StartUPnPClient();
  bool IsUPnPClientRunning();
  bool StopUPnPClient();
  bool StartUPnPController();
  bool IsUPnPControllerRunning();
  bool StopUPnPController();
  bool StartUPnPRenderer();
  bool IsUPnPRendererRunning();
  bool StopUPnPRenderer();
  bool StartUPnPServer();
  bool IsUPnPServerRunning();
  bool StopUPnPServer();

  bool StartRss();
  bool IsRssRunning();
  bool StopRss();

  bool StartZeroconf();
  bool IsZeroconfRunning();
  bool StopZeroconf();

  bool StartWSDiscovery();
  bool IsWSDiscoveryRunning();
  bool StopWSDiscovery();

private:
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);

  bool ValidatePort(int port);

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  // Network services
#ifdef HAS_WEB_SERVER
  CWebServer& m_webserver;
  // Handlers
  CHTTPImageHandler& m_httpImageHandler;
  CHTTPImageTransformationHandler& m_httpImageTransformationHandler;
  CHTTPVfsHandler& m_httpVfsHandler;
  CHTTPJsonRpcHandler& m_httpJsonRpcHandler;
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  CHTTPPythonHandler& m_httpPythonHandler;
#endif
  CHTTPWebinterfaceHandler& m_httpWebinterfaceHandler;
  CHTTPWebinterfaceAddonsHandler& m_httpWebinterfaceAddonsHandler;
#endif
#endif
};
