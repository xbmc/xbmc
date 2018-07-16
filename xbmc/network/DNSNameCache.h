/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
