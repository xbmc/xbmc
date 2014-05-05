//
//  PlexHTTPRemoteHandler.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-16.
//
//

#include "PlexHTTPRemoteHandler.h"
#include "network/WebServer.h"
#include <boost/algorithm/string.hpp>

#include "Client/PlexServerManager.h"
#include "FileSystem/PlexDirectory.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"

#include "utils/log.h"
#include "interfaces/Builtins.h"
#include "PlexRemoteSubscriberManager.h"
#include "Client/PlexTimelineManager.h"

#include <boost/asio/detail/socket_ops.hpp>
#include "PlexApplication.h"

#include "settings/GUISettings.h"
#include "guilib/GUIEditControl.h"
#include "GUIAudioManager.h"

#include "PlayList.h"
#include "Settings.h"

#include "GUIWindowSlideShow.h"
#include "PlexNavigationHelper.h"

#include "ViewDatabase.h"

#include "PlexRemoteApplicationHandler.h"
#include "PlexRemoteNavigationHandler.h"
#include "PlexRemotePlaybackHandler.h"
#include "PlexRemotePlayHandler.h"


static CPlexRemoteApplicationHandler* applicationHandler;
static CPlexRemoteNavigationHandler* navigationHandler;
static CPlexRemotePlaybackHandler* playbackHandler;
static CPlexRemotePlayHandler* playHandler;

#define LEGACY 1


///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexHTTPRemoteHandler::CPlexHTTPRemoteHandler()
{
  applicationHandler = new CPlexRemoteApplicationHandler;
  navigationHandler = new CPlexRemoteNavigationHandler;
  playbackHandler = new CPlexRemotePlaybackHandler;
  playHandler = new CPlexRemotePlayHandler;
}

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPRemoteHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return boost::starts_with(request.url, "/player") || boost::starts_with(request.url, "/resources");;
}

////////////////////////////////////////////////////////////////////////////////////////
int CPlexHTTPRemoteHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  /* defaults */
  m_responseType = HTTPMemoryDownloadNoFreeCopy;
  m_responseCode = MHD_HTTP_OK;

  ArgMap argumentMap;
  ArgMap headerMap;
  CStdString path(request.url);
  
  CWebServer::GetRequestHeaderValues(request.connection, MHD_GET_ARGUMENT_KIND, argumentMap);
  CWebServer::GetRequestHeaderValues(request.connection, MHD_HEADER_KIND, headerMap);

  /* first see if we need to handle CORS requests - Access-Control-Allow-Origin needs to be
   * available for all requests, the other headers is only needed on a OPTIONS call */
  m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Origin", "*"));
  m_responseHeaderFields.insert(std::pair<std::string, std::string>("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid")));
  if (request.method == OPTIONS &&
      headerMap.find("Access-Control-Request-Method") != headerMap.end())
  {
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Content-Type", "text/plain"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT, HEAD"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Max-Age", "1209600"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Connection", "close"));

    if (headerMap.find("Access-Control-Request-Headers") != headerMap.end())
      m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Headers", headerMap["Access-Control-Request-Headers"]));

    return MHD_YES;
  }

  m_responseHeaderFields.insert(std::pair<std::string, std::string>("Content-Type", "text/xml"));
  CPlexRemoteResponse response = updateCommandID(request, argumentMap);
  if (response.code != 200)
  {
    m_data = response.body;
  }

  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::HandleHTTPRequest handling %s", request.url.c_str());

  if (boost::starts_with(path, "/resources"))
    response = resources();
  else if (path.Equals("/player/playback/playMedia") ||
      /* LEGACY */ path.Equals("/player/application/playMedia"))
    response = playHandler->handle(path, argumentMap);
  else if (boost::starts_with(path, "/player/playback"))
    response = playbackHandler->handle(path, argumentMap);
  else if (boost::starts_with(path, "/player/navigation"))
    response = navigationHandler->handle(path, argumentMap);
  else if (boost::starts_with(path, "/player/application"))
    response = applicationHandler->handle(path, argumentMap);
  else if (path.Equals("/player/timeline/subscribe"))
    response = subscribe(request, argumentMap);
  else if (path.Equals("/player/mirror/details"))
    response = showDetails(argumentMap);
  else if (path.Equals("/player/timeline/unsubscribe"))
    response = unsubscribe(request, argumentMap);
  else if (path.Equals("/player/timeline/poll"))
    response = poll(request, argumentMap);
  else
    response = CPlexRemoteResponse(500, "Not implemented");

  m_data = response.body;

  return MHD_YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
class NavigationTimeout : public IPlexGlobalTimeout
{
  public:
    NavigationTimeout() {}
    void OnTimeout()
    {
      if (!g_application.IsPlayingFullScreenVideo())
        CApplicationMessenger::Get().ActivateWindow(WINDOW_HOME, std::vector<CStdString>(), true);
    }

    CStdString TimerName() const { return "navigationTimeout"; }
};

static NavigationTimeout* navTimeout;

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::showDetails(const ArgMap &arguments)
{
  CPlexServerPtr server = getServerFromArguments(arguments);

  if (!server)
  {
    CLog::Log(LOGERROR, "CPlexRemotePlaybackHandler::showDetails Can't find server.");
    return CPlexRemoteResponse(500, "Can't find server");
  }

  std::string key;

  if (arguments.find("key") != arguments.end())
  {
    key = arguments.find("key")->second;
  }
  else
  {
    CLog::Log(LOGERROR, "CPlexRemotePlaybackHandler::showDetails needs a key argument to show something");
    return CPlexRemoteResponse(500, "Need key argument");
  }

  if (!PlexUtils::CurrentSkinHasPreplay() ||
      g_application.IsPlayingFullScreenVideo() ||
      g_application.IsVisualizerActive() ||
      g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return CPlexRemoteResponse();

  CURL u = server->BuildPlexURL(key);

  XFILE::CPlexDirectory dir;
  CFileItemList list;

  if (dir.GetDirectory(u.Get(), list))
  {
    if (list.Size() == 1)
    {
      CFileItemPtr item = list.Get(0);

      /* FIXME: the pre-play for Shows and Epsiodes are not really looking
       * great yet, so let's skip them for now */
      if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_ALBUM ||
          item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE ||
          item->GetPlexDirectoryType() == PLEX_DIR_TYPE_ARTIST ||
          item->GetPlexDirectoryType() == PLEX_DIR_TYPE_EPISODE)
      {
        CPlexNavigationHelper nav;
        nav.navigateToItem(item, CURL(), WINDOW_HOME, true);

        g_application.WakeUpScreenSaverAndDPMS();

        if (!navTimeout)
          navTimeout = new NavigationTimeout;
        g_plexApplication.timer->RestartTimeout(5 * 60 * 1000, navTimeout);
      }
    }
  }

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
void* CPlexHTTPRemoteHandler::GetHTTPResponseData() const
{
  return (void*)m_data.c_str();
}

////////////////////////////////////////////////////////////////////////////////////////
size_t CPlexHTTPRemoteHandler::GetHTTPResonseDataLength() const
{
  return m_data.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::updateCommandID(const HTTPRequest &request, const ArgMap &arguments)
{
  if (arguments.find("commandID") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::updateCommandID no commandID sent to this request!");
    return CPlexRemoteResponse();
  }

  int commandID = -1;
  try { commandID = boost::lexical_cast<int>(arguments.find("commandID")->second); }
  catch (boost::bad_lexical_cast) { return CPlexRemoteResponse(500, "commandID is not a integer!"); }

  std::string uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  if (uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::updateCommandID subscriber didn't set X-Plex-Client-Identifier");
    return CPlexRemoteResponse(500, "When commandID is set you also need to specify X-Plex-Client-Identifier");
  }

  CPlexRemoteSubscriberPtr sub = g_plexApplication.remoteSubscriberManager->findSubscriberByUUID(uuid);
  if (sub)
    sub->setCommandID(commandID);

  return CPlexRemoteResponse();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexServerPtr CPlexHTTPRemoteHandler::getServerFromArguments(const ArgMap &arguments)
{
  CPlexServerPtr server;

  if (arguments.find("machineIdentifier") != arguments.end())
  {
    std::string uuid = arguments.find("machineIdentifier")->second;
    server = g_plexApplication.serverManager->FindByUUID(uuid);
    if (server)
      return server;

    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::getServerFromArguments request had machineIdentifier but that server is not found.");
  }

  /* no machineIdentfier or server not found, at this point we need to synthesize the server instead */
  std::string address, token, portStr, schema;
  int port;

  if (arguments.find("address") != arguments.end())
    address = arguments.find("address")->second;

  if (arguments.find("token") != arguments.end())
    token = arguments.find("token")->second;

  if (arguments.find("protocol") != arguments.end())
    schema = arguments.find("protocol")->second;

  if (schema.empty())
    schema = "http";

  if (arguments.find("port") != arguments.end())
  {
    portStr = arguments.find("port")->second;
    if (!portStr.empty())
    {
      try { port = boost::lexical_cast<int>(portStr); }
      catch (...)
      {
        port = 32400;
        CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::getServerFromArguments port was not parsable, just guessing here.");
      }
    }
  }

  if (address.empty())
  {
    CLog::Log(LOGERROR, "CPlexHTTPRemoteHandler::getServerFromArguments no address found! Can't synthesize server!");
    return server;
  }

  CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED, address, port, schema, token));
  server = CPlexServerPtr(new CPlexServer(connection));
  return server;
}


////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::subscribe(const HTTPRequest &request, const ArgMap &arguments)
{
  CPlexRemoteSubscriberPtr sub = getSubFromRequest(request, arguments);
  if (sub && g_plexApplication.remoteSubscriberManager && g_plexApplication.timelineManager)
  {
    sub = g_plexApplication.remoteSubscriberManager->addSubscriber(sub);

    if (sub)
      g_plexApplication.timelineManager->SendCurrentTimelineToSubscriber(sub);
  }

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::unsubscribe(const HTTPRequest &request, const ArgMap &arguments)
{
  if (g_plexApplication.remoteSubscriberManager)
    g_plexApplication.remoteSubscriberManager->removeSubscriber(getSubFromRequest(request, arguments));
  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriberPtr CPlexHTTPRemoteHandler::getSubFromRequest(const HTTPRequest &request, const ArgMap &arguments)
{
  std::string uuid, name;
  
  uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  if (uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::subscribe subscriber didn't set X-Plex-Client-Identifier");
    return CPlexRemoteSubscriberPtr();
  }

  name = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Device-Name");
  if (name.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::subscribe subscriber didn't set X-Plex-Device-Name");
    return CPlexRemoteSubscriberPtr();
  }
  
  char ipstr[INET_ADDRSTRLEN];

  struct sockaddr_in *so = (struct sockaddr_in *)MHD_get_connection_info(request.connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr;
  boost::system::error_code ec;
  boost::asio::detail::socket_ops::inet_ntop(AF_INET, &(so->sin_addr), ipstr, INET_ADDRSTRLEN, 0, ec);

  if (!so)
    return CPlexRemoteSubscriberPtr();
  
  int port = 32400;
  int commandID = -1;
  CStdString protocol = "http";

  if (arguments.find("port") != arguments.end())
    port = boost::lexical_cast<int>(arguments.find("port")->second);

  if (arguments.find("commandID") != arguments.end())
    commandID = boost::lexical_cast<int>(arguments.find("commandID")->second);

  if (arguments.find("protocol") != arguments.end())
    protocol = arguments.find("protocol")->second;
  
  CPlexRemoteSubscriberPtr sub = CPlexRemoteSubscriber::NewSubscriber(uuid, ipstr, port, commandID);
  sub->setName(name);
  
  return sub;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::poll(const HTTPRequest &request, const ArgMap &arguments)
{
  bool wait = false;

  std::string uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  std::string name = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Device-Name");
  int commandID = -1;

  if (arguments.find("commandID") != arguments.end())
  {
    try { commandID = boost::lexical_cast<int>(arguments.find("commandID")->second); }
    catch (boost::bad_lexical_cast) {}
  }

  if (commandID == -1 || uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::poll the poller needs to set both X-Plex-Client-Identifier header and commandID arguments.");
    return CPlexRemoteResponse(500,
                               "You need to specify both x-Plex-Client-Identifier as a header and commandID as a argument");
  }

  CPlexRemoteSubscriberPtr pollSubscriber = CPlexRemoteSubscriber::NewPollSubscriber(uuid, commandID);
  if (!pollSubscriber)
    return CPlexRemoteResponse(500, "Could not create a poll subscriber. (internal error)");

  if (!name.empty())
    pollSubscriber->setName(name);

  if (g_plexApplication.remoteSubscriberManager && g_plexApplication.timelineManager)
    pollSubscriber = g_plexApplication.remoteSubscriberManager->addSubscriber(pollSubscriber);

  if (!pollSubscriber)
    return CPlexRemoteResponse(500, "We are going away!");

  if (arguments.find("wait") != arguments.end())
  {
    if (arguments.find("wait")->second == "1" || arguments.find("wait")->second == "true")
      wait = true;
  }

  CXBMCTinyXML xmlDoc;

  if (wait)
    xmlDoc = pollSubscriber->waitForTimeline();
  else
    xmlDoc = g_plexApplication.timelineManager->GetCurrentTimeLines()->getTimelinesXML(pollSubscriber->getCommandID());

  m_responseHeaderFields.insert(std::make_pair("Access-Control-Expose-Headers", "X-Plex-Client-Identifier"));

  return CPlexRemoteResponse(xmlDoc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexHTTPRemoteHandler::resources()
{
  CXBMCTinyXML doc;
  doc.LinkEndChild(new TiXmlDeclaration("1.0", "utf-8", ""));

  TiXmlElement *mediaContainer = new TiXmlElement("MediaContainer");
  doc.LinkEndChild(mediaContainer);

  // title="My Nexus 7" machineIdentifier="x" product="p" platform="p" platformVersion="v" protocolVersion="x" protocolCapabilities="y" deviceClass="z"
  TiXmlElement *player = new TiXmlElement("Player");
  player->SetAttribute("title", g_guiSettings.GetString("services.devicename").c_str());
  player->SetAttribute("protocol", "plex");
  player->SetAttribute("protocolVersion", "1");
  player->SetAttribute("protocolCapabilities", "navigation,playback,timeline,mirror,playqueue");
  player->SetAttribute("machineIdentifier", g_guiSettings.GetString("system.uuid").c_str());
  player->SetAttribute("product", "Plex Home Theater");
  player->SetAttribute("platform", PlexUtils::GetMachinePlatform());
  player->SetAttribute("platformVersion", PlexUtils::GetMachinePlatformVersion());
  player->SetAttribute("deviceClass", "pc");

  mediaContainer->LinkEndChild(player);

  return CPlexRemoteResponse(doc);
}


