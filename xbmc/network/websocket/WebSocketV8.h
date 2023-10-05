/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebSocket.h"

#include <string>

class CWebSocketV8 : public CWebSocket
{
public:
  CWebSocketV8() { m_version = 8; }

  bool Handshake(const char* data, size_t length, std::string &response) override;
  const CWebSocketFrame* Ping(const char* data = NULL) const override { return new CWebSocketFrame(WebSocketPing, data); }
  const CWebSocketFrame* Pong(const char* data, uint32_t length) const override
  {
    return new CWebSocketFrame(WebSocketPong, data, length);
  }
  const CWebSocketFrame* Close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "") override;
  void Fail() override;

protected:
  CWebSocketFrame* GetFrame(const char* data, uint64_t length) override;
  CWebSocketFrame* GetFrame(WebSocketFrameOpcode opcode, const char* data = NULL, uint32_t length = 0, bool final = true, bool masked = false, int32_t mask = 0, int8_t extension = 0) override;
  CWebSocketMessage* GetMessage() override;
  virtual const CWebSocketFrame* close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "");

  std::string calculateKey(const std::string &key);
};
