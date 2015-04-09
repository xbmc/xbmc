#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "system.h"
#include "settings/lib/ISettingCallback.h"

#ifdef HAS_WEB_SERVER
class CWebServer;
class CHTTPImageHandler;
class CHTTPImageTransformationHandler;
class CHTTPVfsHandler;
#ifdef HAS_JSONRPC
class CHTTPJsonRpcHandler;
#endif // HAS_JSONRPC
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
  static CNetworkServices& Get();
  
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

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
  CNetworkServices();
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);
  virtual ~CNetworkServices();

  bool ValidatePort(int port);

#ifdef HAS_WEB_SERVER
  CWebServer& m_webserver;
  CHTTPImageHandler& m_httpImageHandler;
  CHTTPImageTransformationHandler& m_httpImageTransformationHandler;
  CHTTPVfsHandler& m_httpVfsHandler;
#ifdef HAS_JSONRPC
  CHTTPJsonRpcHandler& m_httpJsonRpcHandler;
#endif
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  CHTTPPythonHandler& m_httpPythonHandler;
#endif
  CHTTPWebinterfaceHandler& m_httpWebinterfaceHandler;
  CHTTPWebinterfaceAddonsHandler& m_httpWebinterfaceAddonsHandler;
#endif
#endif
};
