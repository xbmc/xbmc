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

#include <utility>

#include "Application.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/RssManager.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"

#ifdef TARGET_LINUX
#include "Util.h"
#endif
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
#include "platform/darwin/osx/XBMCHelper.h"
#endif

using namespace KODI::MESSAGING;
#ifdef HAS_JSONRPC
using namespace JSONRPC;
#endif // HAS_JSONRPC
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif // HAS_EVENT_SERVER
#ifdef HAS_UPNP
using namespace UPNP;
#endif // HAS_UPNP

using KODI::MESSAGING::HELPERS::DialogResponse;

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
  m_webserver.RegisterRequestHandler(&m_httpImageHandler);
  m_webserver.RegisterRequestHandler(&m_httpImageTransformationHandler);
  m_webserver.RegisterRequestHandler(&m_httpVfsHandler);
#ifdef HAS_JSONRPC
  m_webserver.RegisterRequestHandler(&m_httpJsonRpcHandler);
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  m_webserver.RegisterRequestHandler(&m_httpPythonHandler);
#endif
  m_webserver.RegisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  m_webserver.RegisterRequestHandler(&m_httpWebinterfaceHandler);
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER
}

CNetworkServices::~CNetworkServices()
{
#ifdef HAS_WEB_SERVER
  m_webserver.UnregisterRequestHandler(&m_httpImageHandler);
  delete &m_httpImageHandler;
  m_webserver.UnregisterRequestHandler(&m_httpImageTransformationHandler);
  delete &m_httpImageTransformationHandler;
  m_webserver.UnregisterRequestHandler(&m_httpVfsHandler);
  delete &m_httpVfsHandler;
#ifdef HAS_JSONRPC
  m_webserver.UnregisterRequestHandler(&m_httpJsonRpcHandler);
  delete &m_httpJsonRpcHandler;
  CJSONRPC::Cleanup();
#endif // HAS_JSONRPC
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  m_webserver.UnregisterRequestHandler(&m_httpPythonHandler);
  delete &m_httpPythonHandler;
#endif
  m_webserver.UnregisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  delete &m_httpWebinterfaceAddonsHandler;
  m_webserver.UnregisterRequestHandler(&m_httpWebinterfaceHandler);
  delete &m_httpWebinterfaceHandler;
#endif // HAS_WEB_INTERFACE
  delete &m_webserver;
#endif // HAS_WEB_SERVER
}

CNetworkServices& CNetworkServices::GetInstance()
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
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVER ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT)
  {
    if (IsWebserverRunning() && !StopWebserver())
      return false;

    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
    {
      if (!StartWebserver())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{33101}, CVariant{33100});
        return false;
      }
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESPORT ||
           settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT)
    return ValidatePort(((CSettingInt*)setting)->GetValue());
  else
#endif // HAS_WEB_SERVER

#ifdef HAS_ZEROCONF
  if (settingId == CSettings::SETTING_SERVICES_ZEROCONF)
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartZeroconf();
#ifdef HAS_AIRPLAY
    else
    {
      // cannot disable 
      if (IsAirPlayServerRunning() || IsAirTunesServerRunning())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{1259}, CVariant{34303});
        return false;
      }

      return StopZeroconf();
    }
#endif // HAS_AIRPLAY
  }
  else
#endif // HAS_ZEROCONF

#ifdef HAS_AIRPLAY
  if (settingId == CSettings::SETTING_SERVICES_AIRPLAY)
  {
    if (((CSettingBool*)setting)->GetValue())
    {
#ifdef HAS_ZEROCONF
      // AirPlay needs zeroconf
      if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ZEROCONF))
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{1273}, CVariant{34302});
        return false;
      }
#endif //HAS_ZEROCONF

      // note - airtunesserver has to start before airplay server (ios7 client detection bug)
#ifdef HAS_AIRTUNES
      if (!StartAirTunesServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{1274}, CVariant{33100});
        return false;
      }
#endif //HAS_AIRTUNES
      
      if (!StartAirPlayServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{1273}, CVariant{33100});
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
  else if (settingId == CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT)
  {
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartAirPlayServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{1273}, CVariant{33100});
        return false;
      }
    }
    else
    {
      if (!StopAirPlayServer(true))
        return false;
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_AIRPLAYPASSWORD ||
           settingId == CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD)
  {
    if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
      return false;

    if (!CAirPlayServer::SetCredentials(CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                        CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
      return false;
  }
  else
#endif //HAS_AIRPLAY

#ifdef HAS_UPNP
  if (settingId == CSettings::SETTING_SERVICES_UPNPSERVER)
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
  else if (settingId == CSettings::SETTING_SERVICES_UPNPRENDERER)
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_UPNPCONTROLLER)
  {
    // always stop and restart
    StopUPnPController();
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPController();
  }
  else
#endif // HAS_UPNP

  if (settingId == CSettings::SETTING_SERVICES_ESENABLED)
  {
    if (((CSettingBool*)setting)->GetValue())
    {
      bool result = true;
#ifdef HAS_EVENT_SERVER
      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{33102}, CVariant{33100});
        result = false;
      }
#endif // HAS_EVENT_SERVER

#ifdef HAS_JSONRPC
      if (!StartJSONRPCServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{33103}, CVariant{33100});
        result = false;
      }
#endif // HAS_JSONRPC
      return result;
    }
    else
    {
      bool result = true;
#ifdef HAS_EVENT_SERVER
      result = StopEventServer(true, true);
#endif // HAS_EVENT_SERVER
#ifdef HAS_JSONRPC
      result &= StopJSONRPCServer(false);
#endif // HAS_JSONRPC
      return result;
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESPORT)
  {
#ifdef HAS_EVENT_SERVER
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{33102}, CVariant{33100});
      return false;
    }

#if defined(TARGET_DARWIN_OSX)
    // reconfigure XBMCHelper for port changes
    XBMCHelper::GetInstance().Configure();
#endif // TARGET_DARWIN_OSX
#endif // HAS_EVENT_SERVER
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESALLINTERFACES)
  {
#ifdef HAS_EVENT_SERVER
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    {
      if (!StopEventServer(true, true))
        return false;

      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{33102}, CVariant{33100});
        return false;
      }
    }
#endif // HAS_EVENT_SERVER

#ifdef HAS_JSONRPC
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    {
      if (!StopJSONRPCServer(true))
        return false;

      if (!StartJSONRPCServer())
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{33103}, CVariant{33100});
        return false;
      }
    }
#endif // HAS_JSONRPC
  }

#ifdef HAS_EVENT_SERVER
  else if (settingId == CSettings::SETTING_SERVICES_ESINITIALDELAY ||
           settingId == CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY)
  {
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
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
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERUSERNAME ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERPASSWORD)
  {
    m_webserver.SetCredentials(CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME),
                               CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD));
  }
  else
#endif // HAS_WEB_SERVER
  if (settingId == CSettings::SETTING_SMB_WINSSERVER ||
      settingId == CSettings::SETTING_SMB_WORKGROUP)
  {
    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
    //! @todo - General way of handling setting changes that require restart
    if (HELPERS::ShowYesNoDialogText(CVariant{14038}, CVariant{14039}) == DialogResponse::YES)
    {
      CSettings::GetInstance().Save();
      CApplicationMessenger::GetInstance().PostMsg(TMSG_RESTARTAPP);
    }
  }
}

bool CNetworkServices::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERUSERNAME)
  {
    // if webserverusername is xbmc and pw is not empty we treat it as altered
    // and don't change the username to kodi - part of rebrand
    if (CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME) == "xbmc" &&
        !CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())
      return true;
  }
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT)
  {
    // if webserverport is default but webserver is activated then treat it as altered
    // and don't change the port to new value
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
      return true;
  }
  return false;
}

void CNetworkServices::Start()
{
  StartZeroconf();
#ifdef HAS_WEB_SERVER
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_WEBSERVER) && !StartWebserver())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
#endif // HAS_WEB_SERVER
  StartUPnP();
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED) && !StartEventServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED) && !StartJSONRPCServer())
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

  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
    return false;

  int webPort = CSettings::GetInstance().GetInt(CSettings::SETTING_SERVICES_WEBSERVERPORT);
  if (!ValidatePort(webPort))
  {
    CLog::Log(LOGERROR, "Cannot start Web Server on port %i", webPort);
    return false;
  }

  if (IsWebserverRunning())
    return true;

  if (!m_webserver.Start(webPort, CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME), CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD)))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  // publish web frontend and API services
#ifdef HAS_WEB_INTERFACE
  CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp", CSysInfo::GetDeviceName(), webPort, txt);
#endif // HAS_WEB_INTERFACE
#ifdef HAS_JSONRPC
  CZeroconf::GetInstance()->PublishService("servers.jsonrpc-http", "_xbmc-jsonrpc-h._tcp", CSysInfo::GetDeviceName(), webPort, txt);
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

  if (!m_webserver.Stop() || m_webserver.IsStarted())
  {
    CLog::Log(LOGWARNING, "Webserver: Failed to stop.");
    return false;
  }
  
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT))
    return true;

#ifdef HAS_AIRPLAY
  if (!g_application.getNetwork().IsAvailable() || !CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
    return false;

  if (IsAirPlayServerRunning())
    return true;
  
  if (!CAirPlayServer::StartServer(g_advancedSettings.m_airPlayPort, true))
    return false;
  
  if (!CAirPlayServer::SetCredentials(CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                      CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
    return false;
  
#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  txt.push_back(std::make_pair("deviceid", iface != NULL ? iface->GetMacAddress() : "FF:FF:FF:FF:FF:F2"));
  txt.push_back(std::make_pair("model", "Xbmc,1"));
  txt.push_back(std::make_pair("srcvers", AIRPLAY_SERVER_VERSION_STR));

  // for ios8 clients we need to announce mirroring support
  // else we won't get video urls anymore.
  // We also announce photo caching support (as it seems faster and
  // we have implemented it anyways).
  txt.push_back(std::make_pair("features", "0x20F7"));

  CZeroconf::GetInstance()->PublishService("servers.airplay", "_airplay._tcp", CSysInfo::GetDeviceName(), g_advancedSettings.m_airPlayPort, txt);
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
  if (!g_application.getNetwork().IsAvailable() || !CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
    return false;

  if (IsAirTunesServerRunning())
    return true;

  if (!CAirTunesServer::StartServer(g_advancedSettings.m_airTunesPort, true,
                                    CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                    CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (IsJSONRPCServerRunning())
    return true;

  if (!CTCPServer::StartServer(g_advancedSettings.m_jsonTcpPort, CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESALLINTERFACES)))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CZeroconf::GetInstance()->PublishService("servers.jsonrpc-tpc", "_xbmc-jsonrpc._tcp", CSysInfo::GetDeviceName(), g_advancedSettings.m_jsonTcpPort, txt);
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

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
      if (HELPERS::ShowYesNoDialogText(CVariant{13140}, CVariant{13141}, CVariant{""}, CVariant{""}, 10000) != 
        DialogResponse::YES)
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ESENABLED))
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
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPSERVER))
  {
   ret |= StartUPnPServer();
  }

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPCONTROLLER))
  {
    ret |= StartUPnPController();
  }

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER))
  {
    ret |= StartUPnPRenderer();
  }
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPCONTROLLER) ||
      !CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPSERVER))
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER))
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
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_UPNPSERVER))
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

  CRssManager::GetInstance().Start();
  return true;
}

bool CNetworkServices::IsRssRunning()
{
  return CRssManager::GetInstance().IsActive();
}

bool CNetworkServices::StopRss()
{
  if (!IsRssRunning())
    return true;

  CRssManager::GetInstance().Stop();
  return true;
}

bool CNetworkServices::StartZeroconf()
{
#ifdef HAS_ZEROCONF
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_SERVICES_ZEROCONF))
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
