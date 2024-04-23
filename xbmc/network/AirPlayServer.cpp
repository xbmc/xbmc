/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  Many concepts and protocol specification in this code are taken
 *  from Airplayer. https://github.com/PascalW/Airplayer
 */

#include "AirPlayServer.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationVolumeHandling.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "playlists/PlayListTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <mutex>

#include <arpa/inet.h>
#include <netinet/in.h>
#ifdef HAS_ZEROCONF
#include "network/Zeroconf.h"
#endif // HAS_ZEROCONF

#include <inttypes.h>

#include <plist/plist.h>

using KODI::UTILITY::CDigest;
using namespace std::chrono_literals;

#ifdef TARGET_WINDOWS
#define close closesocket
#endif

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_PRECONDITION_FAILED 412
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

CCriticalSection CAirPlayServer::ServerInstanceLock;
CAirPlayServer *CAirPlayServer::ServerInstance = NULL;
int CAirPlayServer::m_isPlaying = 0;

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
const char *eventStrings[] = {"playing", "paused", "loading", "stopped"};

#define PLAYBACK_INFO \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" \
  "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" " \
  "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n" \
  "<plist version=\"1.0\">\r\n" \
  "<dict>\r\n" \
  "<key>duration</key>\r\n" \
  "<real>{:f}</real>\r\n" \
  "<key>loadedTimeRanges</key>\r\n" \
  "<array>\r\n" \
  "\t\t<dict>\r\n" \
  "\t\t\t<key>duration</key>\r\n" \
  "\t\t\t<real>{:f}</real>\r\n" \
  "\t\t\t<key>start</key>\r\n" \
  "\t\t\t<real>0.0</real>\r\n" \
  "\t\t</dict>\r\n" \
  "</array>\r\n" \
  "<key>playbackBufferEmpty</key>\r\n" \
  "<true/>\r\n" \
  "<key>playbackBufferFull</key>\r\n" \
  "<false/>\r\n" \
  "<key>playbackLikelyToKeepUp</key>\r\n" \
  "<true/>\r\n" \
  "<key>position</key>\r\n" \
  "<real>{:f}</real>\r\n" \
  "<key>rate</key>\r\n" \
  "<real>{:d}</real>\r\n" \
  "<key>readyToPlay</key>\r\n" \
  "<true/>\r\n" \
  "<key>seekableTimeRanges</key>\r\n" \
  "<array>\r\n" \
  "\t\t<dict>\r\n" \
  "\t\t\t<key>duration</key>\r\n" \
  "\t\t\t<real>{:f}</real>\r\n" \
  "\t\t\t<key>start</key>\r\n" \
  "\t\t\t<real>0.0</real>\r\n" \
  "\t\t</dict>\r\n" \
  "</array>\r\n" \
  "</dict>\r\n" \
  "</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" \
  "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" " \
  "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n" \
  "<plist version=\"1.0\">\r\n" \
  "<dict>\r\n" \
  "<key>deviceid</key>\r\n" \
  "<string>{:s}</string>\r\n" \
  "<key>features</key>\r\n" \
  "<integer>119</integer>\r\n" \
  "<key>model</key>\r\n" \
  "<string>Kodi,1</string>\r\n" \
  "<key>protovers</key>\r\n" \
  "<string>1.0</string>\r\n" \
  "<key>srcvers</key>\r\n" \
  "<string>" AIRPLAY_SERVER_VERSION_STR "</string>\r\n" \
  "</dict>\r\n" \
  "</plist>\r\n"

#define EVENT_INFO \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n" \
  "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" " \
  "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n" \
  "<plist version=\"1.0\">\r\n" \
  "<dict>\r\n" \
  "<key>category</key>\r\n" \
  "<string>video</string>\r\n" \
  "<key>sessionID</key>\r\n" \
  "<integer>{:d}</integer>\r\n" \
  "<key>state</key>\r\n" \
  "<string>{:s}</string>\r\n" \
  "</dict>\r\n" \
  "</plist>\r\n"

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\"" AUTH_REALM "\", nonce=\"{:s}\"\r\n"

void CAirPlayServer::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                              const std::string& sender,
                              const std::string& message,
                              const CVariant& data)
{
  // We are only interested in player changes
  if ((flag & ANNOUNCEMENT::Player) == 0)
    return;

  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);

  if (sender == ANNOUNCEMENT::CAnnouncementManager::ANNOUNCEMENT_SENDER && ServerInstance)
  {
    if (message == "OnStop")
    {
      bool shouldRestoreVolume = true;
      if (data.isMember("player") && data["player"].isMember("playerid"))
        shouldRestoreVolume = (data["player"]["playerid"] != PLAYLIST::TYPE_PICTURE);

      if (shouldRestoreVolume)
        restoreVolume();

      ServerInstance->AnnounceToClients(EVENT_STOPPED);
    }
    else if (message == "OnPlay" || message == "OnResume")
    {
      ServerInstance->AnnounceToClients(EVENT_PLAYING);
    }
    else if (message == "OnPause")
    {
      ServerInstance->AnnounceToClients(EVENT_PAUSED);
    }
  }
}

bool CAirPlayServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);

  ServerInstance = new CAirPlayServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
    return true;
  }
  else
    return false;
}

bool CAirPlayServer::SetCredentials(bool usePassword, const std::string& password)
{
  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);
  bool ret = false;

  if (ServerInstance)
  {
    ret = ServerInstance->SetInternalCredentials(usePassword, password);
  }
  return ret;
}

bool CAirPlayServer::SetInternalCredentials(bool usePassword, const std::string& password)
{
  m_usePassword = usePassword;
  m_password = password;
  return true;
}

void ClearPhotoAssetCache()
{
  CLog::Log(LOGINFO, "AIRPLAY: Cleaning up photoassetcache");
  // remove all cached photos
  CFileItemList items;
  XFILE::CDirectory::GetDirectory("special://temp/", items, "", XFILE::DIR_FLAG_DEFAULTS);

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      if (StringUtils::StartsWithNoCase(pItem->GetLabel(), "airplayasset") &&
          (StringUtils::EndsWithNoCase(pItem->GetLabel(), ".jpg") ||
           StringUtils::EndsWithNoCase(pItem->GetLabel(), ".png") ))
      {
        XFILE::CFile::Delete(pItem->GetPath());
      }
    }
  }
}

void CAirPlayServer::StopServer(bool bWait)
{
  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);
  //clean up the photo cache temp folder
  ClearPhotoAssetCache();

  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

bool CAirPlayServer::IsRunning()
{
  if (ServerInstance == NULL)
    return false;

  return static_cast<CThread*>(ServerInstance)->IsRunning();
}

void CAirPlayServer::AnnounceToClients(int state)
{
  std::unique_lock<CCriticalSection> lock(m_connectionLock);

  for (auto& it : m_connections)
  {
    std::string reverseHeader;
    std::string reverseBody;
    std::string response;
    int reverseSocket = INVALID_SOCKET;
    it.ComposeReverseEvent(reverseHeader, reverseBody, state);

    // Send event status per reverse http socket (play, loading, paused)
    // if we have a reverse header and a reverse socket
    if (!reverseHeader.empty() && m_reverseSockets.find(it.m_sessionId) != m_reverseSockets.end())
    {
      //search the reverse socket to this sessionid
      response = StringUtils::Format("POST /event HTTP/1.1\r\n");
      reverseSocket = m_reverseSockets[it.m_sessionId]; //that is our reverse socket
      response += reverseHeader;
    }
    response += "\r\n";

    if (!reverseBody.empty())
    {
      response += reverseBody;
    }

    // don't send it to the connection object
    // the reverse socket itself belongs to
    if (reverseSocket != INVALID_SOCKET && reverseSocket != it.m_socket)
    {
      send(reverseSocket, response.c_str(), response.size(), 0);//send the event status on the eventSocket
    }
  }
}

CAirPlayServer::CAirPlayServer(int port, bool nonlocal) : CThread("AirPlayServer")
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_usePassword = false;
  m_origVolume = -1;
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

CAirPlayServer::~CAirPlayServer()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void handleZeroconfAnnouncement()
{
#if defined(HAS_ZEROCONF)
  static XbmcThreads::EndTime<> timeout(10s);
  if(timeout.IsTimePast())
  {
    CZeroconf::GetInstance()->ForceReAnnounceService("servers.airplay");
    timeout.Set(10s);
  }
#endif
}

void CAirPlayServer::Process()
{
  m_bStop = false;
  static int sessionCounter = 0;

  while (!m_bStop)
  {
    int             max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    for (SOCKET socket : m_ServerSockets)
    {
      FD_SET(socket, &rfds);
      if ((intptr_t)socket > (intptr_t)max_fd)
        max_fd = socket;
    }

    for (unsigned int i = 0; i < m_connections.size(); i++)
    {
      FD_SET(m_connections[i].m_socket, &rfds);
      if (m_connections[i].m_socket > max_fd)
        max_fd = m_connections[i].m_socket;
    }

    int res = select(max_fd+1, &rfds, NULL, NULL, &to);
    if (res < 0)
    {
      CLog::Log(LOGERROR, "AIRPLAY Server: Select failed");
      CThread::Sleep(1000ms);
      Initialize();
    }
    else if (res > 0)
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i].m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          if (nread > 0)
          {
            std::string sessionId;
            m_connections[i].PushBuffer(this, buffer, nread, sessionId, m_reverseSockets);
          }
          if (nread <= 0)
          {
            std::unique_lock<CCriticalSection> lock(m_connectionLock);
            CLog::Log(LOGINFO, "AIRPLAY Server: Disconnection detected");
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      for (SOCKET socket : m_ServerSockets)
      {
        if (FD_ISSET(socket, &rfds))
        {
          CLog::Log(LOGDEBUG, "AIRPLAY Server: New connection detected");
          CTCPClient newconnection;
          newconnection.m_socket = accept(socket, (struct sockaddr*) &newconnection.m_cliaddr, &newconnection.m_addrlen);
          sessionCounter++;
          newconnection.m_sessionCounter = sessionCounter;

          if (newconnection.m_socket == INVALID_SOCKET)
          {
            CLog::Log(LOGERROR, "AIRPLAY Server: Accept of new connection failed: {}", errno);
            if (EBADF == errno)
            {
              CThread::Sleep(1000ms);
              Initialize();
              break;
            }
          }
          else
          {
            std::unique_lock<CCriticalSection> lock(m_connectionLock);
            CLog::Log(LOGINFO, "AIRPLAY Server: New connection added");
            m_connections.push_back(newconnection);
          }
        }
      }
    }

    // by reannouncing the zeroconf service
    // we fix issues where xbmc is detected
    // as audio-only target on devices with
    // ios7 and later
    handleZeroconfAnnouncement();
  }

  Deinitialize();
}

bool CAirPlayServer::Initialize()
{
  Deinitialize();

  m_ServerSockets = CreateTCPServerSocket(m_port, !m_nonlocal, 10, "AIRPLAY");
  if (m_ServerSockets.empty())
    return false;

  CLog::Log(LOGINFO, "AIRPLAY Server: Successfully initialized");
  return true;
}

void CAirPlayServer::Deinitialize()
{
  std::unique_lock<CCriticalSection> lock(m_connectionLock);
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i].Disconnect();

  m_connections.clear();
  m_reverseSockets.clear();

  for (SOCKET socket : m_ServerSockets)
  {
    shutdown(socket, SHUT_RDWR);
    close(socket);
  }
  m_ServerSockets.clear();
}

CAirPlayServer::CTCPClient::CTCPClient()
{
  m_socket = INVALID_SOCKET;
  m_httpParser = new HttpParser();

  m_addrlen = sizeof(struct sockaddr_storage);

  m_bAuthenticated = false;
  m_lastEvent = EVENT_NONE;
}

CAirPlayServer::CTCPClient::CTCPClient(const CTCPClient& client)
: m_lastEvent(EVENT_NONE)
{
  Copy(client);
  m_httpParser = new HttpParser();
}

CAirPlayServer::CTCPClient::~CTCPClient()
{
  delete m_httpParser;
}

CAirPlayServer::CTCPClient& CAirPlayServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  m_httpParser = new HttpParser();
  return *this;
}

void CAirPlayServer::CTCPClient::PushBuffer(CAirPlayServer *host, const char *buffer,
                                            int length, std::string &sessionId, std::map<std::string,
                                            int> &reverseSockets)
{
  HttpParser::status_t status = m_httpParser->addBytes(buffer, length);

  if (status == HttpParser::Done)
  {
    // Parse the request
    std::string responseHeader;
    std::string responseBody;
    int status = ProcessRequest(responseHeader, responseBody);
    sessionId = m_sessionId;
    std::string statusMsg = "OK";

    switch(status)
    {
      case AIRPLAY_STATUS_NOT_IMPLEMENTED:
        statusMsg = "Not Implemented";
        break;
      case AIRPLAY_STATUS_SWITCHING_PROTOCOLS:
        statusMsg = "Switching Protocols";
        reverseSockets[sessionId] = m_socket;//save this socket as reverse http socket for this sessionid
        break;
      case AIRPLAY_STATUS_NEED_AUTH:
        statusMsg = "Unauthorized";
        break;
      case AIRPLAY_STATUS_NOT_FOUND:
        statusMsg = "Not Found";
        break;
      case AIRPLAY_STATUS_METHOD_NOT_ALLOWED:
        statusMsg = "Method Not Allowed";
        break;
      case AIRPLAY_STATUS_PRECONDITION_FAILED:
        statusMsg = "Precondition Failed";
        break;
    }

    // Prepare the response
    std::string response;
    const time_t ltime = time(NULL);
    char *date = asctime(gmtime(&ltime)); //Fri, 17 Dec 2010 11:18:01 GMT;
    date[strlen(date) - 1] = '\0'; // remove \n
    response = StringUtils::Format("HTTP/1.1 {} {}\nDate: {}\r\n", status, statusMsg, date);
    if (!responseHeader.empty())
    {
      response += responseHeader;
    }

    response = StringUtils::Format("{}Content-Length: {}\r\n\r\n", response, responseBody.size());

    if (!responseBody.empty())
    {
      response += responseBody;
    }

    // Send the response
    //don't send response on AIRPLAY_STATUS_NO_RESPONSE_NEEDED
    if (status != AIRPLAY_STATUS_NO_RESPONSE_NEEDED)
    {
      send(m_socket, response.c_str(), response.size(), 0);
    }
    // We need a new parser...
    delete m_httpParser;
    m_httpParser = new HttpParser;
  }
}

void CAirPlayServer::CTCPClient::Disconnect()
{
  if (m_socket != INVALID_SOCKET)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = INVALID_SOCKET;
    delete m_httpParser;
    m_httpParser = NULL;
  }
}

void CAirPlayServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_httpParser        = client.m_httpParser;
  m_authNonce         = client.m_authNonce;
  m_bAuthenticated    = client.m_bAuthenticated;
  m_sessionCounter    = client.m_sessionCounter;
}


void CAirPlayServer::CTCPClient::ComposeReverseEvent( std::string& reverseHeader,
                                                      std::string& reverseBody,
                                                      int state)
{

  if ( m_lastEvent != state )
  {
    switch(state)
    {
      case EVENT_PLAYING:
      case EVENT_LOADING:
      case EVENT_PAUSED:
      case EVENT_STOPPED:
        reverseBody = StringUtils::Format(EVENT_INFO, m_sessionCounter, eventStrings[state]);
        CLog::Log(LOGDEBUG, "AIRPLAY: sending event: {}", eventStrings[state]);
        break;
    }
    reverseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
    reverseHeader =
        StringUtils::Format("{}Content-Length: {}\r\n", reverseHeader, reverseBody.size());
    reverseHeader = StringUtils::Format("{}x-apple-session-id: {}\r\n", reverseHeader.c_str(),
                                        m_sessionId.c_str());
    m_lastEvent = state;
  }
}

void CAirPlayServer::CTCPClient::ComposeAuthRequestAnswer(std::string& responseHeader, std::string& responseBody)
{
  int16_t random=rand();
  std::string randomStr = std::to_string(random);
  m_authNonce=CDigest::Calculate(CDigest::Type::MD5, randomStr);
  responseHeader = StringUtils::Format(AUTH_REQUIRED, m_authNonce);
  responseBody.clear();
}


//as of rfc 2617
std::string calcResponse(const std::string& username,
                        const std::string& password,
                        const std::string& realm,
                        const std::string& method,
                        const std::string& digestUri,
                        const std::string& nonce)
{
  std::string response;
  std::string HA1;
  std::string HA2;

  HA1 = CDigest::Calculate(CDigest::Type::MD5, username + ":" + realm + ":" + password);
  HA2 = CDigest::Calculate(CDigest::Type::MD5, method + ":" + digestUri);
  response = CDigest::Calculate(CDigest::Type::MD5, HA1 + ":" + nonce + ":" + HA2);
  return response;
}

//helper function
//from a string field1="value1", field2="value2" it parses the value to a field
std::string getFieldFromString(const std::string &str, const char* field)
{
  std::vector<std::string> tmpAr1 = StringUtils::Split(str, ",");
  for (const auto& i : tmpAr1)
  {
    if (i.find(field) != std::string::npos)
    {
      std::vector<std::string> tmpAr2 = StringUtils::Split(i, "=");
      if (tmpAr2.size() == 2)
      {
        StringUtils::Replace(tmpAr2[1], "\"", "");//remove quotes
        return tmpAr2[1];
      }
    }
  }
  return "";
}

bool CAirPlayServer::CTCPClient::checkAuthorization(const std::string& authStr,
                                                    const std::string& method,
                                                    const std::string& uri)
{
  bool authValid = true;

  std::string username;

  if (authStr.empty())
    return false;

  //first get username - we allow all usernames for airplay (usually it is AirPlay)
  username = getFieldFromString(authStr, "username");
  if (username.empty())
  {
    authValid = false;
  }

  //second check realm
  if (authValid)
  {
    if (getFieldFromString(authStr, "realm") != AUTH_REALM)
    {
      authValid = false;
    }
  }

  //third check nonce
  if (authValid)
  {
    if (getFieldFromString(authStr, "nonce") != m_authNonce)
    {
      authValid = false;
    }
  }

  //forth check uri
  if (authValid)
  {
    if (getFieldFromString(authStr, "uri") != uri)
    {
      authValid = false;
    }
  }

  //last check response
  if (authValid)
  {
     std::string realm = AUTH_REALM;
     std::string ourResponse = calcResponse(username, ServerInstance->m_password, realm, method, uri, m_authNonce);
     std::string theirResponse = getFieldFromString(authStr, "response");
     if (!StringUtils::EqualsNoCase(theirResponse, ourResponse))
     {
       authValid = false;
       CLog::Log(LOGDEBUG, "AirAuth: response mismatch - our: {} theirs: {}", ourResponse,
                 theirResponse);
     }
     else
     {
       CLog::Log(LOGDEBUG, "AirAuth: successful authentication from AirPlay client");
     }
  }
  m_bAuthenticated = authValid;
  return m_bAuthenticated;
}

void CAirPlayServer::backupVolume()
{
  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);

  if (ServerInstance && ServerInstance->m_origVolume == -1)
  {
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    ServerInstance->m_origVolume = static_cast<int>(appVolume->GetVolumePercent());
  }
}

void CAirPlayServer::restoreVolume()
{
  std::unique_lock<CCriticalSection> lock(ServerInstanceLock);

  const auto& settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (ServerInstance && ServerInstance->m_origVolume != -1 &&
      settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL))
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(static_cast<float>(ServerInstance->m_origVolume));
    ServerInstance->m_origVolume = -1;
  }
}

std::string getStringFromPlist(plist_t node)
{
  std::string ret;
  char *tmpStr = nullptr;
  plist_get_string_val(node, &tmpStr);
  ret = tmpStr;
  free(tmpStr);
  return ret;
}

int CAirPlayServer::CTCPClient::ProcessRequest( std::string& responseHeader,
                                                std::string& responseBody)
{
  std::string method = m_httpParser->getMethod() ? m_httpParser->getMethod() : "";
  std::string uri = m_httpParser->getUri() ? m_httpParser->getUri() : "";
  std::string queryString = m_httpParser->getQueryString() ? m_httpParser->getQueryString() : "";
  std::string body = m_httpParser->getBody() ? m_httpParser->getBody() : "";
  std::string contentType = m_httpParser->getValue("content-type") ? m_httpParser->getValue("content-type") : "";
  m_sessionId = m_httpParser->getValue("x-apple-session-id") ? m_httpParser->getValue("x-apple-session-id") : "";
  std::string authorization = m_httpParser->getValue("authorization") ? m_httpParser->getValue("authorization") : "";
  std::string photoAction = m_httpParser->getValue("x-apple-assetaction") ? m_httpParser->getValue("x-apple-assetaction") : "";
  std::string photoCacheId = m_httpParser->getValue("x-apple-assetkey") ? m_httpParser->getValue("x-apple-assetkey") : "";

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int status = AIRPLAY_STATUS_OK;
  bool needAuth = false;

  if (m_sessionId.empty())
    m_sessionId = "00000000-0000-0000-0000-000000000000";

  if (ServerInstance->m_usePassword && !m_bAuthenticated)
  {
    needAuth = true;
  }

  size_t startQs = uri.find('?');
  if (startQs != std::string::npos)
  {
    uri.erase(startQs);
  }

  // This is the socket which will be used for reverse HTTP
  // negotiate reverse HTTP via upgrade
  if (uri == "/reverse")
  {
    status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
    responseHeader = "Upgrade: PTTH/1.0\r\nConnection: Upgrade\r\n";
  }

  // The rate command is used to play/pause media.
  // A value argument should be supplied which indicates media should be played or paused.
  // 0.000000 => pause
  // 1.000000 => play
  else if (uri == "/rate")
  {
      const char* found = strstr(queryString.c_str(), "value=");
      int rate = found ? (int)(atof(found + strlen("value=")) + 0.5) : 0;

      CLog::Log(LOGDEBUG, "AIRPLAY: got request {} with rate {}", uri, rate);

      if (needAuth && !checkAuthorization(authorization, method, uri))
      {
        status = AIRPLAY_STATUS_NEED_AUTH;
      }
      else if (rate == 0)
      {
        if (appPlayer->IsPlaying() && !appPlayer->IsPaused())
        {
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
        }
      }
      else
      {
        if (appPlayer->IsPausedPlayback())
        {
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
        }
      }
  }

  // The volume command is used to change playback volume.
  // A value argument should be supplied which indicates how loud we should get.
  // 0.000000 => silent
  // 1.000000 => loud
  else if (uri == "/volume")
  {
      const char* found = strstr(queryString.c_str(), "volume=");
      float volume = found ? (float)strtod(found + strlen("volume="), NULL) : 0;

      CLog::Log(LOGDEBUG, "AIRPLAY: got request {} with volume {:f}", uri, volume);

      if (needAuth && !checkAuthorization(authorization, method, uri))
      {
        status = AIRPLAY_STATUS_NEED_AUTH;
      }
      else if (volume >= 0 && volume <= 1)
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
        float oldVolume = appVolume->GetVolumePercent();
        volume *= 100;
        const auto& settings = CServiceBroker::GetSettingsComponent()->GetSettings();
        if (oldVolume != volume &&
            settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL))
        {
          backupVolume();
          appVolume->SetVolume(volume);
          CServiceBroker::GetAppMessenger()->PostMsg(
              TMSG_VOLUME_SHOW, oldVolume < volume ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN);
        }
      }
  }


  // Contains a header like format in the request body which should contain a
  // Content-Location and optionally a Start-Position
  else if (uri == "/play")
  {
    std::string location;
    float position = 0.0;
    bool startPlayback = true;
    m_lastEvent = EVENT_NONE;

    CLog::Log(LOGDEBUG, "AIRPLAY: got request {}", uri);

    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (contentType == "application/x-apple-binary-plist")
    {
      CAirPlayServer::m_isPlaying++;

      const char* bodyChr = m_httpParser->getBody();

      plist_t dict = NULL;
      plist_from_bin(bodyChr, m_httpParser->getContentLength(), &dict);

      if (plist_dict_get_size(dict))
      {
        plist_t tmpNode = plist_dict_get_item(dict, "Start-Position");
        if (tmpNode)
        {
          double tmpDouble = 0;
          plist_get_real_val(tmpNode, &tmpDouble);
          position = (float)tmpDouble;
        }

        tmpNode = plist_dict_get_item(dict, "Content-Location");
        if (tmpNode)
        {
          location = getStringFromPlist(tmpNode);
          tmpNode = NULL;
        }

        tmpNode = plist_dict_get_item(dict, "rate");
        if (tmpNode)
        {
          double rate = 0;
          plist_get_real_val(tmpNode, &rate);
          if (rate == 0.0)
          {
            startPlayback = false;
          }
          tmpNode = NULL;
        }

        // in newer protocol versions the location is given
        // via host and path where host is ip:port and path is /path/file.mov
        if (location.empty())
          tmpNode = plist_dict_get_item(dict, "host");
        if (tmpNode)
        {
          location = "http://";
          location += getStringFromPlist(tmpNode);

          tmpNode = plist_dict_get_item(dict, "path");
          if (tmpNode)
          {
            location += getStringFromPlist(tmpNode);
          }
        }

        if (dict)
        {
          plist_free(dict);
        }
      }
      else
      {
        CLog::Log(LOGERROR, "Error parsing plist");
      }
    }
    else
    {
      CAirPlayServer::m_isPlaying++;
      // Get URL to play
      std::string contentLocation = "Content-Location: ";
      size_t start = body.find(contentLocation);
      if (start == std::string::npos)
        return AIRPLAY_STATUS_NOT_IMPLEMENTED;
      start += contentLocation.size();
      int end = body.find('\n', start);
      location = body.substr(start, end - start);

      std::string startPosition = "Start-Position: ";
      start = body.find(startPosition);
      if (start != std::string::npos)
      {
        start += startPosition.size();
        int end = body.find('\n', start);
        std::string positionStr = body.substr(start, end - start);
        position = (float)atof(positionStr.c_str());
      }
    }

    if (status != AIRPLAY_STATUS_NEED_AUTH)
    {
      std::string userAgent(CURL::Encode("AppleCoreMedia/1.0.0.8F455 (AppleTV; U; CPU OS 4_3 like Mac OS X; de_de)"));
      location += "|User-Agent=" + userAgent;

      CFileItem fileToPlay(location, false);
      fileToPlay.SetProperty("StartPercent", position*100.0f);
      ServerInstance->AnnounceToClients(EVENT_LOADING);

      CFileItemList *l = new CFileItemList; //don't delete,
      l->Add(std::make_shared<CFileItem>(fileToPlay));
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));

      // allow starting the player paused in ios8 mode (needed by camera roll app)
      if (!startPlayback)
      {
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
        appPlayer->SeekPercentage(position * 100.0f);
      }
    }
  }

  // Used to perform seeking (POST request) and to retrieve current player position (GET request).
  // GET scrub seems to also set rate 1 - strange but true
  else if (uri == "/scrub")
  {
    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (method == "GET")
    {
      CLog::Log(LOGDEBUG, "AIRPLAY: got GET request {}", uri);

      if (appPlayer->GetTotalTime())
      {
        float position = static_cast<float>(appPlayer->GetTime()) / 1000;
        responseBody =
            StringUtils::Format("duration: {:.6f}\r\nposition: {:.6f}\r\n",
                                static_cast<float>(appPlayer->GetTotalTime()) / 1000, position);
      }
      else
      {
        status = AIRPLAY_STATUS_METHOD_NOT_ALLOWED;
      }
    }
    else
    {
      const char* found = strstr(queryString.c_str(), "position=");

      if (found && appPlayer->HasPlayer())
      {
        int64_t position = (int64_t) (atof(found + strlen("position=")) * 1000.0);
        appPlayer->SeekTime(position);
        CLog::Log(LOGDEBUG, "AIRPLAY: got POST request {} with pos {}", uri, position);
      }
    }
  }

  // Sent when media playback should be stopped
  else if (uri == "/stop")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request {}", uri);
    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else
    {
      if (IsPlaying()) //only stop player if we started him
      {
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
        CAirPlayServer::m_isPlaying--;
      }
      else //if we are not playing and get the stop request - we just wanna stop picture streaming
      {
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
                                                   static_cast<void*>(new CAction(ACTION_STOP)));
      }
    }
    ClearPhotoAssetCache();
  }

  // RAW JPEG data is contained in the request body
  else if (uri == "/photo")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request {}", uri);
    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (m_httpParser->getContentLength() > 0 || photoAction == "displayCached")
    {
      XFILE::CFile tmpFile;
      std::string tmpFileName = "special://temp/airplayasset";
      bool showPhoto = true;
      bool receivePhoto = true;


      if (photoAction == "cacheOnly")
        showPhoto = false;
      else if (photoAction == "displayCached")
      {
        receivePhoto = false;
        if (photoCacheId.length())
          CLog::Log(LOGDEBUG, "AIRPLAY: Trying to show from cache asset: {}", photoCacheId);
      }

      if (photoCacheId.length())
        tmpFileName += photoCacheId;
      else
        tmpFileName += "airplay_photo";

      if( receivePhoto && m_httpParser->getContentLength() > 3 &&
          m_httpParser->getBody()[1] == 'P' &&
          m_httpParser->getBody()[2] == 'N' &&
          m_httpParser->getBody()[3] == 'G')
      {
        tmpFileName += ".png";
      }
      else
      {
        tmpFileName += ".jpg";
      }

      int writtenBytes=0;
      if (receivePhoto)
      {
        if (tmpFile.OpenForWrite(tmpFileName, true))
        {
          writtenBytes = tmpFile.Write(m_httpParser->getBody(), m_httpParser->getContentLength());
          tmpFile.Close();
        }
        if (photoCacheId.length())
          CLog::Log(LOGDEBUG, "AIRPLAY: Cached asset: {}", photoCacheId);
      }

      if (showPhoto)
      {
        if ((writtenBytes > 0 && (unsigned int)writtenBytes == m_httpParser->getContentLength()) || !receivePhoto)
        {
          if (!receivePhoto && !XFILE::CFile::Exists(tmpFileName))
          {
            status = AIRPLAY_STATUS_PRECONDITION_FAILED; //image not found in the cache
            if (photoCacheId.length())
              CLog::Log(LOGWARNING, "AIRPLAY: Asset {} not found in our cache.", photoCacheId);
          }
          else
            CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PICTURE_SHOW, -1, -1, nullptr,
                                                       tmpFileName);
        }
        else
        {
          CLog::Log(LOGERROR,"AirPlayServer: Error writing tmpFile.");
        }
      }
    }
  }

  else if (uri == "/playback-info")
  {
    float position = 0.0f;
    float duration = 0.0f;
    float cachePosition = 0.0f;
    bool playing = false;

    CLog::Log(LOGDEBUG, "AIRPLAY: got request {}", uri);

    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (appPlayer->HasPlayer())
    {
      if (appPlayer->GetTotalTime())
      {
        position = static_cast<float>(appPlayer->GetTime()) / 1000;
        duration = static_cast<float>(appPlayer->GetTotalTime()) / 1000;
        playing = !appPlayer->IsPaused();
        cachePosition = position + (duration * appPlayer->GetCachePercentage() / 100.0f);
      }

      responseBody = StringUtils::Format(PLAYBACK_INFO, duration, cachePosition, position, (playing ? 1 : 0), duration);
      responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";

      if (appPlayer->IsCaching())
      {
        CAirPlayServer::ServerInstance->AnnounceToClients(EVENT_LOADING);
      }
    }
    else
    {
      responseBody = StringUtils::Format(PLAYBACK_INFO_NOT_READY);
      responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
    }
  }

  else if (uri == "/server-info")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request {}", uri);
    responseBody = StringUtils::Format(
        SERVER_INFO, CServiceBroker::GetNetwork().GetFirstConnectedInterface()->GetMacAddress());
    responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
  }

  else if (uri == "/slideshow-features")
  {
    // Ignore for now.
  }

  else if (uri == "/authorize")
  {
    // DRM, ignore for now.
  }

  else if (uri == "/setProperty")
  {
    status = AIRPLAY_STATUS_NOT_FOUND;
  }

  else if (uri == "/getProperty")
  {
    status = AIRPLAY_STATUS_NOT_FOUND;
  }

  else if (uri == "/fp-setup")
  {
    status = AIRPLAY_STATUS_PRECONDITION_FAILED;
  }

  else if (uri == "200") //response OK from the event reverse message
  {
    status = AIRPLAY_STATUS_NO_RESPONSE_NEEDED;
  }
  else
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: unhandled request [{}]", uri);
    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
  }

  if (status == AIRPLAY_STATUS_NEED_AUTH)
  {
    ComposeAuthRequestAnswer(responseHeader, responseBody);
  }

  return status;
}
