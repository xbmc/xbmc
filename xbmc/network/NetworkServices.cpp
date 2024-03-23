/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkServices.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "network/EventServer.h"
#include "network/Network.h"
#include "network/TCPServer.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/RssManager.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <utility>

#ifdef TARGET_LINUX
#include "Util.h"
#endif
#ifdef HAS_AIRPLAY
#include "network/AirPlayServer.h"
#endif // HAS_AIRPLAY

#ifdef HAS_AIRTUNES
#include "network/AirTunesServer.h"
#endif // HAS_AIRTUNES

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
#include "network/httprequesthandler/HTTPJsonRpcHandler.h"
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
#include "network/httprequesthandler/HTTPPythonHandler.h"
#endif
#include "network/httprequesthandler/HTTPWebinterfaceHandler.h"
#include "network/httprequesthandler/HTTPWebinterfaceAddonsHandler.h"
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER

#if defined(HAS_FILESYSTEM_SMB)
#if defined(TARGET_WINDOWS)
#include "platform/win32/network/WSDiscoveryWin32.h"
#else // defined(TARGET_POSIX)
#include "platform/posix/filesystem/SMBWSDiscovery.h"
#endif
#endif

#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
#include "platform/darwin/osx/XBMCHelper.h"
#endif

using namespace KODI::MESSAGING;
using namespace JSONRPC;
using namespace EVENTSERVER;
#ifdef HAS_UPNP
using namespace UPNP;
#endif // HAS_UPNP

using KODI::MESSAGING::HELPERS::DialogResponse;

CNetworkServices::CNetworkServices()
#ifdef HAS_WEB_SERVER
  : m_webserver(*new CWebServer),
  m_httpImageHandler(*new CHTTPImageHandler),
  m_httpImageTransformationHandler(*new CHTTPImageTransformationHandler),
  m_httpVfsHandler(*new CHTTPVfsHandler),
  m_httpJsonRpcHandler(*new CHTTPJsonRpcHandler)
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
  m_webserver.RegisterRequestHandler(&m_httpJsonRpcHandler);
#ifdef HAS_WEB_INTERFACE
#ifdef HAS_PYTHON
  m_webserver.RegisterRequestHandler(&m_httpPythonHandler);
#endif
  m_webserver.RegisterRequestHandler(&m_httpWebinterfaceAddonsHandler);
  m_webserver.RegisterRequestHandler(&m_httpWebinterfaceHandler);
#endif // HAS_WEB_INTERFACE
#endif // HAS_WEB_SERVER
  std::set<std::string> settingSet{
      CSettings::SETTING_SERVICES_WEBSERVER,
      CSettings::SETTING_SERVICES_WEBSERVERPORT,
      CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION,
      CSettings::SETTING_SERVICES_WEBSERVERUSERNAME,
      CSettings::SETTING_SERVICES_WEBSERVERPASSWORD,
      CSettings::SETTING_SERVICES_WEBSERVERSSL,
      CSettings::SETTING_SERVICES_ZEROCONF,
      CSettings::SETTING_SERVICES_AIRPLAY,
      CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL,
      CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT,
      CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD,
      CSettings::SETTING_SERVICES_AIRPLAYPASSWORD,
      CSettings::SETTING_SERVICES_UPNP,
      CSettings::SETTING_SERVICES_UPNPSERVER,
      CSettings::SETTING_SERVICES_UPNPRENDERER,
      CSettings::SETTING_SERVICES_UPNPCONTROLLER,
      CSettings::SETTING_SERVICES_ESENABLED,
      CSettings::SETTING_SERVICES_ESPORT,
      CSettings::SETTING_SERVICES_ESALLINTERFACES,
      CSettings::SETTING_SERVICES_ESINITIALDELAY,
      CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY,
      CSettings::SETTING_SMB_WINSSERVER,
      CSettings::SETTING_SMB_WORKGROUP,
      CSettings::SETTING_SMB_MINPROTOCOL,
      CSettings::SETTING_SMB_MAXPROTOCOL,
      CSettings::SETTING_SMB_LEGACYSECURITY,
      CSettings::SETTING_SERVICES_WSDISCOVERY,
  };
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_settings->GetSettingsManager()->RegisterCallback(this, settingSet);
}

CNetworkServices::~CNetworkServices()
{
  m_settings->GetSettingsManager()->UnregisterCallback(this);
#ifdef HAS_WEB_SERVER
  m_webserver.UnregisterRequestHandler(&m_httpImageHandler);
  delete &m_httpImageHandler;
  m_webserver.UnregisterRequestHandler(&m_httpImageTransformationHandler);
  delete &m_httpImageTransformationHandler;
  m_webserver.UnregisterRequestHandler(&m_httpVfsHandler);
  delete &m_httpVfsHandler;
  m_webserver.UnregisterRequestHandler(&m_httpJsonRpcHandler);
  delete &m_httpJsonRpcHandler;
  CJSONRPC::Cleanup();
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

bool CNetworkServices::OnSettingChanging(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
#ifdef HAS_WEB_SERVER
  // Ask user to confirm disabling the authentication requirement, but not when the configuration
  // would be invalid when authentication was enabled (meaning that the change was triggered
  // automatically)
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION &&
      !std::static_pointer_cast<const CSettingBool>(setting)->GetValue() &&
      (!m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER) ||
       (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER) &&
        !m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())) &&
      HELPERS::ShowYesNoDialogText(19098, 36634) != DialogResponse::CHOICE_YES)
  {
    // Leave it as-is
    return false;
  }

  if (settingId == CSettings::SETTING_SERVICES_WEBSERVER ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERSSL ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERUSERNAME ||
      settingId == CSettings::SETTING_SERVICES_WEBSERVERPASSWORD)
  {
    if (IsWebserverRunning() && !StopWebserver())
      return false;

    if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
    {
      // Prevent changing to an invalid configuration
      if ((settingId == CSettings::SETTING_SERVICES_WEBSERVER ||
           settingId == CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION ||
           settingId == CSettings::SETTING_SERVICES_WEBSERVERPASSWORD) &&
          m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION) &&
          m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())
      {
        if (settingId == CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION)
        {
          HELPERS::ShowOKDialogText(CVariant{257}, CVariant{36636});
        }
        else
        {
          HELPERS::ShowOKDialogText(CVariant{257}, CVariant{36635});
        }
        return false;
      }

      // Ask for confirmation when enabling the web server
      if (settingId == CSettings::SETTING_SERVICES_WEBSERVER &&
          HELPERS::ShowYesNoDialogText(19098, 36632) != DialogResponse::CHOICE_YES)
      {
        // Revert change, do not start server
        return false;
      }

      if (!StartWebserver())
      {
        HELPERS::ShowOKDialogText(CVariant{33101}, CVariant{33100});
        return false;
      }
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESPORT ||
           settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT)
    return ValidatePort(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else
#endif // HAS_WEB_SERVER

#ifdef HAS_ZEROCONF
  if (settingId == CSettings::SETTING_SERVICES_ZEROCONF)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
      return StartZeroconf();
#ifdef HAS_AIRPLAY
    else
    {
      // cannot disable
      if (IsAirPlayServerRunning() || IsAirTunesServerRunning())
      {
        HELPERS::ShowOKDialogText(CVariant{1259}, CVariant{34303});
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
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
#ifdef HAS_ZEROCONF
      // AirPlay needs zeroconf
      if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ZEROCONF))
      {
        HELPERS::ShowOKDialogText(CVariant{1273}, CVariant{34302});
        return false;
      }
#endif //HAS_ZEROCONF

      // note - airtunesserver has to start before airplay server (ios7 client detection bug)
#ifdef HAS_AIRTUNES
      if (!StartAirTunesServer())
      {
        HELPERS::ShowOKDialogText(CVariant{1274}, CVariant{33100});
        return false;
      }
#endif //HAS_AIRTUNES

      if (!StartAirPlayServer())
      {
        HELPERS::ShowOKDialogText(CVariant{1273}, CVariant{33100});
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
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      if (!StartAirPlayServer())
      {
        HELPERS::ShowOKDialogText(CVariant{1273}, CVariant{33100});
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
    if (!m_settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
      return false;

    if (!CAirPlayServer::SetCredentials(m_settings->GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                        m_settings->GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
      return false;
  }
  else
#endif //HAS_AIRPLAY

#ifdef HAS_UPNP
  if (settingId == CSettings::SETTING_SERVICES_UPNP)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      StartUPnPClient();
      StartUPnPController();
      StartUPnPServer();
      StartUPnPRenderer();
    }
    else
    {
      StopUPnPRenderer();
      StopUPnPServer();
      StopUPnPController();
      StopUPnPClient();
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_UPNPSERVER)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
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
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_UPNPCONTROLLER)
  {
    // always stop and restart
    StopUPnPController();
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
      return StartUPnPController();
  }
  else
#endif // HAS_UPNP

  if (settingId == CSettings::SETTING_SERVICES_ESENABLED)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      bool result = true;
      if (!StartEventServer())
      {
        HELPERS::ShowOKDialogText(CVariant{33102}, CVariant{33100});
        result = false;
      }

      if (!StartJSONRPCServer())
      {
        HELPERS::ShowOKDialogText(CVariant{33103}, CVariant{33100});
        result = false;
      }
      return result;
    }
    else
    {
      bool result = true;
      result = StopEventServer(true, true);
      result &= StopJSONRPCServer(false);
      return result;
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESPORT)
  {
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      HELPERS::ShowOKDialogText(CVariant{33102}, CVariant{33100});
      return false;
    }

#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
    // reconfigure XBMCHelper for port changes
    XBMCHelper::GetInstance().Configure();
#endif // TARGET_DARWIN_OSX
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESALLINTERFACES)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESALLINTERFACES) &&
        HELPERS::ShowYesNoDialogText(19098, 36633) != DialogResponse::CHOICE_YES)
    {
      // Revert change, do not start server
      return false;
    }

    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    {
      if (!StopEventServer(true, true))
        return false;

      if (!StartEventServer())
      {
        HELPERS::ShowOKDialogText(CVariant{33102}, CVariant{33100});
        return false;
      }
    }

    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    {
      if (!StopJSONRPCServer(true))
        return false;

      if (!StartJSONRPCServer())
      {
        HELPERS::ShowOKDialogText(CVariant{33103}, CVariant{33100});
        return false;
      }
    }
  }

  else if (settingId == CSettings::SETTING_SERVICES_ESINITIALDELAY ||
           settingId == CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
      return RefreshEventServer();
  }

#if defined(HAS_FILESYSTEM_SMB)
  else if (settingId == CSettings::SETTING_SERVICES_WSDISCOVERY)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      if (!StartWSDiscovery())
        return false;
    }
    else
      return StopWSDiscovery();
  }
#endif // HAS_FILESYSTEM_SMB

  return true;
}

void CNetworkServices::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_SMB_WINSSERVER ||
      settingId == CSettings::SETTING_SMB_WORKGROUP ||
      settingId == CSettings::SETTING_SMB_MINPROTOCOL ||
      settingId == CSettings::SETTING_SMB_MAXPROTOCOL ||
      settingId == CSettings::SETTING_SMB_LEGACYSECURITY)
  {
    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
    //! @todo - General way of handling setting changes that require restart
    if (HELPERS::ShowYesNoDialogText(CVariant{14038}, CVariant{14039}) ==
        DialogResponse::CHOICE_YES)
    {
      m_settings->Save();
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTARTAPP);
    }
  }
}

bool CNetworkServices::OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                                       const char* oldSettingId,
                                       const TiXmlNode* oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERUSERNAME)
  {
    // if webserverusername is xbmc and pw is not empty we treat it as altered
    // and don't change the username to kodi - part of rebrand
    if (m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME) == "xbmc" &&
        !m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())
      return true;
  }
  if (settingId == CSettings::SETTING_SERVICES_WEBSERVERPORT)
  {
    // if webserverport is default but webserver is activated then treat it as altered
    // and don't change the port to new value
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
      return true;
  }
  return false;
}

void CNetworkServices::Start()
{
  StartZeroconf();
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    StartUPnP();
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED) && !StartEventServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED) && !StartJSONRPCServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33103), g_localizeStrings.Get(33100));

#ifdef HAS_WEB_SERVER
  // Start web server after eventserver and JSON-RPC server, so users can use these interfaces
  // to confirm the warning message below if it is shown
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
  {
    // services.webserverauthentication setting was added in Kodi v18 and requires a valid password
    // to be set, but on upgrade the setting will be activated automatically regardless of whether
    // a password was set before -> this can lead to an invalid configuration
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION) &&
        m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())
    {
      // Alert user to new default security settings in new Kodi version
      HELPERS::ShowOKDialogText(33101, 33104);
      // Fix settings: Disable web server
      m_settings->SetBool(CSettings::SETTING_SERVICES_WEBSERVER, false);
      // Bring user to settings screen where authentication can be configured properly
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(
          WINDOW_SETTINGS_SERVICE, std::vector<std::string>{"services.webserverauthentication"});
    }
    // Only try to start server if configuration is OK
    else if (!StartWebserver())
      CGUIDialogKaiToast::QueueNotification(
          CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
  }
#endif // HAS_WEB_SERVER

  // note - airtunesserver has to start before airplay server (ios7 client detection bug)
  StartAirTunesServer();
  StartAirPlayServer();
  StartRss();
  StartWSDiscovery();
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
  StopWSDiscovery();
}

bool CNetworkServices::StartServer(enum ESERVERS server, bool start)
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings)
    return false;

  bool ret = false;
  switch (server)
  {
    case ES_WEBSERVER:
      // the callback will take care of starting/stopping webserver
      ret = settings->SetBool(CSettings::SETTING_SERVICES_WEBSERVER, start);
      break;

    case ES_AIRPLAYSERVER:
      // the callback will take care of starting/stopping airplay
      ret = settings->SetBool(CSettings::SETTING_SERVICES_AIRPLAY, start);
      break;

    case ES_JSONRPCSERVER:
      // the callback will take care of starting/stopping jsonrpc server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ESENABLED, start);
      break;

    case ES_UPNPSERVER:
      // the callback will take care of starting/stopping upnp server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPSERVER, start);
      break;

    case ES_UPNPRENDERER:
      // the callback will take care of starting/stopping upnp renderer
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPRENDERER, start);
      break;

    case ES_EVENTSERVER:
      // the callback will take care of starting/stopping event server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ESENABLED, start);
      break;

    case ES_ZEROCONF:
      // the callback will take care of starting/stopping zeroconf
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ZEROCONF, start);
      break;

    case ES_WSDISCOVERY:
      // the callback will take care of starting/stopping WS-Discovery
      ret = settings->SetBool(CSettings::SETTING_SERVICES_WSDISCOVERY, start);
      break;

    default:
      ret = false;
      break;
  }
  settings->Save();

  return ret;
}

bool CNetworkServices::StartWebserver()
{
#ifdef HAS_WEB_SERVER
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVER))
    return false;

  if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION) &&
      m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD).empty())
  {
    CLog::Log(LOGERROR, "Tried to start webserver with invalid configuration (authentication "
                        "enabled, but no password set");
    return false;
  }

  int webPort = m_settings->GetInt(CSettings::SETTING_SERVICES_WEBSERVERPORT);
  if (!ValidatePort(webPort))
  {
    CLog::Log(LOGERROR, "Cannot start Web Server on port {}", webPort);
    return false;
  }

  if (IsWebserverRunning())
    return true;

  std::string username;
  std::string password;
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION))
  {
    username = m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME);
    password = m_settings->GetString(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD);
  }

  if (!m_webserver.Start(webPort, username, password))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  txt.emplace_back("txtvers", "1");
  txt.emplace_back("uuid", CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                             CSettings::SETTING_SERVICES_DEVICEUUID));

  // publish web frontend and API services
#ifdef HAS_WEB_INTERFACE
  CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp",
                                           CSysInfo::GetDeviceName() + " webserver", webPort, txt);
#endif // HAS_WEB_INTERFACE
  CZeroconf::GetInstance()->PublishService("servers.jsonrpc-http", "_xbmc-jsonrpc-h._tcp",
                                           CSysInfo::GetDeviceName() + " jsonrpc-http", webPort,
                                           txt);
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
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-http");
#endif // HAS_ZEROCONF

  return true;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::StartAirPlayServer()
{
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT))
    return true;

#ifdef HAS_AIRPLAY
  if (!CServiceBroker::GetNetwork().IsAvailable() || !m_settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
    return false;

  if (IsAirPlayServerRunning())
    return true;

  if (!CAirPlayServer::StartServer(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_airPlayPort, true))
    return false;

  if (!CAirPlayServer::SetCredentials(m_settings->GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                      m_settings->GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
  txt.emplace_back("deviceid", iface != nullptr ? iface->GetMacAddress() : "FF:FF:FF:FF:FF:F2");
  txt.emplace_back("model", "Xbmc,1");
  txt.emplace_back("srcvers", AIRPLAY_SERVER_VERSION_STR);

  // for ios8 clients we need to announce mirroring support
  // else we won't get video urls anymore.
  // We also announce photo caching support (as it seems faster and
  // we have implemented it anyways).
  txt.emplace_back("features", "0x20F7");

  CZeroconf::GetInstance()->PublishService(
      "servers.airplay", "_airplay._tcp", CSysInfo::GetDeviceName() + " airplay",
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_airPlayPort, txt);
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
  if (!CServiceBroker::GetNetwork().IsAvailable() || !m_settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
    return false;

  if (IsAirTunesServerRunning())
    return true;

  if (!CAirTunesServer::StartServer(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_airTunesPort, true,
                                    m_settings->GetBool(CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD),
                                    m_settings->GetString(CSettings::SETTING_SERVICES_AIRPLAYPASSWORD)))
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
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (IsJSONRPCServerRunning())
    return true;

  if (!CTCPServer::StartServer(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonTcpPort, m_settings->GetBool(CSettings::SETTING_SERVICES_ESALLINTERFACES)))
    return false;

#ifdef HAS_ZEROCONF
  std::vector<std::pair<std::string, std::string> > txt;
  txt.emplace_back("txtvers", "1");
  txt.emplace_back("uuid", CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                             CSettings::SETTING_SERVICES_DEVICEUUID));

  CZeroconf::GetInstance()->PublishService(
      "servers.jsonrpc-tcp", "_xbmc-jsonrpc._tcp", CSysInfo::GetDeviceName() + " jsonrpc-tcp",
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonTcpPort, txt);
#endif // HAS_ZEROCONF

  return true;
}

bool CNetworkServices::IsJSONRPCServerRunning()
{
  return CTCPServer::IsRunning();
}

bool CNetworkServices::StopJSONRPCServer(bool bWait)
{
  if (!IsJSONRPCServerRunning())
    return true;

  CTCPServer::StopServer(bWait);

#ifdef HAS_ZEROCONF
  CZeroconf::GetInstance()->RemoveService("servers.jsonrpc-tcp");
#endif // HAS_ZEROCONF

  return true;
}

bool CNetworkServices::StartEventServer()
{
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
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
}

bool CNetworkServices::IsEventServerRunning()
{
  return CEventServer::GetInstance()->Running();
}

bool CNetworkServices::StopEventServer(bool bWait, bool promptuser)
{
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
      if (HELPERS::ShowYesNoDialogText(CVariant{13140}, CVariant{13141}, CVariant{""}, CVariant{""},
                                       10000) != DialogResponse::CHOICE_YES)
      {
        CLog::Log(LOGINFO, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGINFO, "ES: Stopping event server with confirmation");

    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
      CLog::Log(LOGINFO, "ES: Stopping event server");

    CEventServer::GetInstance()->StopServer(bWait);
  }

  return true;
}

bool CNetworkServices::RefreshEventServer()
{
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (!IsEventServerRunning())
    return false;

  CEventServer::GetInstance()->RefreshSettings();
  return true;
}

bool CNetworkServices::StartUPnP()
{
  bool ret = false;
#ifdef HAS_UPNP
  ret |= StartUPnPClient();
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPSERVER))
  {
   ret |= StartUPnPServer();
  }

  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPCONTROLLER))
  {
    ret |= StartUPnPController();
  }

  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER))
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

  CLog::Log(LOGINFO, "stopping upnp");
  CUPnP::ReleaseInstance(bWait);

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPClient()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp client");
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

  CLog::Log(LOGINFO, "stopping upnp client");
  CUPnP::GetInstance()->StopClient();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPController()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPCONTROLLER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPSERVER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp controller");
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

  CLog::Log(LOGINFO, "stopping upnp controller");
  CUPnP::GetInstance()->StopController();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp renderer");
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

  CLog::Log(LOGINFO, "stopping upnp renderer");
  CUPnP::GetInstance()->StopRenderer();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPSERVER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp server");
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

  CLog::Log(LOGINFO, "stopping upnp server");
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
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ZEROCONF))
    return false;

  if (IsZeroconfRunning())
    return true;

  CLog::Log(LOGINFO, "starting zeroconf publishing");
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

  CLog::Log(LOGINFO, "stopping zeroconf publishing");
  CZeroconf::GetInstance()->Stop();

  return true;
#endif // HAS_ZEROCONF
  return false;
}

bool CNetworkServices::StartWSDiscovery()
{
#if defined(HAS_FILESYSTEM_SMB)
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_WSDISCOVERY))
    return false;

  if (IsWSDiscoveryRunning())
    return true;

  return CServiceBroker::GetWSDiscovery().StartServices();
#endif // HAS_FILESYSTEM_SMB
  return false;
}

bool CNetworkServices::IsWSDiscoveryRunning()
{
#if defined(HAS_FILESYSTEM_SMB)
  return CServiceBroker::GetWSDiscovery().IsRunning();
#endif // HAS_FILESYSTEM_SMB
  return false;
}

bool CNetworkServices::StopWSDiscovery()
{
#if defined(HAS_FILESYSTEM_SMB)
  if (!IsWSDiscoveryRunning())
    return true;

  CServiceBroker::GetWSDiscovery().StopServices();

  return true;
#endif // HAS_FILESYSTEM_SMB
  return false;
}

bool CNetworkServices::ValidatePort(int port)
{
  if (port <= 0 || port > 65535)
    return false;

#ifdef TARGET_LINUX
  if (!CUtil::CanBindPrivileged() && (port < 1024))
    return false;
#endif

  return true;
}
