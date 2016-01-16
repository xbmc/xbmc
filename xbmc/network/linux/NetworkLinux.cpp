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

#include <cstdlib>
#include <algorithm>

#include "xbmc/messaging/ApplicationMessenger.h"
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef TARGET_ANDROID
  #include "network/linux/android-ifaddrs/ifaddrs.h"
#endif
#if defined(TARGET_LINUX)
  #include <linux/if.h>
  #include <linux/wireless.h>
  #include <linux/sockios.h>
  #include <linux/netlink.h>
  #include <linux/rtnetlink.h>
#ifndef _IFADDRS_H_
  #include <ifaddrs.h>
#endif
#else
  #include "network/osx/priv_netlink.h"
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
  #include "network/osx/ioshacks.h"
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
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace KODI::MESSAGING;

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkLinux* network, unsigned int ifa_flags,
                                               struct sockaddr *address, struct sockaddr *netmask,
                                               std::string interfaceName, char interfaceMacAddrRaw[6]) :
  m_interfaceFlags(ifa_flags),
  m_removed(false),
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
  m_address = (struct sockaddr *) malloc(sizeof(struct sockaddr_storage));
  m_netmask = (struct sockaddr *) malloc(sizeof(struct sockaddr_storage));
  memcpy(m_address, address, sizeof(struct sockaddr_storage));
  memcpy(m_netmask, netmask, sizeof(struct sockaddr_storage));
   memcpy(m_interfaceMacAddrRaw, interfaceMacAddrRaw, sizeof(m_interfaceMacAddrRaw));
}

CNetworkInterfaceLinux::~CNetworkInterfaceLinux(void)
{
  free(m_address);
  free(m_netmask);
}

std::string& CNetworkInterfaceLinux::GetName(void)
{
   return m_interfaceName;
}

bool CNetworkInterfaceLinux::IsWireless()
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

bool CNetworkInterfaceLinux::IsEnabled()
{
   return true;
}

bool CNetworkInterfaceLinux::IsConnected()
{
   // ignore loopback
   if (IsRemoved() || m_interfaceFlags & IFF_LOOPBACK)
     return false;

   // Don't add IFF_LOWER_UP - looks it is driver dependent on Linux
   // and missing on running interfaces on OSX (Maverick tested)
   unsigned int needFlags = IFF_RUNNING; //IFF_LOWER_UP
   bool iRunning = (m_interfaceFlags & needFlags) == needFlags;

  return iRunning;
}

std::string CNetworkInterfaceLinux::GetMacAddress()
{
  return m_interfaceMacAdr;
}

void CNetworkInterfaceLinux::GetMacAddressRaw(char rawMac[6])
{
  memcpy(rawMac, m_interfaceMacAddrRaw, 6);
}

std::string CNetworkInterfaceLinux::GetCurrentIPAddress(void)
{
   return CNetwork::GetIpStr(m_address);
}

std::string CNetworkInterfaceLinux::GetCurrentNetmask(void)
{
   std::string result;

   if (isIPv4())
    result = CNetwork::GetIpStr(m_netmask);
   else
    result = StringUtils::Format("%u", CNetwork::PrefixLength(m_netmask));
   return result;
}

std::string CNetworkInterfaceLinux::GetCurrentWirelessEssId(void)
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

std::string CNetworkInterfaceLinux::GetCurrentDefaultGateway(void)
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
   FILE* fp = isIPv4() ? fopen("/proc/net/route", "r") : fopen("/proc/net/ipv6_route", "r");
   if (!fp)
   {
     // TBD: Error
     return result;
   }

   char* line = NULL;
   char iface[16];
   char dst[128];
   char gateway[128];
   unsigned int metric;
   unsigned int metric_prev = -1;
   size_t linel = 0;
   int n;
   int linenum = 0;
   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      // skip first two lines
      if (isIPv4() && linenum++ < 1)
         continue;

      // search where the word begins
      if (isIPv4())
        n = sscanf(line,  "%15s %127s %127s",
           iface, dst, gateway);
      else
        n = sscanf(line,  "%*32s %*2s %32s %*2s %32s %8x %*8s %*8s %*8s %8s",
           dst, gateway, &metric, iface);

      if (n < 3)
         continue;

      if (strcmp(iface, m_interfaceName.c_str()) == 0 &&
         (strcmp(dst, "00000000") == 0 || strcmp(dst, "00000000000000000000000000000000") == 0) &&
          strcmp(gateway, "00000000") != 0 && strcmp(gateway, "00000000000000000000000000000000") != 0)
      {
         if (isIPv4())
         {
           struct in_addr in;
           sscanf(gateway, "%8x", &in.s_addr);
           result = inet_ntoa(in);
         }
         else if (metric < metric_prev)
         {
           metric_prev = metric;
           std::string tstr = gateway;
           for(int i = 7; i > 0; i--)
             tstr.insert(tstr.begin() + i*4, ':');
           result = CNetwork::CanonizeIPv6(tstr);
         }
      }
   }
   free(line);
   fclose(fp);
#endif

   return result;
}

CNetworkLinux::CNetworkLinux(void)
{
   m_sock = socket(AF_INET, SOCK_DGRAM, 0);
   queryInterfaceList();
   RegisterWatcher(WatcherProcess);
   CApplicationMessenger::GetInstance().PostMsg(TMSG_NETWORKMESSAGE, CNetwork::SERVICES_UP, 0);
}

CNetworkLinux::~CNetworkLinux(void)
{
  if (m_sock != -1)
    close(CNetworkLinux::m_sock);

  CSingleLock lock(m_lockInterfaces);
  InterfacesClear();
  DeleteRemoved();
}

void CNetworkLinux::DeleteRemoved(void)
{
  m_interfaces.remove_if(IsRemoved);
}

void CNetworkLinux::InterfacesClear(void)
{
  for (auto &&iface: m_interfaces)
    ((CNetworkInterfaceLinux*)iface)->SetRemoved();
}

std::forward_list<CNetworkInterface*>& CNetworkLinux::GetInterfaceList(void)
{
  CSingleLock lock(m_lockInterfaces);
  return m_interfaces;
}

//! @bug
//! Overwrite the GetFirstConnectedInterface and requery
//! the interface list if no connected device is found
//! this fixes a bug when no network is available after first start of xbmc
//! and the interface comes up during runtime
CNetworkInterface* CNetworkLinux::GetFirstConnectedInterface(void)
{
    CNetworkInterface *pNetIf=CNetwork::GetFirstConnectedInterface();
    
    // no connected Interfaces found? - requeryInterfaceList
    if (!pNetIf)
    {
        CLog::Log(LOGDEBUG,"%s no connected interface found - requery list",__FUNCTION__);        
        queryInterfaceList();        
        //retry finding a connected if
        pNetIf = CNetwork::GetFirstConnectedInterface();
    }
    
    return pNetIf;
}

void CNetworkLinux::GetMacAddress(struct ifaddrs *tif, char *mac)
{
#if !defined(TARGET_LINUX)
#if !defined(IFT_ETHER)
#define IFT_ETHER 0x6/* Ethernet CSMACD */
#endif
  if (((const struct sockaddr_dl*) tif->ifa_addr)->sdl_type == IFT_ETHER)
  {
    const struct sockaddr_dl *dlAddr = (const struct sockaddr_dl *) tif->ifa_addr;
    const uint8_t *base = (const uint8_t*) &dlAddr->sdl_data[dlAddr->sdl_nlen];

    if( dlAddr->sdl_alen > 5 )
      memcpy(mac, base, 6);
  }
#else
  struct ifreq ifr;
  strcpy(ifr.ifr_name, tif->ifa_name);
  if (ioctl(GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
#endif
}

CNetworkInterfaceLinux *CNetworkLinux::Exists(const struct sockaddr *addr, const struct sockaddr *mask, const std::string &name)
{
  for (auto &&iface: m_interfaces)
    if (((CNetworkInterfaceLinux*)iface)->Exists(addr, mask, name))
      return (CNetworkInterfaceLinux*)iface;

  return NULL;
}

bool CNetworkLinux::queryInterfaceList()
{
  bool change = false;

  CSingleLock lock(m_lockInterfaces);

  // Query the list of interfaces.
  struct ifaddrs *list;
  if (getifaddrs(&list) < 0)
    return false;

#if !defined(TARGET_LINUX)
  std::map<std::string,struct ifaddrs> t_hwaddrs;
#endif

  InterfacesClear();

  // find last IPv4 record, we will add new interfaces
  // right after this one (to keep IPv4 in front).
  auto pos = m_interfaces.before_begin();
  for (auto &&iface : m_interfaces)
  {
    if (iface && iface->isIPv6())
      break;
    ++pos;
  }

   struct ifaddrs *cur;
   for(cur = list; cur != NULL; cur = cur->ifa_next)
   {
     std::string name = cur->ifa_name;
#if !defined(TARGET_LINUX)
     if(cur->ifa_addr->sa_family == AF_LINK)
     {
       struct ifaddrs &t = *cur;
       t_hwaddrs[name] = t;
     }
#endif

     if(!cur->ifa_addr ||
        (cur->ifa_addr->sa_family != AF_INET &&
         cur->ifa_addr->sa_family != AF_INET6))
       continue;

     if(!(cur->ifa_flags & IFF_UP))
       continue;

     // Add the interface.
     std::string addr = CNetwork::GetIpStr(cur->ifa_addr);
     std::string mask = CNetwork::GetIpStr(cur->ifa_netmask);

     if(addr.empty() || mask.empty())
       continue;

     CNetworkInterfaceLinux *iface = Exists(cur->ifa_addr, cur->ifa_netmask, name);
     if (iface)
     {
       iface->SetRemoved(false);
       iface->m_interfaceFlags = cur->ifa_flags;
       continue;
     }

     char macAddrRaw[6] = {0};
#if !defined(TARGET_LINUX)
     GetMacAddress(&t_hwaddrs[name], macAddrRaw);
#else
     GetMacAddress(cur, macAddrRaw);
#endif

     CNetworkInterfaceLinux *i = new CNetworkInterfaceLinux(this, cur->ifa_flags, cur->ifa_addr,
                                                            cur->ifa_netmask, name, macAddrRaw);

     m_interfaces.insert_after(pos, i);
     if (i->isIPv4())
       pos++;
     change = true;
   }

   freeifaddrs(list);

   change |= std::count_if(m_interfaces.begin(), m_interfaces.end(), IsRemoved);
   DeleteRemoved();

   return change;
}

std::vector<std::string> CNetworkLinux::GetNameServers(void)
{
   std::vector<std::string> result;

#if defined(TARGET_ANDROID)
  char nameserver[PROP_VALUE_MAX];

  if (__system_property_get("net.dns1",nameserver))
    result.push_back(nameserver);
  if (__system_property_get("net.dns2",nameserver))
    result.push_back(nameserver);
  if (__system_property_get("net.dns3",nameserver))
    result.push_back(nameserver);
#else
   int res = res_init();

   for (int i = 0; i < MAXNS && !res; i++)
   {
     std::string strIp = CNetwork::GetIpStr((struct sockaddr *)&_res.nsaddr_list[i]);
     if (!strIp.empty())
       result.push_back(strIp);

#if !defined(TARGET_DARWIN)
     strIp = CNetwork::GetIpStr((struct sockaddr *)_res._u._ext.nsaddrs[i]);
     if (!strIp.empty())
       result.push_back(strIp);
#endif

     if (_res.nscount
#if !defined(TARGET_DARWIN)
         + _res._u._ext.nscount6
#endif
         == result.size())
       break;
   }

#endif
  if (result.empty())
       CLog::Log(LOGWARNING, "Unable to determine nameserver");

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

bool CNetworkLinux::PingHostImpl(const std::string &target, unsigned int timeout_ms)
{
  bool isIPv6 = CNetwork::ConvIPv6(target);
  if (isIPv6 && !SupportsIPv6())
    return false;

  char cmd_line [64];
  std::string ping = isIPv6 ? "ping6" : "ping";

#if defined (TARGET_DARWIN_IOS) // no timeout option available
  sprintf(cmd_line, "%s -c 1 %s", ping.c_str(), target.c_str());
#elif defined (TARGET_DARWIN) || defined (TARGET_FREEBSD)
  sprintf(cmd_line, "%s -c 1 -%s %d %s", ping.c_str(), isIPv6 ? "i" : "t", timeout_ms / 1000 + (timeout_ms % 1000) != 0, target.c_str());
#else
  sprintf(cmd_line, "%s -c 1 -w %d %s", ping.c_str(), timeout_ms / 1000 + (timeout_ms % 1000) != 0, target.c_str());
#endif

  int status = system (cmd_line);

  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = %d, errno = %d : '%s'", status, errno, cmd_line);

  return result == 0;
}

bool CNetworkInterfaceLinux::GetHostMacAddress(unsigned long host_ip, std::string& mac)
{
  if (m_network->GetFirstConnectedFamily() == AF_INET6 || isIPv6())
    return false;

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
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
#else
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
#endif
}

std::vector<NetworkAccessPoint> CNetworkInterfaceLinux::GetAccessPoints(void)
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

void CNetworkInterfaceLinux::GetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
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

void CNetworkInterfaceLinux::SetSettings(NetworkAssignment& assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
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

void CNetworkInterfaceLinux::WriteSettings(FILE* fw, NetworkAssignment assignment, std::string& ipAddress, std::string& networkMask, std::string& defaultGateway, std::string& essId, std::string& key, EncMode& encryptionMode)
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

void WatcherProcess(void *caller)
{
  struct sockaddr_nl addr;
  int fds = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  struct pollfd m_fds = { fds, POLLIN, 0 };
  char msg[4096];
  volatile bool *stopping = ((CNetwork::CNetworkUpdater*)caller)->Stopping();

  memset (&addr, 0, sizeof(struct sockaddr_nl));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = getpid ();
#if defined(TARGET_LINUX)
  addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
                /* RTMGRP_IPV4_IFADDR | RTMGRP_TC | RTMGRP_IPV4_MROUTE |
                   RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_RULE |
                   RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_MROUTE |
                   RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFINFO |
                   RTMGRP_IPV6_PREFIX */
#else
  addr.nl_groups = RTMGRP_LINK;
#endif

  if (-1 == bind(fds, (const struct sockaddr *) &addr, sizeof(struct sockaddr)))
  {
    close(fds);
    fds = 0;
  }
  else
    fcntl(fds, F_SETFL, O_NONBLOCK);

  while(!*stopping)
    if (poll(&m_fds, 1, 1000) > 0)
    {
      while (!*stopping && fds && recv(fds, &msg, sizeof(msg), 0) > 0);
      if (*stopping)
        continue;

      if (!fds)
        ((CNetwork::CNetworkUpdater*)caller)->Sleep(5000);
      else
        ((CNetwork::CNetworkUpdater*)caller)->Sleep(1000);

      if (stopping || !g_application.getNetwork().ForceRereadInterfaces())
        continue;

      CLog::Log(LOGINFO, "Interfaces change %s", __FUNCTION__);
      CApplicationMessenger::GetInstance().SendMsg(TMSG_NETWORKMESSAGE, CNetwork::NETWORK_CHANGED, 0);
    }
}

