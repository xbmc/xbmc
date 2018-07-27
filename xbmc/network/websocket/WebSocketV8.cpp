/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <string>
#include <sstream>

#include "WebSocketV8.h"
#include "WebSocket.h"
#include "utils/Base64.h"
#include "utils/Digest.h"
#include "utils/EndianSwap.h"
#include "utils/HttpParser.h"
#include "utils/HttpResponse.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using KODI::UTILITY::CDigest;

#define WS_HTTP_METHOD          "GET"
#define WS_HTTP_TAG             "HTTP/"

#define WS_HEADER_UPGRADE       "Upgrade"
#define WS_HEADER_CONNECTION    "Connection"

#define WS_HEADER_KEY_LC        "sec-websocket-key"         // "Sec-WebSocket-Key"
#define WS_HEADER_ACCEPT        "Sec-WebSocket-Accept"
#define WS_HEADER_PROTOCOL      "Sec-WebSocket-Protocol"
#define WS_HEADER_PROTOCOL_LC   "sec-websocket-protocol"    // "Sec-WebSocket-Protocol"

#define WS_PROTOCOL_JSONRPC     "jsonrpc.xbmc.org"
#define WS_HEADER_UPGRADE_VALUE "websocket"
#define WS_KEY_MAGICSTRING      "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

bool CWebSocketV8::Handshake(const char* data, size_t length, std::string &response)
{
  std::string strHeader(data, length);
  const char *value;
  HttpParser header;
  if (header.addBytes(data, length) != HttpParser::Done)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: incomplete handshake received");
    return false;
  }

  // The request must be GET
  value = header.getMethod();
  if (value == NULL || strnicmp(value, WS_HTTP_METHOD, strlen(WS_HTTP_METHOD)) != 0)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid HTTP method received (GET expected)");
    return false;
  }

  // The request must be HTTP/1.1 or higher
  size_t pos;
  if ((pos = strHeader.find(WS_HTTP_TAG)) == std::string::npos)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid handshake received");
    return false;
  }

  pos += strlen(WS_HTTP_TAG);
  std::istringstream converter(strHeader.substr(pos, strHeader.find_first_of(" \r\n\t", pos) - pos));
  float fVersion;
  converter >> fVersion;

  if (fVersion < 1.1f)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid HTTP version %f (1.1 or higher expected)", fVersion);
    return false;
  }

  std::string websocketKey, websocketProtocol;
  // There must be a "Host" header
  value = header.getValue("host");
  if (value == NULL || strlen(value) == 0)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: \"Host\" header missing");
    return true;
  }

  // There must be a base64 encoded 16 byte (=> 24 byte as base64) "Sec-WebSocket-Key" header
  value = header.getValue(WS_HEADER_KEY_LC);
  if (value == NULL || (websocketKey = value).size() != 24)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid \"Sec-WebSocket-Key\" received");
    return true;
  }

  // There might be a "Sec-WebSocket-Protocol" header
  value = header.getValue(WS_HEADER_PROTOCOL_LC);
  if (value && strlen(value) > 0)
  {
    std::vector<std::string> protocols = StringUtils::Split(value, ",");
    for (std::vector<std::string>::iterator protocol = protocols.begin(); protocol != protocols.end(); ++protocol)
    {
      StringUtils::Trim(*protocol);
      if (*protocol == WS_PROTOCOL_JSONRPC)
      {
        websocketProtocol = WS_PROTOCOL_JSONRPC;
        break;
      }
    }
  }

  CHttpResponse httpResponse(HTTP::Get, HTTP::SwitchingProtocols, HTTP::Version1_1);
  httpResponse.AddHeader(WS_HEADER_UPGRADE, WS_HEADER_UPGRADE_VALUE);
  httpResponse.AddHeader(WS_HEADER_CONNECTION, WS_HEADER_UPGRADE);
  httpResponse.AddHeader(WS_HEADER_ACCEPT, calculateKey(websocketKey));
  if (!websocketProtocol.empty())
    httpResponse.AddHeader(WS_HEADER_PROTOCOL, websocketProtocol);

  response = response = httpResponse.Create();

  m_state = WebSocketStateConnected;

  return true;
}

const CWebSocketFrame* CWebSocketV8::Close(WebSocketCloseReason reason /* = WebSocketCloseNormal */, const std::string &message /* = "" */)
{
  if (m_state == WebSocketStateNotConnected || m_state == WebSocketStateHandshaking || m_state == WebSocketStateClosed)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: Cannot send a closing handshake if no connection has been established");
    return NULL;
  }

  return close(reason, message);
}

void CWebSocketV8::Fail()
{
  m_state = WebSocketStateClosed;
}

CWebSocketFrame* CWebSocketV8::GetFrame(const char* data, uint64_t length)
{
  return new CWebSocketFrame(data, length);
}

CWebSocketFrame* CWebSocketV8::GetFrame(WebSocketFrameOpcode opcode, const char* data /* = NULL */, uint32_t length /* = 0 */,
                                        bool final /* = true */, bool masked /* = false */, int32_t mask /* = 0 */, int8_t extension /* = 0 */)
{
  return new CWebSocketFrame(opcode, data, length, final, masked, mask, extension);
}

CWebSocketMessage* CWebSocketV8::GetMessage()
{
  return new CWebSocketMessage();
}

const CWebSocketFrame* CWebSocketV8::close(WebSocketCloseReason reason /* = WebSocketCloseNormal */, const std::string &message /* = "" */)
{
  size_t length = 2 + message.size();

  char* data = new char[length + 1];
  memset(data, 0, length + 1);
  uint16_t iReason = Endian_SwapBE16((uint16_t)reason);
  memcpy(data, &iReason, 2);
  message.copy(data + 2, message.size());

  if (m_state == WebSocketStateConnected)
    m_state = WebSocketStateClosing;
  else
    m_state = WebSocketStateClosed;

  CWebSocketFrame* frame = new CWebSocketFrame(WebSocketConnectionClose, data, length);
  delete[] data;

  return frame;
}

std::string CWebSocketV8::calculateKey(const std::string &key)
{
  std::string acceptKey = key;
  acceptKey.append(WS_KEY_MAGICSTRING);

  CDigest digest{CDigest::Type::SHA1};
  digest.Update(acceptKey);

  return Base64::Encode(digest.FinalizeRaw());
}
