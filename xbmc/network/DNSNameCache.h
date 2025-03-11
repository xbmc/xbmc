/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

class CCriticalSection;

class CDNSNameCache
{
public:
  CDNSNameCache(void);
  virtual ~CDNSNameCache(void);
  static void Add(const std::string& strHostName, const std::string& strIpAddress);
  static void AddPermanent(const std::string& strHostName, const std::string& strIpAddress);
  static bool GetCached(const std::string& strHostName, std::string& strIpAddress);
  static bool Lookup(const std::string& strHostName, std::string& strIpAddress);

protected:
  static CCriticalSection m_critical;
  static constexpr std::chrono::seconds TTL{60};
  struct CacheEntry
  {
    CacheEntry(std::string ip, std::optional<std::chrono::steady_clock::time_point> expirationTime);

    std::string m_ip;
    std::optional<std::chrono::steady_clock::time_point> m_expirationTime;
  };
  std::unordered_map<std::string, CacheEntry> m_hostToIp;
};
