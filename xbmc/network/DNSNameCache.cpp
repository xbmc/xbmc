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

#include "DNSNameCache.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "network/Network.h"
#include "Application.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace ANNOUNCEMENT;

CDNSNameCache g_DNSCache;

CCriticalSection CDNSNameCache::m_critical;

CDNSNameCache::CDNSNameCache(void)
{
  CAnnouncementManager::GetInstance().AddAnnouncer(this);
}

CDNSNameCache::~CDNSNameCache(void)
{
  CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
}

void CDNSNameCache::Flush()
{
  CSingleLock lock(m_critical);
  CLog::Log(LOGINFO, "%s - DNS cache flushed (%u records)", __FUNCTION__, g_DNSCache.m_vecDNSNames.size());
  g_DNSCache.m_vecDNSNames.clear();
}

void CDNSNameCache::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!strcmp(sender, "network") && !strcmp(message, "OnInterfacesChange"))
    Flush();
}

bool CDNSNameCache::Lookup(const std::string& strHostName, std::string& strIpAddress)
{
  if (strHostName.empty() && strIpAddress.empty())
    return false;

  strIpAddress.clear();
  // first see if this is already an ip address
  {
    struct sockaddr_in sa;

    if (CNetwork::ConvIPv6(strHostName))
      strIpAddress = CNetwork::CanonizeIPv6(strHostName);
    else if (CNetwork::ConvIPv4(strHostName, &sa))
      strIpAddress = inet_ntoa(sa.sin_addr);
  }
  if (!strIpAddress.empty())
    return true;

  // check if there's a custom entry or if it's already cached
  if(g_DNSCache.GetCached(strHostName, strIpAddress))
    return true;

#ifndef TARGET_WINDOWS
  // perform netbios lookup (win32 is handling this via gethostbyname)
  char nmb_ip[100];
  char line[200];

  std::string cmd = "nmblookup " + strHostName;
  FILE* fp = popen(cmd.c_str(), "r");
  if (fp)
  {
    while (fgets(line, sizeof line, fp))
    {
      if (sscanf(line, "%99s *<00>\n", nmb_ip))
      {
        if (inet_addr(nmb_ip) != INADDR_NONE)
          strIpAddress = nmb_ip;
      }
    }
    pclose(fp);
  }

  if (!strIpAddress.empty())
  {
    g_DNSCache.Add(strHostName, strIpAddress);
    return true;
  }
#endif

  // perform dns lookup
  struct addrinfo hints;
  struct addrinfo *result = NULL;

  memset(&hints, 0, sizeof(struct addrinfo));
  int err;

  // prefer DNS record type (A vs AAAA) be the same as active interface(address).
  // otherwise (by default) system prefers A(IPv4) records. this can make
  // troubles on dual stack configured hosts or even make network access completely
  // unusable in case of pure IPv6 configuration because returning IPv4 record as first.

  // (NOTE/TODO: we might consider for future iterating via all returned results,
  // while checking accessibiity and use record which we can access. For now we just grab
  // first record from returned list).
  do
  {
    if (result)
      freeaddrinfo(result);
    hints.ai_family = hints.ai_family == 0 ? g_application.getNetwork().GetFirstConnectedFamily() : AF_UNSPEC;
    err = getaddrinfo(strHostName.c_str(), NULL, &hints, &result);
  } while ((err || !result) && hints.ai_family != AF_UNSPEC);

  std::string str_err;
  if (err)
    str_err = gai_strerror(err);

  bool bReturn;
  if (result)
  {
    strIpAddress = CNetwork::GetIpStr(result->ai_addr);
    freeaddrinfo(result);
    CLog::Log(LOGDEBUG, "%s - %s", __FUNCTION__, strIpAddress.c_str());
    g_DNSCache.Add(strHostName, strIpAddress);
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGERROR, "Unable to lookup host: '%s' (err detail: %s)", strHostName.c_str(), str_err.c_str());
    bReturn = false;
  }

  return bReturn;
}

bool CDNSNameCache::GetCached(const std::string& strHostName, std::string& strIpAddress)
{
  CSingleLock lock(m_critical);

  // loop through all DNSname entries and see if strHostName is cached
  for (int i = 0; i < (int)g_DNSCache.m_vecDNSNames.size(); ++i)
  {
    CDNSName& DNSname = g_DNSCache.m_vecDNSNames[i];
    if ( DNSname.m_strHostName == strHostName )
    {
      strIpAddress = DNSname.m_strIpAddress;
      return true;
    }
  }

  // not cached
  return false;
}

void CDNSNameCache::Add(const std::string &strHostName, const std::string &strIpAddress)
{
  CDNSName dnsName;

  dnsName.m_strHostName = strHostName;
  dnsName.m_strIpAddress  = strIpAddress;

  CSingleLock lock(m_critical);
  g_DNSCache.m_vecDNSNames.push_back(dnsName);
}

