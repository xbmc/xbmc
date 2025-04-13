/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

class CDNSNameCache final
{
public:
  /*!
   * \brief Add the IP for a hostname to the cache. From the time of adding it stays in the cache for \ref TTL
   *
   * \param strHostName The hostname
   * \param strIpAddress The IP for that hostname. Can be IPv4 or IPv6
   */
  void Add(const std::string& strHostName, const std::string& strIpAddress);
  /*!
   * \brief Add the IP for a hostname to the cache indefinitely
   *
   * \param strHostName The hostname
   * \param strIpAddress The IP for that hostname. Can be IPv4 or IPv6
   */
  void AddPermanent(const std::string& strHostName, const std::string& strIpAddress);
  /*!
   * \brief Get an IP for a hostname from the cache
   *
   * \param strHostName The hostname to look up
   * \param[out] strIpAddress Contains the IP for the hostname if the info is in the cache, otherwise unchanged
   *
   * \return true if the IP is cached
   */
  bool GetCached(const std::string& strHostName, std::string& strIpAddress) const;
  /*!
   * \brief Get the IP for the hostname from the cache or query it form the DNS
   *
   * If a successful DNS query was performed the result is added to the cache for the duration of \ref TTL
   *
   * \param strHostName The hostname to look up
   * \param[out] strIpAddress Contains the IP for the hostname if the info can be provided, otherwise unchanged
   *
   * \return true if the IP is cached or the DNS query was successful
   */
  bool Lookup(const std::string& strHostName, std::string& strIpAddress);

private:
  static constexpr std::chrono::seconds TTL{60};

  struct CacheEntry
  {
    CacheEntry(std::string ip, std::optional<std::chrono::steady_clock::time_point> expirationTime);

    std::string m_ip;
    std::optional<std::chrono::steady_clock::time_point> m_expirationTime;
  };

  mutable CCriticalSection m_critical;
  mutable std::unordered_map<std::string, CacheEntry> m_hostToIp;
};
