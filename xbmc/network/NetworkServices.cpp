/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "NetworkServices.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#ifdef TARGET_LINUX
#include "Util.h"
#endif
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/LocalizeStrings.h"
#include "network/Network.h"

#ifdef HAS_AIRPLAY
#include "network/AirPlayServer.h"
#endif // HAS_AIRPLAY

#ifdef HAS_AIRTUNES
#include "network/AirTunesServer.h"
#endif // HAS_AIRTUNES

#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif // HAS_EVENT_SERVER

#ifdef HAS_JSONRPC
#include "interfaces/json-rpc/JSONRPC.h"
#include "network/TCPServer.h"
#endif

#ifdef HAS_ZEROCONF
#include "network/Zeroconf.h"
#endif // HAS_ZEROCONF

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif // HAS_UPNP

#ifdef HAS_WEB_SERVER
#include "network/WebServer.h"
#include "network/httprequesthandler/HTTPImageHandler.h"
#include "network/httprequesthandler/HTTPImageTransformationHandler.h"
#include "network/httprequesthandler/HTTPVfsHandler.h"
#ifdef HAS_JSONRPC
#include "network/httprequesthandler/HTTPJsonRpcHandler.h"
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
#include "network/httprequesthandler/HTTPPythonHandler.h"
#endif
#include "network/httprequesthandler/HTTPWebinterfaceHandler.h"
#include "network/httprequesthandler/HTTPWebinterfaceAddonsHandler.h"
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER

#if defined(TARGET_DARWIN_OSX)
#include "osx/XBMCHelper.h"
#endif

#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/RssManager.h"

using namespace std;
#ifdef HAS_JSONRPC
using namespace JSONRPC;
#endif // HAS_JSONRPC
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif // HAS_EVENT_SERVER
#ifdef HAS_UPNP
using namespace UPNP;
#endif // HAS_UPNP

CNetworkServices::CNetworkServices()
#ifdef HAS_WEB_SERVER
  :
  m_webserver(*new CWebServer),
  m_httpImageHandler(*new CHTTPImageHandler),
  m_httpImageTransformationHandler(*new CHTTPImageTransformationHandler),
  m_httpVfsHandler(*new CHTTPVfsHandler)
#ifdef HAS_JSONRPC
  , m_httpJsonRpcHandler(*new CHTTPJsonRpcHandler)
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  , m_httpPythonHandler(*new CHTTPPythonHandler)
#endif
  , m_httpWebinterfaceHandler(*new CHTTPWebinterfaceHandler)
  , m_httpWebinterfaceAddonsHandler(*new CHTTPWebinterfaceAddonsHandler)
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER
{
#ifdef HAS_WEB_SERVER
  CWebServer::RegisterRequestHandler(&m_httpImageHandler);
  CWebServer::RegisterRequestHandler(&m_httpImageTransformationHandler);
  CWebServer::RegisterRequestHandler(&m_httpVfsHandler);
#ifdef HAS_JSONRPC
  CWebServer::RegisterRequestHandler(&m_httpJsonRpcHandler);
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  CWebServer::RegisterRequestHandler(&m_httpPythonHandler);
#endif
  CWebServer::RegisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  CWebServer::RegisterRequestHandler(&m_httpWebinterfaceHandler);
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER
}

CNetworkServices::~CNetworkServices()
{
#ifdef HAS_WEB_SERVER
  CWebServer::UnregisterRequestHandler(&m_httpImageHandler);
  delete &m_httpImageHandler;
  CWebServer::UnregisterRequestHandler(&m_httpImageTransformationHandler);
  delete &m_httpImageTransformationHandler;
  CWebServer::UnregisterRequestHandler(&m_httpVfsHandler);
  delete &m_httpVfsHandler;
#ifdef HAS_JSONRPC
  CWebServer::UnregisterRequestHandler(&m_httpJsonRpcHandler);
  delete &m_httpJsonRpcHandler;
  CJSONRPC::Cleanup();
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  CWebServer::UnregisterRequestHandler(&m_httpPythonHandler);
  delete &m_httpPythonHandler;
#endif
  CWebServer::UnregisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  delete &m_httpWebinterfaceAddonsHandler;
  CWebServer::UnregisterRequestHandler(&m_httpWebinterfaceHandler);
  delete &m_httpWebinterfaceHandler;
#endif // HAS_WEB_INTERFACE
  delete &m_webserver;
#endif // HAS_WEB_SERVER
}

CNetworkServices& CNetworkServices::Get()
{
  static CNetworkServices sNetworkServices;
  return sNetworkServices;
}

bool CNetworkServices::OnSettingChanging(const CSetting *setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
#ifdef HAS_WEB_SERVER
  if (settingId == "services.webserver" ||
      settingId == "services.webserverport")
  {
    if (IsWebserverRunning() && !StopWebserver())
      return false;

    if (CSettings::Get().GetBool("services.webserver"))
    {
      if (!StartWebserver())
      {
        CGUIDialogOK::ShowAndGetInput(33101, 33100);
        return false;
      }
    }
  }
  else if (settingId == "services.esport" ||
           settingId == "services.webserverport")
    return ValidatePort(((CSettingInt*)setting)->GetValue());
  else
#endif // HAS_WEB_SERVER

#ifdef HAS_ZEROCONF
  if (settingId == "services.zeroconf")
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartZeroconf();
#ifdef HAS_AIRPLAY
    else
    {
      // cannot disable 
      if (IsAirPlayServerRunning() || IsAirTunesServerRunning())
      {
        CGUIDialogOK::ShowAndGetInput(1259, 34303);
        return false;
      }

      return StopZeroconf();
    }
#endif // HAS_AIRPLAY
  }
  else
#endif // HAS_ZEROCONF

#ifdef HAS_AIRPLAY
  if (settingId == "services.airplay")
  {
    if (((CSettingBool*)setting)->GetValue())
    {
#ifdef HAS_ZEROCONF
      // AirPlay needs zeroconf
      if (!CSettings::Get().GetBool("services.zeroconf"))
      {
        CGUIDialogOK::ShowAndGetInput(1273, 34302);
        return false;
      }
#endif //HAS_ZEROCONF

      // note - airtunesserver has to start before airplay server (ios7 client detection bug)
#ifdef HAS_AIRTUNES
      if (!StartAirTunesServer())
      {
        CGUIDialogOK::ShowAndGetInput(1274, 33100);
        return false;
      }
#endif //HAS_AIRTUNES
      
      if (!StartAirPlayServer())
      {
        CGUIDialogOK::ShowAndGetInput(1273, 33100);
        return false;
      }      
    }
    else
    {
      bool ret = true;
#ifdef HAS_AIRTUNES
      if (!StopAirTunesServer(true))
        ret = false;
#endif //HAS_AIRTUNES
      
      if (!StopAirPlayServer(true))
        ret = false;

      if (!ret)
        return false;
    }
  }
  else if (settingId == "services.airplaypassword" ||
           settingId == "services.useairplaypassword")
  {
    if (!CSettings::Get().GetBool("services.airplay"))
      return false;

    if (!CAirPlayServer::SetCredentials(CSettings::Get().GetBool("services.useairplaypassword"),
                                        CSettings::Get().GetString("services.airplaypassword")))
      return false;
  }
  else
#endif //HAS_AIRPLAY

#ifdef HAS_UPNP
  if (settingId == "services.upnpserver")
  {
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartUPnPServer())
        return false;

      // always stop and restart the client and controller if necessary
      StopUPnPClient();
      StopUPnPController();
      StartUPnPClient();
      StartUPnPController();
    }
    else
      return StopUPnPServer();
  }
  else if (settingId == "services.upnprenderer")
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else if (settingId == "services.upnpcontroller")
  {
    // always stop and restart
    StopUPnPController();
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPController();
  }
  else
#endif // HAS_UPNP

  if (settingId == "services.esenabled")
  {
#ifdef HAS_EVENT_SERVER
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(33102, 33100);
        return false;
      }
    }
    else
      return StopEventServer(true, true);
#endif // HAS_EVENT_SERVER

#ifdef HAS_JSONRPC
    if (CSettings::Get().GetBool("services.esenabled"))
    {
      if (!StartJSONRPCServer())
      {
        CGUIDialogOK::ShowAndGetInput(33103, 33100);
        return false;
      }
    }
    else
      return StopJSONRPCServer(false);
#endif // HAS_JSONRPC
  }
  else if (settingId == "services.esport")
  {
#ifdef HAS_EVENT_SERVER
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      CGUIDialogOK::ShowAndGetInput(33102, 33100);
      return false;
    }

#if defined(TARGET_DARWIN_OSX)
    // reconfigure XBMCHelper for port changes
    XBMCHelper::GetInstance().Configure();
#endif // TARGET_DARWIN_OSX
#endif // HAS_EVENT_SERVER
  }
  else if (settingId == "services.esallinterfaces")
  {
#ifdef HAS_EVENT_SERVER
    if (CSettings::Get().GetBool("services.esenabled"))
    {
      if (!StopEventServer(true, true))
        return false;

      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(33102, 33100);
        return false;
      }
    }
#endif // HAS_EVENT_SERVER

#ifdef HAS_JSONRPC
    if (CSettings::Get().GetBool("services.esenabled"))
    {
      if (!StartJSONRPCServer())
      {
        CGUIDialogOK::ShowAndGetInput(33103, 33100);
        return false;
      }
    }
#endif // HAS_JSONRPC
  }

#ifdef HAS_EVENT_SERVER
  else if (settingId == "services.esinitialdelay" ||
           settingId == "services.escontinuousdelay")
  {
    if (CSettings::Get().GetBool("services.esenabled"))
      return RefreshEventServer();
  }
#endif // HAS_EVENT_SERVER

  return true;
}

void CNetworkServices::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
#ifdef HAS_WEB_SERVER
  if (settingId == "services.webserverusername" ||
      settingId == "services.webserverpassword")
  {
    m_webserver.SetCredentials(CSettings::Get().GetString("services.webserverusername"),
                               CSettings::Get().GetString("services.webserverpassword"));
  }
  else
#endif // HAS_WEB_SERVER
  if (settingId == "smb.winsserver" ||
      settingId == "smb.workgroup")
  {
    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
    // TODO - General way of handling setting changes that require restart
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039))
    {
      CSettings::Get().Save();
      CApplicationMessenger::Get().RestartApp();
    }
  }
}

bool CNetworkServices::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == "services.webserverusername")
  {
    // if webserverusername is xbmc and pw is not empty we treat it as altered
    // and don't change the username to kodi - part of rebrand
    if (CSettings::Get().GetString("services.webserverusername") == "xbmc" &&
        !CSettings::Get().GetString("services.webserverpassword").empty())
      return true;
  }
  return false;
}

void CNetworkServices::Start()
{
  StartZeroconf();
#ifdef HAS_WEB_SERVER
  if (CSettings::Get().GetBool("services.webserver") && !StartWebserver())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
#endif // HAS_WEB_SERVER
  StartUPnP();
  if (CSettings::Get().GetBool("services.esenabled") && !StartEventServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
  if (CSettings::Get().GetBool("services.esenabled") && !StartJSONRPCServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33103), g_localizeStrings.Get(33100));
  
  // note - airtunesserver has to start before airplay server (ios7 client detection bug)
  StartAirTunesServer();
  StartAirPlayServer();
  StartRss();
}

void CNetworkServices::Stop(bool bWait)
{
  if (bWait)
  {
    StopUPnP(bWait);
    StopZeroconf();
    StopWebserver();
    StopRss();
  }

  StopEventServer(bWait, false);
  StopJSONRPCServer(bWait);
  StopAirPlayServer(bWait);
  StopAirTunesServer(bWait);
}

bool CNetworkServices::StartWebserver()
{
#ifdef HAS_WEB_SERVER
  if (!g_application.getNetwork().IsAvailable())
    return false;

  if (!CSettings::Get().GetBool("services.webserver"))
    return false;

  int webPort = CSettings::Get().GetInt("services.webserverport");
  if (!ValidatePort(webPort))
  {
    CLog::Log(LOGERROR, "Cannot start Web Server on port %i", webPort);
    return false;
  }

  if (IsWebserverRunning())
    return true;

  CLog::Log(LOGNOTICE, "Webserver: Starting...");
  if (!m_webserver.Start(webPort, CSettings::Get().GetString("services.webserverusername"), CSettings::Get().GetString("services.webserverpassword")))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  // publish web frontend and API services
#ifdef HAS_WEB_INTERFACE
  CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), webPort, txt);
#endif // HAS_WEB_INTERFACE
#ifdef HAS_JSONRPC
  CZeroconf::GetInstance()->PublishService("servers.jsonrpc-http", "_xbmc-jsonrpc-h._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), webPort, txt);
#endif // HAS_JSONRPC
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::IsWebserverRunning()
{
#ifdef HAS_WEB_SERVER
  return m_webserver.IsStarted();
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::StopWebserver()
{
#ifdef HAS_WEB_SERVER
  if (!IsWebserverRunning())
    return true;

  CLog::Log(LOGNOTICE, "Webserver: Stopping...");
  if (!m_webserver.Stop() || m_webserver.IsStarted())
  {
    CLog::Log(LOGWARNING, "Webserver: Failed to stop.");
    return false;
  }
  
  CLog::Log(LOGNOTICE, "Webserver: Stopped...");
#ifdef HAS_ZEROCONF
#ifdef HAS_WEB_INTERFACE
  CZeroconf::GetInstance()->RemoveService("servers.webserver");
#endif // HAS_WEB_INTERFACE
#ifdef HAS_JSONRPC
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-http");
#endif // HAS_JSONRPC
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::StartAirPlayServer()
{
#ifdef HAS_AIRPLAY
  if (!g_application.getNetwork().IsAvailable() || !CSettings::Get().GetBool("services.airplay"))
    return false;

  if (IsAirPlayServerRunning())
    return true;
  
  if (!CAirPlayServer::StartServer(g_advancedSettings.m_airPlayPort, true))
    return false;
  
  if (!CAirPlayServer::SetCredentials(CSettings::Get().GetBool("services.useairplaypassword"),
                                      CSettings::Get().GetString("services.airplaypassword")))
    return false;
  
#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  txt.push_back(make_pair("deviceid", iface != NULL ? iface->GetMacAddress() : "FF:FF:FF:FF:FF:F2"));
  txt.push_back(make_pair("model", "Xbmc,1"));
  txt.push_back(make_pair("srcvers", AIRPLAY_SERVER_VERSION_STR));

  if (CSettings::Get().GetBool("services.airplayios8compat"))
  {
    // for ios8 clients we need to announce mirroring support
    // else we won't get video urls anymore.
    // We also announce photo caching support (as it seems faster and
    // we have implemented it anyways). 
    txt.push_back(make_pair("features", "0x20F7"));
  }
  else
  {
    txt.push_back(make_pair("features", "0x77"));
  }

  CZeroconf::GetInstance()->PublishService("servers.airplay", "_airplay._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), g_advancedSettings.m_airPlayPort, txt);
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::IsAirPlayServerRunning()
{
#ifdef HAS_AIRPLAY
  return CAirPlayServer::IsRunning();
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::StopAirPlayServer(bool bWait)
{
#ifdef HAS_AIRPLAY
  if (!IsAirPlayServerRunning())
    return true;

  CAirPlayServer::StopServer(bWait);

#ifdef HAS_ZEROCONF
  CZeroconf::GetInstance()->RemoveService("servers.airplay");
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_AIRPLAY
  return false;
}

bool CNetworkServices::StartAirTunesServer()
{
#ifdef HAS_AIRTUNES
  if (!g_application.getNetwork().IsAvailable() || !CSettings::Get().GetBool("services.airplay"))
    return false;

  if (IsAirTunesServerRunning())
    return true;

  if (!CAirTunesServer::StartServer(g_advancedSettings.m_airTunesPort, true,
                                    CSettings::Get().GetBool("services.useairplaypassword"),
                                    CSettings::Get().GetString("services.airplaypassword")))
  {
    CLog::Log(LOGERROR, "Failed to start AirTunes Server");
    return false;
  }

  return true;
#endif // HAS_AIRTUNES
  return false;
}

bool CNetworkServices::IsAirTunesServerRunning()
{
#ifdef HAS_AIRTUNES
  return CAirTunesServer::IsRunning();
#endif // HAS_AIRTUNES
  return false;
}

bool CNetworkServices::StopAirTunesServer(bool bWait)
{
#ifdef HAS_AIRTUNES
  if (!IsAirTunesServerRunning())
    return true;

  CAirTunesServer::StopServer(bWait);
  return true;
#endif // HAS_AIRTUNES
  return false;
}

bool CNetworkServices::StartJSONRPCServer()
{
#ifdef HAS_JSONRPC
  if (!CSettings::Get().GetBool("services.esenabled"))
    return false;

  if (IsJSONRPCServerRunning())
    return true;

  if (!CTCPServer::StartServer(g_advancedSettings.m_jsonTcpPort, CSettings::Get().GetBool("services.esallinterfaces")))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CZeroconf::GetInstance()->PublishService("servers.jsonrpc-tpc", "_xbmc-jsonrpc._tcp", g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME), g_advancedSettings.m_jsonTcpPort, txt);
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_JSONRPC
  return false;
}

bool CNetworkServices::IsJSONRPCServerRunning()
{
#ifdef HAS_JSONRPC
  return CTCPServer::IsRunning();
#endif // HAS_JSONRPC
  return false;
}

bool CNetworkServices::StopJSONRPCServer(bool bWait)
{
#ifdef HAS_JSONRPC
  if (!IsJSONRPCServerRunning())
    return true;

  CTCPServer::StopServer(bWait);

#ifdef HAS_ZEROCONF
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-tcp");
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_JSONRPC
  return false;
}

bool CNetworkServices::StartEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (!CSettings::Get().GetBool("services.esenabled"))
    return false;

  if (IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

  CLog::Log(LOGNOTICE, "ES: Starting event server");
  server->StartServer();

  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::IsEventServerRunning()
{
#ifdef HAS_EVENT_SERVER
  return CEventServer::GetInstance()->Running();
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::StopEventServer(bool bWait, bool promptuser)
{
#ifdef HAS_EVENT_SERVER
  if (!IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

  if (promptuser)
  {
    if (server->GetNumberOfClients() > 0)
    {
      bool cancelled = false;
      if (!CGUIDialogYesNo::ShowAndGetInput(13140, 13141, cancelled, "", "", 10000)
          || cancelled)
      {
        CLog::Log(LOGNOTICE, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGNOTICE, "ES: Stopping event server with confirmation");

    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
      CLog::Log(LOGNOTICE, "ES: Stopping event server");

    CEventServer::GetInstance()->StopServer(bWait);
  }

  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (!CSettings::Get().GetBool("services.esenabled"))
    return false;

  if (!IsEventServerRunning())
    return false;

  CEventServer::GetInstance()->RefreshSettings();
  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::StartUPnP()
{
  bool ret = false;
#ifdef HAS_UPNP
  ret |= StartUPnPClient();
  ret |= StartUPnPServer();
  ret |= StartUPnPController();
  ret |= StartUPnPRenderer();
#endif // HAS_UPNP
  return ret;
}

bool CNetworkServices::StopUPnP(bool bWait)
{
#ifdef HAS_UPNP
  if (!CUPnP::IsInstantiated())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp");
  CUPnP::ReleaseInstance(bWait);

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPClient()
{
#ifdef HAS_UPNP
  CLog::Log(LOGNOTICE, "starting upnp client");
  CUPnP::GetInstance()->StartClient();
  return IsUPnPClientRunning();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPClientRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsClientStarted();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPClient()
{
#ifdef HAS_UPNP
  if (!IsUPnPClientRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp client");
  CUPnP::GetInstance()->StopClient();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPController()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnpcontroller") ||
      !CSettings::Get().GetBool("services.upnpserver"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp controller");
  CUPnP::GetInstance()->StartController();
  return IsUPnPControllerRunning();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPControllerRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsControllerStarted();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPController()
{
#ifdef HAS_UPNP
  if (!IsUPnPControllerRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp controller");
  CUPnP::GetInstance()->StopController();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnprenderer"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp renderer");
  return CUPnP::GetInstance()->StartRenderer();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPRendererRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsInstantiated();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!IsUPnPRendererRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp renderer");
  CUPnP::GetInstance()->StopRenderer();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnpserver"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp server");
  return CUPnP::GetInstance()->StartServer();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPServerRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsInstantiated();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPServer()
{
#ifdef HAS_UPNP
  if (!IsUPnPServerRunning())
    return true;

  StopUPnPController();

  CLog::Log(LOGNOTICE, "stopping upnp server");
  CUPnP::GetInstance()->StopServer();

  return true;
#endif // HAS_UPNP
  return false;
}
  
bool CNetworkServices::StartRss()
{
  if (IsRssRunning())
    return true;

  CRssManager::Get().Start();
  return true;
}

bool CNetworkServices::IsRssRunning()
{
  return CRssManager::Get().IsActive();
}

bool CNetworkServices::StopRss()
{
  if (!IsRssRunning())
    return true;

  CRssManager::Get().Stop();
  return true;
}

bool CNetworkServices::StartZeroconf()
{
#ifdef HAS_ZEROCONF
  if (!CSettings::Get().GetBool("services.zeroconf"))
    return false;

  if (IsZeroconfRunning())
    return true;

  CLog::Log(LOGNOTICE, "starting zeroconf publishing");
  return CZeroconf::GetInstance()->Start();
#endif // HAS_ZEROCONF
  return false;
}

bool CNetworkServices::IsZeroconfRunning()
{
#ifdef HAS_ZEROCONF
  return CZeroconf::GetInstance()->IsStarted();
#endif // HAS_ZEROCONF
  return false;
}

bool CNetworkServices::StopZeroconf()
{
#ifdef HAS_ZEROCONF
  if (!IsZeroconfRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping zeroconf publishing");
  CZeroconf::GetInstance()->Stop();

  return true;
#endif // HAS_ZEROCONF
  return false;
}

bool CNetworkServices::ValidatePort(int port)
{
  if (port <= 0 || port > 65535)
    return false;

#ifdef TARGET_LINUX
  if (!CUtil::CanBindPrivileged() && (port < 1024 || port > 65535))
    return false;
#endif

  return true;
}
