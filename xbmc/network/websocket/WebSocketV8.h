/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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
#pragma once
#include "WebSocket.h"

class CWebSocketV8 : public CWebSocket
{
public:
  CWebSocketV8() { m_version = 8; }

  bool Handshake(const char* data, size_t length, std::string &response) override;
  const CWebSocketFrame* Ping(const char* data = NULL) const override { return new CWebSocketFrame(WebSocketPing, data); }
  const CWebSocketFrame* Pong(const char* data = NULL) const override { return new CWebSocketFrame(WebSocketPong, data); }
  const CWebSocketFrame* Close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "") override;
  void Fail() override;

protected:
  CWebSocketFrame* GetFrame(const char* data, uint64_t length) override;
  CWebSocketFrame* GetFrame(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0, bool final = true, bool masked = false, int32_t mask = 0, int8_t extension = 0) override;
  CWebSocketMessage* GetMessage() override;
  virtual const CWebSocketFrame* close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "");

  std::string calculateKey(const std::string &key);
};
