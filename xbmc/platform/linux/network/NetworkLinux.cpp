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

const std::string& CNetworkInterfaceLinux::GetName(void) const
{
   return m_interfaceName;
}

bool CNetworkInterfaceLinux::IsWireless() const
{
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  return false;
#else
  struct iwreq wrq;
   strcpy(wrq.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIWNAME, &wrq) < 0)
      return false;
#endif

   return true;
}

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

std::string CNetworkInterfaceLinux::GetCurrentWirelessEssId(void) const
{
   std::string result;

#if defined(TARGET_LINUX)
   char essid[IW_ESSID_MAX_SIZE + 1];
   memset(&essid, 0, sizeof(essid));

   struct iwreq wrq;
   strcpy(wrq.ifr_name,  m_interfaceName.c_str());
   wrq.u.essid.pointer = (caddr_t) essid;
   wrq.u.essid.length = IW_ESSID_MAX_SIZE;
   wrq.u.essid.flags = 0;
   if (ioctl(m_network->GetSocket(), SIOCGIWESSID, &wrq) >= 0)
   {
      result = essid;
   }
#endif

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

void CNetworkLinux::SetNameServers(const std::vector<std::string>& nameServers)
{
#if !defined(TARGET_ANDROID)
   FILE* fp = fopen("/etc/resolv.conf", "w");
   if (fp != NULL)
   {
      for (unsigned int i = 0; i < nameServers.size(); i++)
      {
         fprintf(fp, "nameserver %s\n", nameServers[i].c_str());
      }
      fclose(fp);
   }
   else
   {
      //! @todo implement
   }
#endif
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
#if !defined (TARGET_DARWIN_EMBEDDED) // no system calls allowed since ios11
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

std::vector<NetworkAccessPoint> CNetworkInterfaceLinux::GetAccessPoints(void) const
{
   std::vector<NetworkAccessPoint> result;

   if (!IsWireless())
      return result;

#if defined(TARGET_LINUX)
   // Query the wireless extension's version number. It will help us when we
   // parse the resulting events
   struct iwreq iwr;
   char rangebuffer[sizeof(iw_range) * 2];    /* Large enough */
   struct iw_range*  range = (struct iw_range*) rangebuffer;

   memset(rangebuffer, 0, sizeof(rangebuffer));
   iwr.u.data.pointer = (caddr_t) rangebuffer;
   iwr.u.data.length = sizeof(rangebuffer);
   iwr.u.data.flags = 0;
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   iwr.ifr_name[IFNAMSIZ - 1] = 0;
   if (ioctl(m_network->GetSocket(), SIOCGIWRANGE, &iwr) < 0)
   {
      CLog::Log(LOGWARNING, "%-8.16s  Driver has no Wireless Extension version information.",
         GetName().c_str());
      return result;
   }

   // Scan for wireless access points
   memset(&iwr, 0, sizeof(iwr));
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   iwr.ifr_name[IFNAMSIZ - 1] = 0;
   if (ioctl(m_network->GetSocket(), SIOCSIWSCAN, &iwr) < 0)
   {
      // Triggering scanning is a privileged operation (root only)
      if (errno == EPERM)
         CLog::Log(LOGWARNING, "Cannot initiate wireless scan: ioctl[SIOCSIWSCAN]: %s. Try running as root", strerror(errno));
      else
         CLog::Log(LOGWARNING, "Cannot initiate wireless scan: ioctl[SIOCSIWSCAN]: %s", strerror(errno));
      return result;
   }

   // Get the results of the scanning. Three scenarios:
   //    1. There's not enough room in the result buffer (E2BIG)
   //    2. The scanning is not complete (EAGAIN) and we need to try again. We cap this with 15 seconds.
   //    3. We're good.
   int duration = 0; // ms
   unsigned char* res_buf = NULL;
   int res_buf_len = IW_SCAN_MAX_DATA;
   while (duration < 15000)
   {
      if (!res_buf)
         res_buf = (unsigned char*) malloc(res_buf_len);

      if (res_buf == NULL)
      {
         CLog::Log(LOGWARNING, "Cannot alloc memory for wireless scanning");
         return result;
      }

      strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
      iwr.ifr_name[IFNAMSIZ - 1] = 0;
      iwr.u.data.pointer = res_buf;
      iwr.u.data.length = res_buf_len;
      iwr.u.data.flags = 0;
      int x = ioctl(m_network->GetSocket(), SIOCGIWSCAN, &iwr);
      if (x == 0)
         break;

      if (errno == E2BIG && res_buf_len < 100000)
      {
         free(res_buf);
         res_buf = NULL;
         res_buf_len *= 2;
         CLog::Log(LOGDEBUG, "Scan results did not fit - trying larger buffer (%lu bytes)",
                        (unsigned long) res_buf_len);
      }
      else if (errno == EAGAIN)
      {
         usleep(250000); // sleep for 250ms
         duration += 250;
      }
      else
      {
         CLog::Log(LOGWARNING, "Cannot get wireless scan results: ioctl[SIOCGIWSCAN]: %s", strerror(errno));
         free(res_buf);
         return result;
      }
   }

   size_t len = iwr.u.data.length;           // total length of the wireless events from the scan results
   unsigned char* pos = res_buf;             // pointer to the current event (about 10 per wireless network)
   unsigned char* end = res_buf + len;       // marks the end of the scan results
   unsigned char* custom;                    // pointer to the event payload
   struct iw_event iwe_buf, *iwe = &iwe_buf; // buffer to hold individual events

   std::string essId;
   std::string macAddress;
   int signalLevel = 0;
   EncMode encryption = ENC_NONE;
   int channel = 0;

   while (pos + IW_EV_LCP_LEN <= end)
   {
      /* Event data may be unaligned, so make a local, aligned copy
       * before processing. */

      // copy event prefix (size of event minus IOCTL fixed payload)
      memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
      if (iwe->len <= IW_EV_LCP_LEN)
         break;

      // if the payload is nontrivial (i.e. > 16 octets) assume it comes after a pointer
      custom = pos + IW_EV_POINT_LEN;
      if (range->we_version_compiled > 18 &&
          (iwe->cmd == SIOCGIWESSID ||
           iwe->cmd == SIOCGIWENCODE ||
           iwe->cmd == IWEVGENIE ||
           iwe->cmd == IWEVCUSTOM))
      {
         /* Wireless extensions v19 removed the pointer from struct iw_point */
         char *data_pos = (char *) &iwe_buf.u.data.length;
         int data_len = data_pos - (char *) &iwe_buf;
         memcpy(data_pos, pos + IW_EV_LCP_LEN, sizeof(struct iw_event) - data_len);
      }
      else
      {
         // copy the rest of the event and point custom toward the payload offset
         memcpy(&iwe_buf, pos, sizeof(struct iw_event));
         custom += IW_EV_POINT_OFF;
      }

      // Interpret the payload based on event type. Each access point generates ~12 different events
      switch (iwe->cmd)
      {
         // Get access point MAC addresses
         case SIOCGIWAP:
         {
            // This event marks a new access point, so push back the old information
            if (!macAddress.empty())
               result.push_back(NetworkAccessPoint(essId, macAddress, signalLevel, encryption, channel));
            unsigned char* mac = (unsigned char*)iwe->u.ap_addr.sa_data;
            // macAddress is big-endian, write in byte chunks
            macAddress = StringUtils::Format("%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            // Reset the remaining fields
            essId = "";
            encryption = ENC_NONE;
            signalLevel = 0;
            channel = 0;
            break;
         }

         // Get operation mode
         case SIOCGIWMODE:
         {
            // Ignore Ad-Hoc networks (1 is the magic number for this)
            if (iwe->u.mode == 1)
               macAddress = "";
            break;
         }

         // Get ESSID
         case SIOCGIWESSID:
         {
            char essid[IW_ESSID_MAX_SIZE+1];
            memset(essid, '\0', sizeof(essid));
            if ((custom) && (iwe->u.essid.length))
            {
               memcpy(essid, custom, iwe->u.essid.length);
               essId = essid;
            }
            break;
         }

         // Quality part of statistics
         case IWEVQUAL:
         {
            // u.qual.qual is scaled to a vendor-specific RSSI_Max, so use u.qual.level
            signalLevel = iwe->u.qual.level - 0x100; // and remember we use 8-bit arithmetic
            break;
         }

         // Get channel/frequency (Hz)
         // This gets called twice per network, what's the difference between the two?
         case SIOCGIWFREQ:
         {
            float freq = ((float)iwe->u.freq.m) * pow(10.0, iwe->u.freq.e);
            if (freq > 1000)
               channel = NetworkAccessPoint::FreqToChannel(freq);
            else
               channel = (int)freq; // Some drivers report channel instead of frequency
            break;
         }

         // Get encoding token & mode
         case SIOCGIWENCODE:
         {
            if (!(iwe->u.data.flags & IW_ENCODE_DISABLED) && encryption == ENC_NONE)
               encryption = ENC_WEP;
            break;
         }

         // Generic IEEE 802.11 information element (IE) for WPA, RSN, WMM, ...
         case IWEVGENIE:
         {
            int offset = 0;
            // Loop on each IE, each IE is minimum 2 bytes
            while (offset <= (iwe_buf.u.data.length - 2))
            {
               switch (custom[offset])
               {
                  case 0xdd: /* WPA1 */
                     if (encryption != ENC_WPA2)
                        encryption = ENC_WPA;
                     break;
                  case 0x30: /* WPA2 */
                     encryption = ENC_WPA2;
               }
               // Skip over this IE to the next one in the list
               offset += custom[offset+1] + 2;
            }
         }
      }

      pos += iwe->len;
   }

   if (!macAddress.empty())
      result.push_back(NetworkAccessPoint(essId, macAddress, signalLevel, encryption, channel));

   free(res_buf);
   res_buf = NULL;
#endif

   return result;
}

void CNetworkInterfaceLinux::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)  const
{
   ipAddress = "0.0.0.0";
   networkMask = "0.0.0.0";
   defaultGateway = "0.0.0.0";
   essId = "";
   key = "";
   encryptionMode = ENC_NONE;
   assignment = NETWORK_DISABLED;

#if defined(TARGET_LINUX)
   FILE* fp = fopen("/etc/network/interfaces", "r");
   if (!fp)
   {
      //! @todo implement
      return;
   }

   char* line = NULL;
   size_t linel = 0;
   std::string s;
   bool foundInterface = false;

   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      std::vector<std::string> tokens;

      s = line;
      StringUtils::TrimLeft(s, " \t");
      StringUtils::TrimRight(s," \n");

      // skip comments
      if (s.empty() || s[0] == '#')
         continue;

      // look for "iface <interface name> inet"
      StringUtils::Tokenize(s, tokens, " ");
      if (!foundInterface &&
          tokens.size() >=3 &&
          StringUtils::EqualsNoCase(tokens[0], "iface") &&
          StringUtils::EqualsNoCase(tokens[1], GetName()) &&
          StringUtils::EqualsNoCase(tokens[2], "inet"))
      {
         if (StringUtils::EqualsNoCase(tokens[3], "dhcp"))
         {
            assignment = NETWORK_DHCP;
            foundInterface = true;
         }
         if (StringUtils::EqualsNoCase(tokens[3], "static"))
         {
            assignment = NETWORK_STATIC;
            foundInterface = true;
         }
      }

      if (foundInterface && tokens.size() == 2)
      {
         if (StringUtils::EqualsNoCase(tokens[0], "address")) ipAddress = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "netmask")) networkMask = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "gateway")) defaultGateway = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "wireless-essid")) essId = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "wireless-key"))
         {
            key = tokens[1];
            if (key.length() > 2 && key[0] == 's' && key[1] == ':')
               key.erase(0, 2);
            encryptionMode = ENC_WEP;
         }
         else if (StringUtils::EqualsNoCase(tokens[0], "wpa-ssid")) essId = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "wpa-proto") && StringUtils::EqualsNoCase(tokens[1], "WPA")) encryptionMode = ENC_WPA;
         else if (StringUtils::EqualsNoCase(tokens[0], "wpa-proto") && StringUtils::EqualsNoCase(tokens[1], "WPA2")) encryptionMode = ENC_WPA2;
         else if (StringUtils::EqualsNoCase(tokens[0], "wpa-psk")) key = tokens[1];
         else if (StringUtils::EqualsNoCase(tokens[0], "auto") || StringUtils::EqualsNoCase(tokens[0], "iface") || StringUtils::EqualsNoCase(tokens[0], "mapping")) break;
      }
   }
   free(line);

   // Fallback in case wpa-proto is not set
   if (key != "" && encryptionMode == ENC_NONE)
      encryptionMode = ENC_WPA;

   fclose(fp);
#endif
}

void CNetworkInterfaceLinux::SetSettings(const NetworkAssignment& assignment, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway, const std::string& essId, const std::string& key, const EncMode& encryptionMode)
{
#if defined(TARGET_LINUX)
   FILE* fr = fopen("/etc/network/interfaces", "r");
   if (!fr)
   {
      //! @todo implement
      return;
   }

   FILE* fw = fopen("/tmp/interfaces.temp", "w");
   if (!fw)
   {
      //! @todo implement
      fclose(fr);
      return;
   }

   char* line = NULL;
   size_t linel = 0;
   std::string s;
   bool foundInterface = false;
   bool dataWritten = false;

   while (getdelim(&line, &linel, '\n', fr) > 0)
   {
      std::vector<std::string> tokens;

      s = line;
      StringUtils::TrimLeft(s, " \t");
      StringUtils::TrimRight(s," \n");

      // skip comments
      if (!foundInterface && (s.empty() || s[0] == '#'))
      {
        fprintf(fw, "%s", line);
        continue;
      }

      // look for "iface <interface name> inet"
      StringUtils::Tokenize(s, tokens, " ");
      if (tokens.size() == 2 &&
          StringUtils::EqualsNoCase(tokens[0], "auto") &&
          StringUtils::EqualsNoCase(tokens[1], GetName()))
      {
         continue;
      }
      else if (!foundInterface &&
          tokens.size() == 4 &&
          StringUtils::EqualsNoCase(tokens[0], "iface") &&
          StringUtils::EqualsNoCase(tokens[1], GetName()) &&
          StringUtils::EqualsNoCase(tokens[2], "inet"))
      {
         foundInterface = true;
         WriteSettings(fw, assignment, ipAddress, networkMask, defaultGateway, essId, key, encryptionMode);
         dataWritten = true;
      }
      else if (foundInterface &&
               tokens.size() == 4 &&
               StringUtils::EqualsNoCase(tokens[0], "iface"))
      {
        foundInterface = false;
        fprintf(fw, "%s", line);
      }
      else if (!foundInterface)
      {
        fprintf(fw, "%s", line);
      }
   }
   free(line);

   if (!dataWritten && assignment != NETWORK_DISABLED)
   {
      fprintf(fw, "\n");
      WriteSettings(fw, assignment, ipAddress, networkMask, defaultGateway, essId, key, encryptionMode);
   }

   fclose(fr);
   fclose(fw);

   // Rename the file
   if (rename("/tmp/interfaces.temp", "/etc/network/interfaces") < 0)
   {
      //! @todo implement
      return;
   }

   std::string cmd = "/sbin/ifdown " + GetName();
   if (system(cmd.c_str()) != 0)
     CLog::Log(LOGERROR, "Unable to stop interface %s", GetName().c_str());
   else
     CLog::Log(LOGINFO, "Stopped interface %s", GetName().c_str());

   if (assignment != NETWORK_DISABLED)
   {
      cmd = "/sbin/ifup " + GetName();
      if (system(cmd.c_str()) != 0)
        CLog::Log(LOGERROR, "Unable to start interface %s", GetName().c_str());
      else
        CLog::Log(LOGINFO, "Started interface %s", GetName().c_str());
   }
#endif
}

void CNetworkInterfaceLinux::WriteSettings(FILE* fw, NetworkAssignment assignment, const std::string& ipAddress, const std::string& networkMask, const std::string& defaultGateway, const std::string& essId, const std::string& key, const EncMode& encryptionMode)
{
   if (assignment == NETWORK_DHCP)
   {
      fprintf(fw, "iface %s inet dhcp\n", GetName().c_str());
   }
   else if (assignment == NETWORK_STATIC)
   {
      fprintf(fw, "iface %s inet static\n", GetName().c_str());
      fprintf(fw, "  address %s\n", ipAddress.c_str());
      fprintf(fw, "  netmask %s\n", networkMask.c_str());
      fprintf(fw, "  gateway %s\n", defaultGateway.c_str());
   }

   if (assignment != NETWORK_DISABLED && IsWireless())
   {
      if (encryptionMode == ENC_NONE)
      {
         fprintf(fw, "  wireless-essid %s\n", essId.c_str());
      }
      else if (encryptionMode == ENC_WEP)
      {
         fprintf(fw, "  wireless-essid %s\n", essId.c_str());
         fprintf(fw, "  wireless-key s:%s\n", key.c_str());
      }
      else if (encryptionMode == ENC_WPA || encryptionMode == ENC_WPA2)
      {
         fprintf(fw, "  wpa-ssid %s\n", essId.c_str());
         fprintf(fw, "  wpa-psk %s\n", key.c_str());
         fprintf(fw, "  wpa-proto %s\n", encryptionMode == ENC_WPA ? "WPA" : "WPA2");
      }
   }

   if (assignment != NETWORK_DISABLED)
      fprintf(fw, "auto %s\n\n", GetName().c_str());
}


