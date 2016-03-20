#pragma once
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

#include <cstdint>
#include <memory>
#include <string>

class CProxy;

typedef std::shared_ptr<CProxy> CProxyPtr;

class CProxy
{
public:
  typedef enum
  {
    ProxyHttp = 0,
    ProxySocks4,
    ProxySocks4A,
    ProxySocks5,
    ProxySocks5Remote,
  } Type;

public:
  CProxy();

  CProxy(Type type, const std::string& host, uint16_t port,
    const std::string& user, const std::string& password);

  ~CProxy();

public:
  operator bool() const { return !m_host.empty(); }

  Type GetType() const { return m_type; }
  void SetType(Type type) { m_type = type; }
  const std::string& GetHost() const { return m_host; }
  void SetHost(const std::string &host) { m_host = host; }
  const std::string& GetUser() const { return m_user; }
  void SetPort(const uint16_t port) { m_port = port; }
  uint16_t GetPort() const { return m_port; }
  void SetUser(const std::string &user) { m_user = user; }
  const std::string& GetPassword() const { return m_password; }
  void SetPassword(const std::string &password) { m_password = password; }


private:
  Type m_type;
  std::string m_host;
  uint16_t m_port;
  std::string m_user;
  std::string m_password;
};
