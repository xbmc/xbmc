/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

CWebSocket* CWebSocketManager::Handle(const char* data, unsigned int length, std::string &response)
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
    response = response = httpResponse.Create();;

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
    response = response = httpResponse.Create();

    return NULL;
  }

  if (websocket->Handshake(data, length, response))
    return websocket;

  return NULL;
}
