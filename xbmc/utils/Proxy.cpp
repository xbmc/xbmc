/*
 *      Copyright (C) 2016 Team XBMC
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

#include "Proxy.h"

CProxy::CProxy()
  : m_type(ProxyHttp)
  , m_port(3128)
{
}

CProxy::CProxy(Type type,
  const std::string& host,
  uint16_t port,
  const std::string& user,
  const std::string& password)
  : m_type(type)
  , m_host(host)
  , m_port(port)
  , m_user(user)
  , m_password(password)
{
}

CProxy::~CProxy()
{
}
