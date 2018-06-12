/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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
  CNetworkServices(CSettings &settings);
  ~CNetworkServices() override;

  bool OnSettingChanging(std::shared_ptr<const CSetting> setting) override;
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  bool OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode) override;

  void Start();
  void Stop(bool bWait);

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

private:
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);

  bool ValidatePort(int port);

  // Construction parameters
  CSettings &m_settings;

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
