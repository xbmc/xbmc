/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebSocketV8.h"

class CWebSocketV13 : public CWebSocketV8
{
public:
  CWebSocketV13() { m_version = 13; }

  bool Handshake(const char* data, size_t length, std::string &response) override;
  const CWebSocketFrame* Close(WebSocketCloseReason reason = WebSocketCloseNormal, const std::string &message = "") override;
};
