/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebSocketV13.h"

#include "WebSocket.h"
#include "utils/HttpParser.h"
#include "utils/HttpResponse.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <sstream>
#include <string>

#define WS_HTTP_METHOD          "GET"
#define WS_HTTP_TAG             "HTTP/"

#define WS_HEADER_UPGRADE       "Upgrade"
#define WS_HEADER_UPGRADE_LC    "upgrade"
#define WS_HEADER_CONNECTION    "Connection"
#define WS_HEADER_CONNECTION_LC "connection"

#define WS_HEADER_KEY_LC        "sec-websocket-key"         // "Sec-WebSocket-Key"
#define WS_HEADER_ACCEPT        "Sec-WebSocket-Accept"
#define WS_HEADER_PROTOCOL      "Sec-WebSocket-Protocol"
#define WS_HEADER_PROTOCOL_LC   "sec-websocket-protocol"    // "Sec-WebSocket-Protocol"

#define WS_PROTOCOL_JSONRPC     "jsonrpc.xbmc.org"
#define WS_HEADER_UPGRADE_VALUE "websocket"

bool CWebSocketV13::Handshake(const char* data, size_t length, std::string &response)
{
  std::string strHeader(data, length);
  const char *value;
  HttpParser header;
  if (header.addBytes(data, length) != HttpParser::Done)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: incomplete handshake received");
    return false;
  }

  // The request must be GET
  value = header.getMethod();
  if (value == NULL || strnicmp(value, WS_HTTP_METHOD, strlen(WS_HTTP_METHOD)) != 0)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid HTTP method received (GET expected)");
    return false;
  }

  // The request must be HTTP/1.1 or higher
  size_t pos;
  if ((pos = strHeader.find(WS_HTTP_TAG)) == std::string::npos)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid handshake received");
    return false;
  }

  pos += strlen(WS_HTTP_TAG);
  std::istringstream converter(strHeader.substr(pos, strHeader.find_first_of(" \r\n\t", pos) - pos));
  float fVersion;
  converter >> fVersion;

  if (fVersion < 1.1f)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid HTTP version %f (1.1 or higher expected)", fVersion);
    return false;
  }

  std::string websocketKey, websocketProtocol;
  // There must be a "Host" header
  value = header.getValue("host");
  if (value == NULL || strlen(value) == 0)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: \"Host\" header missing");
    return true;
  }

  // There must be a "Upgrade" header with the value "websocket"
  value = header.getValue(WS_HEADER_UPGRADE_LC);
  if (value == NULL || strnicmp(value, WS_HEADER_UPGRADE_VALUE, strlen(WS_HEADER_UPGRADE_VALUE)) != 0)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid \"%s\" received", WS_HEADER_UPGRADE);
    return true;
  }

  // There must be a "Connection" header with the value "Upgrade"
  value = header.getValue(WS_HEADER_CONNECTION_LC);
  std::vector<std::string> elements;
  if (value != nullptr)
    elements = StringUtils::Split(value, ",");
  if (elements.empty() || !std::any_of(elements.begin(), elements.end(), [](std::string& elem) { return StringUtils::EqualsNoCase(StringUtils::Trim(elem), WS_HEADER_UPGRADE); }))
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid \"%s\" received", WS_HEADER_CONNECTION_LC);
    return true;
  }

  // There must be a base64 encoded 16 byte (=> 24 byte as base62) "Sec-WebSocket-Key" header
  value = header.getValue(WS_HEADER_KEY_LC);
  if (value == NULL || (websocketKey = value).size() != 24)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: invalid \"Sec-WebSocket-Key\" received");
    return true;
  }

  // There might be a "Sec-WebSocket-Protocol" header
  value = header.getValue(WS_HEADER_PROTOCOL_LC);
  if (value && strlen(value) > 0)
  {
    std::vector<std::string> protocols = StringUtils::Split(value, ",");
    for (auto& protocol : protocols)
    {
      StringUtils::Trim(protocol);
      if (protocol == WS_PROTOCOL_JSONRPC)
      {
        websocketProtocol = WS_PROTOCOL_JSONRPC;
        break;
      }
    }
  }

  CHttpResponse httpResponse(HTTP::Get, HTTP::SwitchingProtocols, HTTP::Version1_1);
  httpResponse.AddHeader(WS_HEADER_UPGRADE, WS_HEADER_UPGRADE_VALUE);
  httpResponse.AddHeader(WS_HEADER_CONNECTION, WS_HEADER_UPGRADE);
  std::string responseKey = calculateKey(websocketKey);
  httpResponse.AddHeader(WS_HEADER_ACCEPT, responseKey);
  if (!websocketProtocol.empty())
    httpResponse.AddHeader(WS_HEADER_PROTOCOL, websocketProtocol);

  response = httpResponse.Create();

  m_state = WebSocketStateConnected;

  return true;
}

const CWebSocketFrame* CWebSocketV13::Close(WebSocketCloseReason reason /* = WebSocketCloseNormal */, const std::string &message /* = "" */)
{
  if (m_state == WebSocketStateNotConnected || m_state == WebSocketStateHandshaking || m_state == WebSocketStateClosed)
  {
    CLog::Log(LOGINFO, "WebSocket [RFC6455]: Cannot send a closing handshake if no connection has been established");
    return NULL;
  }

  return close(reason, message);
}
