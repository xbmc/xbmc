/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cstdlib>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(TARGET_LINUX)
  #include <linux/if.h>
  #include <linux/wireless.h>
  #include <linux/sockios.h>
#endif
#ifdef TARGET_ANDROID
#include "platform/android/bionic_supplement/bionic_supplement.h"
#include "sys/system_properties.h"
#include <sys/wait.h>
#endif
#include <errno.h>
#include <resolv.h>
#if defined(TARGET_DARWIN)
  #include <sys/sockio.h>
  #include <net/if.h>
  #include <net/if_dl.h>
#if defined(TARGET_DARWIN_OSX)
  #include <net/if_types.h>
  #include <net/route.h>
  #include <netinet/if_ether.h>
#else //IOS
  #include "platform/darwin/network/ioshacks.h"
#endif
  #include <ifaddrs.h>
#elif defined(TARGET_FREEBSD)
  #include <sys/sockio.h>
  #include <sys/wait.h>
  #include <net/if.h>
  #include <net/if_arp.h>
  #include <net/if_dl.h>
  #include <ifaddrs.h>
  #include <net/route.h>
  #include <netinet/if_ether.h>
#else
  #include <net/if_arp.h>
#endif
#include "PlatformDefs.h"
#include "NetworkLinux.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkLinux* network, std::string interfaceName, char interfaceMacAddrRaw[6]):
  m_interfaceName(interfaceName),
  m_interfaceMacAdr(StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
                                        (uint8_t)interfaceMacAddrRaw[0],
                                        (uint8_t)interfaceMacAddrRaw[1],
                                        (uint8_t)interfaceMacAddrRaw[2],
                                        (uint8_t)interfaceMacAddrRaw[3],
                                        (uint8_t)interfaceMacAddrRaw[4],
                                        (uint8_t)interfaceMacAddrRaw[5]))
{
   m_network = network;
   memcpy(m_interfaceMacAddrRaw, interfaceMacAddrRaw, sizeof(m_interfaceMacAddrRaw));
}

CNetworkInterfaceLinux::~CNetworkInterfaceLinux(void) = default;

bool CNetworkInterfaceLinux::IsEnabled() const
{
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
}

bool CNetworkInterfaceLinux::IsConnected() const
{
   struct ifreq ifr;
   int zero = 0;
   memset(&ifr,0,sizeof(struct ifreq));
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   // ignore loopback
   int iRunning = ( (ifr.ifr_flags & IFF_RUNNING) && (!(ifr.ifr_flags & IFF_LOOPBACK)));

   if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) < 0)
      return false;

   // return only interfaces which has ip address
   return iRunning && (0 != memcmp(ifr.ifr_addr.sa_data+sizeof(short), &zero, sizeof(int)));
}

std::string CNetworkInterfaceLinux::GetMacAddress() const
{
  return m_interfaceMacAdr;
}

void CNetworkInterfaceLinux::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_interfaceMacAddrRaw, 6);
}

std::string CNetworkInterfaceLinux::GetCurrentIPAddress(void) const
{
   std::string result;

   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0)
   {
      result = inet_ntoa((*((struct sockaddr_in *)&ifr.ifr_addr)).sin_addr);
   }

   return result;
}

std::string CNetworkInterfaceLinux::GetCurrentNetmask(void) const
{
   std::string result;

   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFNETMASK, &ifr) >= 0)
   {
      result = inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
   }

   return result;
}

std::string CNetworkInterfaceLinux::GetCurrentDefaultGateway(void) const
{
   std::string result;

#if defined(TARGET_DARWIN)
  FILE* pipe = popen("echo \"show State:/Network/Global/IPv4\" | scutil | grep Router", "r");
  usleep(100000);
  if (pipe)
  {
    std::string tmpStr;
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
    {
      tmpStr = buffer;
      if (tmpStr.length() >= 11)
        result = tmpStr.substr(11);
    }
    pclose(pipe);
  }
  if (result.empty())
    CLog::Log(LOGWARNING, "Unable to determine gateway");
#elif defined(TARGET_FREEBSD)
   size_t needed;
   int mib[6];
   char *buf, *next, *lim;
   char line[16];
   struct rt_msghdr *rtm;
   struct sockaddr *sa;
   struct sockaddr_in *sockin;

   mib[0] = CTL_NET;
   mib[1] = PF_ROUTE;
   mib[2] = 0;
   mib[3] = 0;
   mib[4] = NET_RT_DUMP;
   mib[5] = 0;
   if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
      return result;

   if ((buf = (char *)malloc(needed)) == NULL)
      return result;

   if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
      free(buf);
      return result;
   }

   lim  = buf + needed;
   for (next = buf; next < lim; next += rtm->rtm_msglen) {
      rtm = (struct rt_msghdr *)next;
      sa = (struct sockaddr *)(rtm + 1);
      sa = (struct sockaddr *)(SA_SIZE(sa) + (char *)sa);
      sockin = (struct sockaddr_in *)sa;
      if (inet_ntop(AF_INET, &sockin->sin_addr.s_addr,
         line, sizeof(line)) == NULL) {
            free(buf);
            return result;
	  }
	  result = line;
      break;
   }
   free(buf);
#else
   FILE* fp = fopen("/proc/net/route", "r");
   if (!fp)
   {
     // TBD: Error
     return result;
   }

   char* line = NULL;
   char iface[16];
   char dst[128];
   char gateway[128];
   size_t linel = 0;
   int n;
   int linenum = 0;
   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      // skip first two lines
      if (linenum++ < 1)
         continue;

      // search where the word begins
      n = sscanf(line,  "%15s %127s %127s",
         iface, dst, gateway);

      if (n < 3)
         continue;

      if (strcmp(iface, m_interfaceName.c_str()) == 0 &&
          strcmp(dst, "00000000") == 0 &&
          strcmp(gateway, "00000000") != 0)
      {
         unsigned char gatewayAddr[4];
         int len = CNetworkBase::ParseHex(gateway, gatewayAddr);
         if (len == 4)
         {
            struct in_addr in;
            in.s_addr = (gatewayAddr[0] << 24) | (gatewayAddr[1] << 16) |
                        (gatewayAddr[2] << 8) | (gatewayAddr[3]);
            result = inet_ntoa(in);
            break;
         }
      }
   }
   free(line);
   fclose(fp);
#endif

   return result;
}

CNetworkLinux::CNetworkLinux()
 : CNetworkBase()
{
   m_sock = socket(AF_INET, SOCK_DGRAM, 0);
   queryInterfaceList();
}

CNetworkLinux::~CNetworkLinux(void)
{
  if (m_sock != -1)
    close(CNetworkLinux::m_sock);

  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
}

std::vector<CNetworkInterface*>& CNetworkLinux::GetInterfaceList(void)
{
   return m_interfaces;
}

//! @bug
//! Overwrite the GetFirstConnectedInterface and requery
//! the interface list if no connected device is found
//! this fixes a bug when no network is available after first start of xbmc
//! and the interface comes up during runtime
CNetworkInterface* CNetworkLinux::GetFirstConnectedInterface(void)
{
    CNetworkInterface *pNetIf=CNetworkBase::GetFirstConnectedInterface();

    // no connected Interfaces found? - requeryInterfaceList
    if (!pNetIf)
    {
        CLog::Log(LOGDEBUG,"%s no connected interface found - requery list",__FUNCTION__);
        queryInterfaceList();
        //retry finding a connected if
        pNetIf = CNetworkBase::GetFirstConnectedInterface();
    }

    return pNetIf;
}


void CNetworkLinux::GetMacAddress(const std::string& interfaceName, char rawMac[6])
{
  memset(rawMac, 0, 6);
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)

#if !defined(IFT_ETHER)
#define IFT_ETHER 0x6/* Ethernet CSMACD */
#endif
  const struct sockaddr_dl* dlAddr = NULL;
  const uint8_t * base = NULL;
  // Query the list of interfaces.
  struct ifaddrs *list;
  struct ifaddrs *interface;

  if( getifaddrs(&list) < 0 )
  {
    return;
  }

  for(interface = list; interface != NULL; interface = interface->ifa_next)
  {
    if(interfaceName == interface->ifa_name)
    {
      if ( (interface->ifa_addr->sa_family == AF_LINK) && (((const struct sockaddr_dl *) interface->ifa_addr)->sdl_type == IFT_ETHER) )
      {
        dlAddr = (const struct sockaddr_dl *) interface->ifa_addr;
        base = (const uint8_t *) &dlAddr->sdl_data[dlAddr->sdl_nlen];

        if( dlAddr->sdl_alen > 5 )
        {
          memcpy(rawMac, base, 6);
        }
      }
      break;
    }
  }

  freeifaddrs(list);

#else

   struct ifreq ifr;
   strcpy(ifr.ifr_name, interfaceName.c_str());
   if (ioctl(GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
   {
      memcpy(rawMac, ifr.ifr_hwaddr.sa_data, 6);
   }
#endif
}

void CNetworkLinux::queryInterfaceList()
{
  char macAddrRaw[6];
  m_interfaces.clear();

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)

   // Query the list of interfaces.
   struct ifaddrs *list;
   if (getifaddrs(&list) < 0)
     return;

   struct ifaddrs *cur;
   for(cur = list; cur != NULL; cur = cur->ifa_next)
   {
     if(cur->ifa_addr->sa_family != AF_INET)
       continue;

     GetMacAddress(cur->ifa_name, macAddrRaw);

      // only add interfaces with non-zero mac addresses
      if (macAddrRaw[0] || macAddrRaw[1] || macAddrRaw[2] || macAddrRaw[3] || macAddrRaw[4] || macAddrRaw[5])
         // Add the interface.
         m_interfaces.push_back(new CNetworkInterfaceLinux(this, cur->ifa_name, macAddrRaw));
   }

   freeifaddrs(list);

#else
   FILE* fp = fopen("/proc/net/dev", "r");
   if (!fp)
   {
     // TBD: Error
     return;
   }

   char* line = NULL;
   size_t linel = 0;
   int n;
   char* p;
   int linenum = 0;
   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      // skip first two lines
      if (linenum++ < 2)
         continue;

    // search where the word begins
      p = line;
      while (isspace(*p))
      ++p;

      // read word until :
      n = strcspn(p, ": \t");
      p[n] = 0;

      // save the result
      std::string interfaceName = p;
      GetMacAddress(interfaceName, macAddrRaw);

      // only add interfaces with non-zero mac addresses
      if (macAddrRaw[0] || macAddrRaw[1] || macAddrRaw[2] || macAddrRaw[3] || macAddrRaw[4] || macAddrRaw[5])
          m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName, macAddrRaw));
   }
   free(line);
   fclose(fp);
#endif
}

std::vector<std::string> CNetworkLinux::GetNameServers(void)
{
   std::vector<std::string> result;

#if defined(TARGET_DARWIN)
  FILE* pipe = popen("scutil --dns | grep \"nameserver\" | tail -n2", "r");
  usleep(100000);
  if (pipe)
  {
    std::vector<std::string> tmpStr;
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
    {
      tmpStr = StringUtils::Split(buffer, "\n");
      for (unsigned int i = 0; i < tmpStr.size(); i ++)
      {
        // result looks like this - > '  nameserver[0] : 192.168.1.1'
        // 2 blank spaces + 13 in 'nameserver[0]' + blank + ':' + blank == 18 :)
        if (tmpStr[i].length() >= 18)
          result.push_back(tmpStr[i].substr(18));
      }
    }
    pclose(pipe);
  }
  if (result.empty())
    CLog::Log(LOGWARNING, "Unable to determine nameserver");
#elif defined(TARGET_ANDROID)
  char nameserver[PROP_VALUE_MAX];

  if (__system_property_get("net.dns1",nameserver))
    result.push_back(nameserver);
  if (__system_property_get("net.dns2",nameserver))
    result.push_back(nameserver);
  if (__system_property_get("net.dns3",nameserver))
    result.push_back(nameserver);

  if (!result.size())
       CLog::Log(LOGWARNING, "Unable to determine nameserver");
#else
   res_init();

   for (int i = 0; i < _res.nscount; i ++)
   {
      std::string ns = inet_ntoa(_res.nsaddr_list[i].sin_addr);
      result.push_back(ns);
   }
#endif
   return result;
}

bool CNetworkLinux::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  char cmd_line [64];

  struct in_addr host_ip;
  host_ip.s_addr = remote_ip;

#if defined (TARGET_DARWIN) || defined (TARGET_FREEBSD)
  sprintf(cmd_line, "ping -c 1 -t %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));
#else
  sprintf(cmd_line, "ping -c 1 -w %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));
#endif

  int status = -1;
#if !defined (TARGET_DARWIN_IOS) // no system calls allowed since ios11
  status = system (cmd_line);
#endif
  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = %d, errno = %d : '%s'", status, errno, cmd_line);

  return result == 0;
}

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
bool CNetworkInterfaceLinux::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  bool ret = false;
  size_t needed;
  char *buf, *next;
  struct rt_msghdr *rtm;
  struct sockaddr_inarp *sin;
  struct sockaddr_dl *sdl;
  int mib[6];

  mac = "";

  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET;
  mib[4] = NET_RT_FLAGS;
  mib[5] = RTF_LLINFO;

  if (sysctl(mib, ARRAY_SIZE(mib), NULL, &needed, NULL, 0) == 0)
  {
    buf = (char*)malloc(needed);
    if (buf)
    {
      if (sysctl(mib, ARRAY_SIZE(mib), buf, &needed, NULL, 0) == 0)
      {
        for (next = buf; next < buf + needed; next += rtm->rtm_msglen)
        {

          rtm = (struct rt_msghdr *)next;
          sin = (struct sockaddr_inarp *)(rtm + 1);
          sdl = (struct sockaddr_dl *)(sin + 1);

          if (host_ip != sin->sin_addr.s_addr || sdl->sdl_alen < 6)
            continue;

          u_char *cp = (u_char*)LLADDR(sdl);

          mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
                                    cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
          ret = true;
          break;
        }
      }
      free(buf);
    }
  }
  return ret;
}
#else
bool CNetworkInterfaceLinux::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  struct arpreq areq;
  struct sockaddr_in* sin;

  memset(&areq, 0x0, sizeof(areq));

  sin = (struct sockaddr_in *) &areq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = host_ip;

  sin = (struct sockaddr_in *) &areq.arp_ha;
  sin->sin_family = ARPHRD_ETHER;

  strncpy(areq.arp_dev, m_interfaceName.c_str(), sizeof(areq.arp_dev));
  areq.arp_dev[sizeof(areq.arp_dev)-1] = '\0';

  int result = ioctl (m_network->GetSocket(), SIOCGARP, (caddr_t) &areq);

  if (result != 0)
  {
//  CLog::Log(LOGERROR, "%s - GetHostMacAddress/ioctl failed with errno (%d)", __FUNCTION__, errno);
    return false;
  }

  struct sockaddr* res = &areq.arp_ha;
  mac = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t) res->sa_data[0], (uint8_t) res->sa_data[1], (uint8_t) res->sa_data[2],
    (uint8_t) res->sa_data[3], (uint8_t) res->sa_data[4], (uint8_t) res->sa_data[5]);

  for (int i=0; i<6; ++i)
    if (res->sa_data[i])
      return true;

  return false;
}
#endif
