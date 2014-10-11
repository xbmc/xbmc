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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

CDNSNameCache g_DNSCache;

CCriticalSection CDNSNameCache::m_critical;

CDNSNameCache::CDNSNameCache(void)
{}

CDNSNameCache::~CDNSNameCache(void)
{}

bool CDNSNameCache::Lookup(const std::string& strHostName, std::string& strIpAddress)
{
  if (strHostName.empty() && strIpAddress.empty())
    return false;

  // first see if this is already an ip address
  unsigned long address = inet_addr(strHostName.c_str());
  strIpAddress.clear();

  if (address != INADDR_NONE)
  {
    strIpAddress = StringUtils::Format("%lu.%lu.%lu.%lu", (address & 0xFF), (address & 0xFF00) >> 8, (address & 0xFF0000) >> 16, (address & 0xFF000000) >> 24 );
    return true;
  }

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
  struct hostent *host = gethostbyname(strHostName.c_str());
  if (host && host->h_addr_list[0])
  {
    strIpAddress = StringUtils::Format("%d.%d.%d.%d",
                                       (unsigned char)host->h_addr_list[0][0],
                                       (unsigned char)host->h_addr_list[0][1],
                                       (unsigned char)host->h_addr_list[0][2],
                                       (unsigned char)host->h_addr_list[0][3]);
    g_DNSCache.Add(strHostName, strIpAddress);
    return true;
  }

  CLog::Log(LOGERROR, "Unable to lookup host: '%s'", strHostName.c_str());
  return false;
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

