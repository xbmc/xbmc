#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <vector>
#include <string>
#include "interfaces/AnnouncementManager.h"

class CCriticalSection;

class CDNSNameCache : public ANNOUNCEMENT::IAnnouncer
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

  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

protected:
  static bool GetCached(const std::string& strHostName, std::string& strIpAddress);
  void Flush();
  static CCriticalSection m_critical;
  std::vector<CDNSName> m_vecDNSNames;
};
