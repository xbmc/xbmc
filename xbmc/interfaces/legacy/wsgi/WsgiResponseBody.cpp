/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WsgiResponseBody.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    WsgiResponseBody::WsgiResponseBody()
      : m_data()
    { }

    WsgiResponseBody::~WsgiResponseBody() = default;

    void WsgiResponseBody::operator()(const String& data)
    {
      if (data.empty())
        return;

      m_data.append(data);
    }
  }
}
