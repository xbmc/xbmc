#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <vector>
#include <string>

class CCriticalSection;

class CDNSNameCache
{
public:
  class CDNSName
  {
  public:
    std::string m_strHostName;
    std::string m_strIpAddress;
  };
  CDNSNameCache(void);
  virtual ~CDNSNameCache(void);
  static bool Lookup(const std::string& strHostName, std::string& strIpAddress);
  static void Add(const std::string& strHostName, const std::string& strIpAddress);

protected:
  static bool GetCached(const std::string& strHostName, std::string& strIpAddress);
  static CCriticalSection m_critical;
  std::vector<CDNSName> m_vecDNSNames;
};
