/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DNSNameCache.h"

#include "network/Network.h"
#include "utils/log.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>

#if !defined(TARGET_WINDOWS) && defined(HAS_FILESYSTEM_SMB)
#include "ServiceBroker.h"

#include "platform/posix/filesystem/SMBWSDiscovery.h"
#endif

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#if defined(TARGET_FREEBSD)
#include <sys/socket.h>
#endif

namespace
{
/*!
 * \brief Check whether an address can never be used to reach a remote host
 *
 * Resolvers do hand out loopback addresses for names that belong to another machine, e.g. an
 * AAAA record of ::1 for a host whose A record is its real address. RFC 6724 destination
 * address sorting puts such an address first, so the first result of a lookup cannot be
 * trusted to be reachable.
 */
bool IsUnusableForRemoteHost(const addrinfo* info)
{
  switch (info->ai_family)
  {
    case AF_INET:
    {
      const uint32_t address =
          ntohl(reinterpret_cast<const sockaddr_in*>(info->ai_addr)->sin_addr.s_addr);
      return address == INADDR_ANY || (address >> 24) == 127;
    }
    case AF_INET6:
    {
      const in6_addr& address = reinterpret_cast<const sockaddr_in6*>(info->ai_addr)->sin6_addr;
      return IN6_IS_ADDR_UNSPECIFIED(&address) || IN6_IS_ADDR_LOOPBACK(&address);
    }
    default:
      return true;
  }
}
} // unnamed namespace

bool CDNSNameCache::Lookup(const std::string& strHostName, std::string& strIpAddress)
{
  if (strHostName.empty() && strIpAddress.empty())
    return false;

  // first see if this is already an ip address
  in_addr addr4;
  in6_addr addr6;
  strIpAddress.clear();

  if (inet_pton(AF_INET, strHostName.c_str(), &addr4) ||
      inet_pton(AF_INET6, strHostName.c_str(), &addr6))
  {
    strIpAddress = strHostName;
    return true;
  }

  // check if there's a custom entry or if it's already cached
  if (GetCached(strHostName, strIpAddress))
    return true;

  // perform dns lookup
  addrinfo hints{};
  addrinfo* res;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;
  // don't get offered addresses of a family this host has no address configured for, they
  // could never be connected to
  hints.ai_flags |= AI_ADDRCONFIG;

  if (getaddrinfo(strHostName.c_str(), nullptr, &hints, &res) == 0)
  {
    // take the first address that can reach a remote host, but keep an unreachable one as a
    // fallback so that a name which only resolves to loopback, e.g. "localhost", still works
    std::string fallback;
    for (const addrinfo* info = res; info; info = info->ai_next)
    {
      std::string address = CNetworkBase::GetIpStr(info->ai_addr);
      if (address.empty())
        continue;

      if (IsUnusableForRemoteHost(info))
      {
        if (fallback.empty())
          fallback = std::move(address);
        continue;
      }

      strIpAddress = std::move(address);
      break;
    }
    freeaddrinfo(res);

    if (strIpAddress.empty())
      strIpAddress = std::move(fallback);

    if (!strIpAddress.empty())
    {
      Add(strHostName, strIpAddress);
      return true;
    }
  }

  CLog::Log(LOGERROR, "Unable to lookup host: '{}'", strHostName);
  return false;
}

bool CDNSNameCache::GetCached(const std::string& strHostName, std::string& strIpAddress) const
{
  std::lock_guard lock(m_critical);

  if (auto iter = m_hostToIp.find(strHostName); iter != m_hostToIp.end())
  {
    if (!iter->second.m_expirationTime ||
        iter->second.m_expirationTime > std::chrono::steady_clock::now())
    {
      strIpAddress = iter->second.m_ip;
      return true;
    }
    else
      m_hostToIp.erase(iter);
  }

#if !defined(TARGET_WINDOWS) && defined(HAS_FILESYSTEM_SMB)
  if (WSDiscovery::CWSDiscoveryPosix::IsInitialized())
  {
    WSDiscovery::CWSDiscoveryPosix& WSInstance =
        dynamic_cast<WSDiscovery::CWSDiscoveryPosix&>(CServiceBroker::GetWSDiscovery());
    if (WSInstance.GetCached(strHostName, strIpAddress))
      return true;
  }
  else
    CLog::Log(LOGDEBUG, LOGWSDISCOVERY,
              "CDNSNameCache::GetCached: CWSDiscoveryPosix not initialized");
#endif

  // not cached
  return false;
}

void CDNSNameCache::Add(const std::string& strHostName, const std::string& strIpAddress)
{
  std::lock_guard lock(m_critical);
  m_hostToIp.emplace(std::piecewise_construct, std::forward_as_tuple(strHostName),
                     std::forward_as_tuple(strIpAddress, std::chrono::steady_clock::now() + TTL));
}

void CDNSNameCache::AddPermanent(const std::string& strHostName, const std::string& strIpAddress)
{
  std::lock_guard lock(m_critical);
  m_hostToIp.emplace(std::piecewise_construct, std::forward_as_tuple(strHostName),
                     std::forward_as_tuple(strIpAddress, std::nullopt));
}

CDNSNameCache::CacheEntry::CacheEntry(
    std::string ip, std::optional<std::chrono::steady_clock::time_point> expirationTime)
  : m_ip(std::move(ip)), m_expirationTime(expirationTime)
{
}
