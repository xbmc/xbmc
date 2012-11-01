/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <string>

#include "WebSocketManager.h"
#include "WebSocket.h"
#include "WebSocketV8.h"
#include "WebSocketV13.h"
#include "utils/HttpParser.h"
#include "utils/HttpResponse.h"
#include "utils/log.h"

#define WS_HTTP_METHOD          "GET"
#define WS_HTTP_TAG             "HTTP/"
#define WS_SUPPORTED_VERSIONS   "8, 13"

#define WS_HEADER_VERSION       "Sec-WebSocket-Version"
#define WS_HEADER_VERSION_LC    "sec-websocket-version"     // "Sec-WebSocket-Version"

using namespace std;

CWebSocket* CWebSocketManager::Handle(const char* data, unsigned int length, string &response)
{
  if (data == NULL || length <= 0)
    return NULL;

  HttpParser header;
  HttpParser::status_t status = header.addBytes(data, length);
  switch (status)
  {
    case HttpParser::Error:
    case HttpParser::Incomplete:
      response.clear();
      return NULL;

    case HttpParser::Done:
    default:
      break;
  }

  // There must be a "Sec-WebSocket-Version" header
  const char* value = header.getValue(WS_HEADER_VERSION_LC);
  if (value == NULL)
  {
    CLog::Log(LOGINFO, "WebSocket: missing Sec-WebSocket-Version");
    CHttpResponse httpResponse(HTTP::Get, HTTP::BadRequest, HTTP::Version1_1);
    char *responseBuffer;
    int responseLength = httpResponse.Create(responseBuffer);
    response = std::string(responseBuffer, responseLength);

    return NULL;
  }
  
  CWebSocket *websocket = NULL;
  if (strncmp(value, "8", 1) == 0)
    websocket = new CWebSocketV8();
  else if (strncmp(value, "13", 2) == 0)
    websocket = new CWebSocketV13();

  if (websocket == NULL)
  {
    CLog::Log(LOGINFO, "WebSocket: Unsupported Sec-WebSocket-Version %s", value);
    CHttpResponse httpResponse(HTTP::Get, HTTP::UpgradeRequired, HTTP::Version1_1);
    httpResponse.AddHeader(WS_HEADER_VERSION, WS_SUPPORTED_VERSIONS);
    char *responseBuffer;
    int responseLength = httpResponse.Create(responseBuffer);
    response = std::string(responseBuffer, responseLength);

    return NULL;
  }

  if (websocket->Handshake(data, length, response))
    return websocket;

  return NULL;
}
