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
#include <sstream>
#include <boost/uuid/sha1.hpp>

#include "WebSocketV8.h"
#include "WebSocket.h"
#include "utils/Base64.h"
#include "utils/CharsetConverter.h"
#include "utils/EndianSwap.h"
#include "utils/HttpParser.h"
#include "utils/HttpResponse.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

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

using namespace std;

bool CWebSocketV8::Handshake(const char* data, size_t length, std::string &response)
{
  string strHeader(data, length);
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
  if ((pos = strHeader.find(WS_HTTP_TAG)) == string::npos)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid handshake received");
    return false;
  }

  pos += strlen(WS_HTTP_TAG);
  istringstream converter(strHeader.substr(pos, strHeader.find_first_of(" \r\n\t", pos) - pos));
  float fVersion;
  converter >> fVersion;

  if (fVersion < 1.1f)
  {
    CLog::Log(LOGINFO, "WebSocket [hybi-10]: invalid HTTP version %f (1.1 or higher expected)", fVersion);
    return false;
  }

  string websocketKey, websocketProtocol;
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
    CStdStringArray protocols;
    StringUtils::SplitString(value, ",", protocols);
    for (unsigned int index = 0; index < protocols.size(); index++)
    {
      if (protocols.at(index).Trim().Equals(WS_PROTOCOL_JSONRPC))
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

  char *responseBuffer;
  int responseLength = httpResponse.Create(responseBuffer);
  response = std::string(responseBuffer, responseLength);
  
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
  string acceptKey = key;
  acceptKey.append(WS_KEY_MAGICSTRING);

  boost::uuids::detail::sha1 hash;
  hash.process_bytes(acceptKey.c_str(), acceptKey.size());

  unsigned int digest[5];
  hash.get_digest(digest);

  for (unsigned int index = 0; index < 5; index++)
    digest[index] = Endian_SwapBE32(digest[index]);

  return Base64::Encode((const char*)digest, sizeof(digest));
}
