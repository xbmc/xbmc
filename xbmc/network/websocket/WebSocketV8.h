#pragma once
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

#include "WebSocket.h"

class CWebSocketV8 : public CWebSocket
{
public:
  CWebSocketV8() { m_version = 8; }

  virtual bool Handshake(const char* data, size_t length, std::string &response);
  virtual const CWebSocketFrame* Ping(const char* data = NULL) const { return new CWebSocketFrame(WebSocketPing, data); }
  virtual const CWebSocketFrame* Pong(const char* data = NULL) const { return new CWebSocketFrame(WebSocketPong, data); }
  virtual const CWebSocketFrame* Close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "");
  virtual void Fail();

protected:
  virtual CWebSocketFrame* GetFrame(const char* data, uint64_t length);
  virtual CWebSocketFrame* GetFrame(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0, bool final = true, bool masked = false, int32_t mask = 0, int8_t extension = 0);
  virtual CWebSocketMessage* GetMessage();
  virtual const CWebSocketFrame* close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "");

  std::string calculateKey(const std::string &key);
};
