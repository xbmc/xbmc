/*
 *  Copyright (C) 2011-2013 Team XBMC
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CWebSocket;

class CWebSocketManager
{
public:
  static CWebSocket* Handle(const char* data, unsigned int length, std::string &response);
};
