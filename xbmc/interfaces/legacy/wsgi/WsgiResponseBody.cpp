/*
 *      Copyright (C) 2015 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
