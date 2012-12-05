/*
 * Many concepts and protocol specification in this code are taken
 * from Airplayer. https://github.com/PascalW/Airplayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "AirPlayServer.h"

#ifdef HAS_AIRPLAY

#include <netinet/in.h>
#include <arpa/inet.h>
#include "DllLibPlist.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "utils/md5.h"
#include "utils/Variant.h"
#include "guilib/GUIWindowManager.h"
#include "URL.h"

#ifdef TARGET_WINDOWS
#define close closesocket
#endif

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

CAirPlayServer *CAirPlayServer::ServerInstance = NULL;
int CAirPlayServer::m_isPlaying = 0;

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
const char *eventStrings[] = {"playing", "paused", "loading", "stopped"};

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>duration</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>loadedTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"<key>playbackBufferEmpty</key>\r\n"\
"<true/>\r\n"\
"<key>playbackBufferFull</key>\r\n"\
"<false/>\r\n"\
"<key>playbackLikelyToKeepUp</key>\r\n"\
"<true/>\r\n"\
"<key>position</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>rate</key>\r\n"\
"<real>%d</real>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<true/>\r\n"\
"<key>seekableTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>deviceid</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>features</key>\r\n"\
"<integer>119</integer>\r\n"\
"<key>model</key>\r\n"\
"<string>AppleTV2,1</string>\r\n"\
"<key>protovers</key>\r\n"\
"<string>1.0</string>\r\n"\
"<key>srcvers</key>\r\n"\
"<string>"AIRPLAY_SERVER_VERSION_STR"</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>category</key>\r\n"\
"<string>video</string>\r\n"\
"<key>state</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

bool CAirPlayServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  ServerInstance = new CAirPlayServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
    return true;
  }
  else
    return false;
}

bool CAirPlayServer::SetCredentials(bool usePassword, const CStdString& password)
{
  bool ret = false;

  if (ServerInstance)
  {
    ret = ServerInstance->SetInternalCredentials(usePassword, password);
  }
  return ret;
}

bool CAirPlayServer::SetInternalCredentials(bool usePassword, const CStdString& password)
{
  m_usePassword = usePassword;
  m_password = password;
  return true;
}

void CAirPlayServer::StopServer(bool bWait)
{
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

CAirPlayServer::CAirPlayServer(int port, bool nonlocal) : CThread("AirPlayServer")
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_ServerSocket = INVALID_SOCKET;
  m_usePassword = false;
}

void CAirPlayServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    int             max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    FD_SET(m_ServerSocket, &rfds);
    max_fd = m_ServerSocket;

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
      Sleep(1000);
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
            CStdString sessionId;
            m_connections[i].PushBuffer(this, buffer, nread, sessionId, m_reverseSockets);
          }
          if (nread <= 0)
          {
            CLog::Log(LOGINFO, "AIRPLAY Server: Disconnection detected");
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      if (FD_ISSET(m_ServerSocket, &rfds))
      {
        CLog::Log(LOGDEBUG, "AIRPLAY Server: New connection detected");
        CTCPClient newconnection;
        newconnection.m_socket = accept(m_ServerSocket, &newconnection.m_cliaddr, &newconnection.m_addrlen);

        if (newconnection.m_socket == INVALID_SOCKET)
          CLog::Log(LOGERROR, "AIRPLAY Server: Accept of new connection failed");
        else
        {
          CLog::Log(LOGINFO, "AIRPLAY Server: New connection added");
          m_connections.push_back(newconnection);
        }
      }
    }
  }

  Deinitialize();
}

bool CAirPlayServer::Initialize()
{
  Deinitialize();

  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(m_port);

  if (m_nonlocal)
    myaddr.sin_addr.s_addr = INADDR_ANY;
  else
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr.s_addr);

  m_ServerSocket = socket(PF_INET, SOCK_STREAM, 0);

  if (m_ServerSocket == INVALID_SOCKET)
  {

    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to create serversocket");
    return false;
  }

  if (bind(m_ServerSocket, (struct sockaddr*)&myaddr, sizeof myaddr) < 0)
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to bind serversocket");
    close(m_ServerSocket);
    return false;
  }

  if (listen(m_ServerSocket, 10) < 0)
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to set listen");
    close(m_ServerSocket);
    return false;
  }

  CLog::Log(LOGINFO, "AIRPLAY Server: Successfully initialized");
  return true;
}

void CAirPlayServer::Deinitialize()
{
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i].Disconnect();

  m_connections.clear();
  m_reverseSockets.clear();

  if (m_ServerSocket != INVALID_SOCKET)
  {
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_ServerSocket = INVALID_SOCKET;
  }
}

CAirPlayServer::CTCPClient::CTCPClient()
{
  m_socket = INVALID_SOCKET;
  m_httpParser = new HttpParser();

  m_addrlen = sizeof(struct sockaddr);
  m_pLibPlist = new DllLibPlist();

  m_bAuthenticated = false;
  m_lastEvent = EVENT_NONE;
}

CAirPlayServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
  m_httpParser = new HttpParser();
  m_pLibPlist = new DllLibPlist();
}

CAirPlayServer::CTCPClient::~CTCPClient()
{
  if (m_pLibPlist->IsLoaded())
  {
    m_pLibPlist->Unload();
  }
  delete m_pLibPlist;
  delete m_httpParser;
}

CAirPlayServer::CTCPClient& CAirPlayServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  m_httpParser = new HttpParser();
  m_pLibPlist = new DllLibPlist();
  return *this;
}

void CAirPlayServer::CTCPClient::PushBuffer(CAirPlayServer *host, const char *buffer,
                                            int length, CStdString &sessionId, std::map<CStdString,
                                            int> &reverseSockets)
{
  HttpParser::status_t status = m_httpParser->addBytes(buffer, length);

  if (status == HttpParser::Done)
  {
    // Parse the request
    CStdString responseHeader;
    CStdString responseBody;
    CStdString reverseHeader;
    CStdString reverseBody;
    int status = ProcessRequest(responseHeader, responseBody, reverseHeader, reverseBody, sessionId);
    CStdString statusMsg = "OK";
    int reverseSocket = INVALID_SOCKET;

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
    }

    // Prepare the response
    CStdString response;
    const time_t ltime = time(NULL);
    char *date = asctime(gmtime(&ltime)); //Fri, 17 Dec 2010 11:18:01 GMT;
    date[strlen(date) - 1] = '\0'; // remove \n
    response.Format("HTTP/1.1 %d %s\nDate: %s\r\n", status, statusMsg.c_str(), date);
    if (responseHeader.size() > 0)
    {
      response += responseHeader;
    }

    if (responseBody.size() > 0)
    {
      response.Format("%sContent-Length: %d\r\n", response.c_str(), responseBody.size());
    }
    response += "\r\n";

    if (responseBody.size() > 0)
    {
      response += responseBody;
    }

    // Send the response
    //don't send response on AIRPLAY_STATUS_NO_RESPONSE_NEEDED
    if (status != AIRPLAY_STATUS_NO_RESPONSE_NEEDED)
    {
      send(m_socket, response.c_str(), response.size(), 0);
    }

    // Send event status per reverse http socket (play, loading, paused)
    // if we have a reverse header and a reverse socket
    if (reverseHeader.size() > 0 && reverseSockets.find(sessionId) != reverseSockets.end())
    {
      //search the reverse socket to this sessionid
      response.Format("POST /event HTTP/1.1\r\n");
      reverseSocket = reverseSockets[sessionId]; //that is our reverse socket
      response += reverseHeader;
    }
    response += "\r\n";

    if (reverseBody.size() > 0)
    {
      response += reverseBody;
    }

    if (reverseSocket != INVALID_SOCKET)
    {
      send(reverseSocket, response.c_str(), response.size(), 0);//send the event status on the eventSocket
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
    CSingleLock lock (m_critSection);
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
}


void CAirPlayServer::CTCPClient::ComposeReverseEvent( CStdString& reverseHeader,
                                                      CStdString& reverseBody,
                                                      CStdString sessionId,
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
        reverseBody.Format(EVENT_INFO, eventStrings[state]);
        CLog::Log(LOGDEBUG, "AIRPLAY: sending event: %s", eventStrings[state]);
        break;
    }
    reverseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
    reverseHeader.Format("%sContent-Length: %d",reverseHeader.c_str(),reverseBody.size());
    reverseHeader.Format("%sx-apple-session-id: %s\r\n",reverseHeader.c_str(),sessionId.c_str());
    m_lastEvent = state;
  }
}

void CAirPlayServer::CTCPClient::ComposeAuthRequestAnswer(CStdString& responseHeader, CStdString& responseBody)
{
  CStdString randomStr;
  int16_t random=rand();
  randomStr.Format("%i", random);
  m_authNonce=XBMC::XBMC_MD5::GetMD5(randomStr);
  responseHeader.Format(AUTH_REQUIRED,m_authNonce);
  responseBody.clear();
}


//as of rfc 2617
CStdString calcResponse(const CStdString& username,
                        const CStdString& password,
                        const CStdString& realm,
                        const CStdString& method,
                        const CStdString& digestUri,
                        const CStdString& nonce)
{
  CStdString response;
  CStdString HA1;
  CStdString HA2;

  HA1 = XBMC::XBMC_MD5::GetMD5(username + ":" + realm + ":" + password);
  HA2 = XBMC::XBMC_MD5::GetMD5(method + ":" + digestUri);
  response = XBMC::XBMC_MD5::GetMD5(HA1.ToLower() + ":" + nonce + ":" + HA2.ToLower());
  return response.ToLower();
}

//helper function
//from a string field1="value1", field2="value2" it parses the value to a field
CStdString getFieldFromString(const CStdString &str, const char* field)
{
  CStdString tmpStr;
  CStdStringArray tmpAr1;
  CStdStringArray tmpAr2;

  StringUtils::SplitString(str, ",", tmpAr1);

  for(unsigned int i = 0;i<tmpAr1.size();i++)
  {
    if (tmpAr1[i].Find(field) != -1)
    {
      if (StringUtils::SplitString(tmpAr1[i], "=", tmpAr2) == 2)
      {
        tmpAr2[1].Remove('\"');//remove quotes
        return tmpAr2[1];
      }
    }
  }
  return "";
}

bool CAirPlayServer::CTCPClient::checkAuthorization(const CStdString& authStr,
                                                    const CStdString& method,
                                                    const CStdString& uri)
{
  bool authValid = true;

  CStdString username;

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
     CStdString realm = AUTH_REALM;
     CStdString ourResponse = calcResponse(username, ServerInstance->m_password, realm, method, uri, m_authNonce);
     CStdString theirResponse = getFieldFromString(authStr, "response");
     if (!theirResponse.Equals(ourResponse, false))
     {
       authValid = false;
       CLog::Log(LOGDEBUG,"AirAuth: response mismatch - our: %s theirs: %s",ourResponse.c_str(), theirResponse.c_str());
     }
     else
     {
       CLog::Log(LOGDEBUG, "AirAuth: successfull authentication from AirPlay client");
     }
  }
  m_bAuthenticated = authValid;
  return m_bAuthenticated;
}

int CAirPlayServer::CTCPClient::ProcessRequest( CStdString& responseHeader,
                                                CStdString& responseBody,
                                                CStdString& reverseHeader,
                                                CStdString& reverseBody,
                                                CStdString& sessionId)
{
  CStdString method = m_httpParser->getMethod();
  CStdString uri = m_httpParser->getUri();
  CStdString queryString = m_httpParser->getQueryString();
  CStdString body = m_httpParser->getBody();
  CStdString contentType = m_httpParser->getValue("content-type");
  sessionId = m_httpParser->getValue("x-apple-session-id");
  CStdString authorization = m_httpParser->getValue("authorization");
  int status = AIRPLAY_STATUS_OK;
  bool needAuth = false;

  if (ServerInstance->m_usePassword && !m_bAuthenticated)
  {
    needAuth = true;
  }

  int startQs = uri.Find('?');
  if (startQs != -1)
  {
    uri = uri.Left(startQs);
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
      int rate = found ? (int)(atof(found + strlen("value=")) + 0.5f) : 0;

      CLog::Log(LOGDEBUG, "AIRPLAY: got request %s with rate %i", uri.c_str(), rate);

      if (needAuth && !checkAuthorization(authorization, method, uri))
      {
        status = AIRPLAY_STATUS_NEED_AUTH;
      }
      else if (rate == 0)
      {
        if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
        {
          CApplicationMessenger::Get().MediaPause();
          ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_PAUSED);
        }
      }
      else
      {
        if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->IsPaused())
        {
          CApplicationMessenger::Get().MediaPause();
          ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_PLAYING);
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

      CLog::Log(LOGDEBUG, "AIRPLAY: got request %s with volume %f", uri.c_str(), volume);

      if (needAuth && !checkAuthorization(authorization, method, uri))
      {
        status = AIRPLAY_STATUS_NEED_AUTH;
      }
      else if (volume >= 0 && volume <= 1)
      {
        int oldVolume = g_application.GetVolume();
        volume *= 100;
        if(oldVolume != (int)volume)
        {
          g_application.SetVolume(volume);          
          CApplicationMessenger::Get().ShowVolumeBar(oldVolume < volume);
        }
      }
  }


  // Contains a header like format in the request body which should contain a
  // Content-Location and optionally a Start-Position
  else if (uri == "/play")
  {
    CStdString location;
    float position = 0.0;
    m_lastEvent = EVENT_NONE;

    CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());

    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (contentType == "application/x-apple-binary-plist")
    {
      CAirPlayServer::m_isPlaying++;    
      
      if (m_pLibPlist->Load())
      {
        m_pLibPlist->EnableDelayedUnload(false);

        const char* bodyChr = m_httpParser->getBody();

        plist_t dict = NULL;
        m_pLibPlist->plist_from_bin(bodyChr, m_httpParser->getContentLength(), &dict);

        if (m_pLibPlist->plist_dict_get_size(dict))
        {
          plist_t tmpNode = m_pLibPlist->plist_dict_get_item(dict, "Start-Position");
          if (tmpNode)
          {
            double tmpDouble = 0;
            m_pLibPlist->plist_get_real_val(tmpNode, &tmpDouble);
            position = (float)tmpDouble;
          }

          tmpNode = m_pLibPlist->plist_dict_get_item(dict, "Content-Location");
          if (tmpNode)
          {
            char *tmpStr = NULL;
            m_pLibPlist->plist_get_string_val(tmpNode, &tmpStr);
            location=tmpStr;
#ifdef TARGET_WINDOWS
            m_pLibPlist->plist_free_string_val(tmpStr);
#else
            free(tmpStr);
#endif
          }

          if (dict)
          {
            m_pLibPlist->plist_free(dict);
          }
        }
        else
        {
          CLog::Log(LOGERROR, "Error parsing plist");
        }
        m_pLibPlist->Unload();
      }
    }
    else
    {
      CAirPlayServer::m_isPlaying++;        
      // Get URL to play
      int start = body.Find("Content-Location: ");
      if (start == -1)
        return AIRPLAY_STATUS_NOT_IMPLEMENTED;
      start += strlen("Content-Location: ");
      int end = body.Find('\n', start);
      location = body.Mid(start, end - start);

      start = body.Find("Start-Position");
      if (start != -1)
      {
        start += strlen("Start-Position: ");
        int end = body.Find('\n', start);
        CStdString positionStr = body.Mid(start, end - start);
        position = (float)atof(positionStr.c_str());
      }
    }

    if (status != AIRPLAY_STATUS_NEED_AUTH)
    {
      CStdString userAgent="AppleCoreMedia/1.0.0.8F455 (AppleTV; U; CPU OS 4_3 like Mac OS X; de_de)";
      CURL::Encode(userAgent);
      location += "|User-Agent=" + userAgent;

      CFileItem fileToPlay(location, false);
      fileToPlay.SetProperty("StartPercent", position*100.0f);
      CApplicationMessenger::Get().MediaPlay(fileToPlay);
      ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_PLAYING);
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
      CLog::Log(LOGDEBUG, "AIRPLAY: got GET request %s", uri.c_str());
      
      if (g_application.m_pPlayer && g_application.m_pPlayer->GetTotalTime())
      {
        float position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
        responseBody.Format("duration: %d\r\nposition: %f", g_application.m_pPlayer->GetTotalTime() / 1000, position);
      }
      else 
      {
        status = AIRPLAY_STATUS_METHOD_NOT_ALLOWED;
      }
    }
    else
    {
      const char* found = strstr(queryString.c_str(), "position=");
      
      if (found && g_application.m_pPlayer)
      {
        int64_t position = (int64_t) (atof(found + strlen("position=")) * 1000.0);
        g_application.m_pPlayer->SeekTime(position);
        CLog::Log(LOGDEBUG, "AIRPLAY: got POST request %s with pos %"PRId64, uri.c_str(), position);
      }
    }
  }

  // Sent when media playback should be stopped
  else if (uri == "/stop")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else
    {
      if (IsPlaying()) //only stop player if we started him
      {
        CApplicationMessenger::Get().MediaStop();
        CAirPlayServer::m_isPlaying--;
      }
      else //if we are not playing and get the stop request - we just wanna stop picture streaming
      {
        g_windowManager.PreviousWindow();
      }
      ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_STOPPED);
    }
  }

  // RAW JPEG data is contained in the request body
  else if (uri == "/photo")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (m_httpParser->getContentLength() > 0)
    {
      XFILE::CFile tmpFile;
      CStdString tmpFileName = "special://temp/airplay_photo.jpg";

      if( m_httpParser->getContentLength() > 3 &&
          m_httpParser->getBody()[1] == 'P' &&
          m_httpParser->getBody()[2] == 'N' &&
          m_httpParser->getBody()[3] == 'G')
      {
        tmpFileName = "special://temp/airplay_photo.png";
      }

      if (tmpFile.OpenForWrite(tmpFileName, true))
      {
        int writtenBytes=0;
        writtenBytes = tmpFile.Write(m_httpParser->getBody(), m_httpParser->getContentLength());
        tmpFile.Close();

        if (writtenBytes > 0 && (unsigned int)writtenBytes == m_httpParser->getContentLength())
        {
          CApplicationMessenger::Get().PictureShow(tmpFileName);
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

    CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());

    if (needAuth && !checkAuthorization(authorization, method, uri))
    {
      status = AIRPLAY_STATUS_NEED_AUTH;
    }
    else if (g_application.m_pPlayer)
    {
      if (g_application.m_pPlayer->GetTotalTime())
      {
        position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
        duration = ((float) g_application.m_pPlayer->GetTotalTime()) / 1000;
        playing = g_application.m_pPlayer ? !g_application.m_pPlayer->IsPaused() : false;
        cachePosition = position + (duration * g_application.m_pPlayer->GetCachePercentage() / 100.0f);
      }

      responseBody.Format(PLAYBACK_INFO, duration, cachePosition, position, (playing ? 1 : 0), duration);
      responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";

      if (g_application.m_pPlayer->IsCaching())
      {
        ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_LOADING);
      }
      else if (playing)
      {
        ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_PLAYING);
      }
      else
      {
        ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_PAUSED);
      }
    }
    else
    {
      responseBody.Format(PLAYBACK_INFO_NOT_READY, duration, cachePosition, position, (playing ? 1 : 0), duration);
      responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";     
      ComposeReverseEvent(reverseHeader, reverseBody, sessionId, EVENT_STOPPED);
    }
  }

  else if (uri == "/server-info")
  {
    CLog::Log(LOGDEBUG, "AIRPLAY: got request %s", uri.c_str());
    responseBody.Format(SERVER_INFO, g_application.getNetwork().GetFirstConnectedInterface()->GetMacAddress());
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

  else if (uri == "200") //response OK from the event reverse message
  {
    status = AIRPLAY_STATUS_NO_RESPONSE_NEEDED;
  }
  else
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: unhandled request [%s]\n", uri.c_str());
    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
  }

  if (status == AIRPLAY_STATUS_NEED_AUTH)
  {
    ComposeAuthRequestAnswer(responseHeader, responseBody);
  }

  return status;
}

#endif
