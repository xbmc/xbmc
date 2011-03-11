/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __APPLE__
  #include <linux/if.h>
  #include <linux/wireless.h>
  #include <linux/sockios.h>
#endif
#include <errno.h>
#include <resolv.h>
#if defined(__APPLE__)
  #include <sys/sockio.h>
  #include <net/if.h>
  #include <ifaddrs.h>
#else
  #include <net/if_arp.h>
#endif
#include "PlatformDefs.h"
#include "NetworkLinux.h"
#include "Util.h"
#include "utils/log.h"

using namespace std;

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkLinux* network, CStdString interfaceName)

{
   m_network = network;
   m_interfaceName = interfaceName;
}

CNetworkInterfaceLinux::~CNetworkInterfaceLinux(void)
{
}

CStdString& CNetworkInterfaceLinux::GetName(void)
{
   return m_interfaceName;
}

bool CNetworkInterfaceLinux::IsWireless()
{
#ifdef __APPLE__
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
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
}

bool CNetworkInterfaceLinux::IsConnected()
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

CStdString CNetworkInterfaceLinux::GetMacAddress()
{
   CStdString result = "";

#ifdef __APPLE__
   result.Format("00:00:00:00:00:00");
#else
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
   {
      result.Format("%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
         ifr.ifr_hwaddr.sa_data[0],
         ifr.ifr_hwaddr.sa_data[1],
         ifr.ifr_hwaddr.sa_data[2],
         ifr.ifr_hwaddr.sa_data[3],
         ifr.ifr_hwaddr.sa_data[4],
         ifr.ifr_hwaddr.sa_data[5]);
   }
#endif

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentIPAddress(void)
{
   CStdString result = "";

   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) >= 0)
   {
      result = inet_ntoa((*((struct sockaddr_in *)&ifr.ifr_addr)).sin_addr);
   }

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentNetmask(void)
{
   CStdString result = "";

   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   ifr.ifr_addr.sa_family = AF_INET;
   if (ioctl(m_network->GetSocket(), SIOCGIFNETMASK, &ifr) >= 0)
   {
      result = inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
   }

   return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentWirelessEssId(void)
{
   CStdString result = "";

#ifndef __APPLE__
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

CStdString CNetworkInterfaceLinux::GetCurrentDefaultGateway(void)
{
   CStdString result = "";

#ifndef __APPLE__
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
      n = sscanf(line,  "%16s %128s %128s",
         iface, dst, gateway);

      if (n < 3)
         continue;

      if (strcmp(iface, m_interfaceName.c_str()) == 0 &&
          strcmp(dst, "00000000") == 0 &&
          strcmp(gateway, "00000000") != 0)
      {
         unsigned char gatewayAddr[4];
         int len = CNetwork::ParseHex(gateway, gatewayAddr);
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

CNetworkLinux::CNetworkLinux(void)
{
   m_sock = socket(AF_INET, SOCK_DGRAM, 0);
   queryInterfaceList();
}

CNetworkLinux::~CNetworkLinux(void)
{
  if (m_sock != -1)
    close(CNetworkLinux::m_sock);

  vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
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

#if defined(__APPLE__) && defined(__arm__)
// on iOS, overwrite the GetFirstConnectedInterface and requery
// the interface list if no connected device is found
// this fixes a bug when no network is available after first start of xbmc after reboot
CNetworkInterface* CNetworkLinux::GetFirstConnectedInterface(void)
{
    CNetworkInterface *pNetIf=CNetwork::GetFirstConnectedInterface();
    
    // no connected Interfaces found? - requeryInterfaceList
    if (!pNetIf)
    {
        CLog::Log(LOGDEBUG,"%s no connected if found - requery if list",__FUNCTION__);        
        queryInterfaceList();        
        //retry finding a connected if
        pNetIf = CNetwork::GetFirstConnectedInterface();
    }
    
    return pNetIf;
}
#endif

void CNetworkLinux::queryInterfaceList()
{
   m_interfaces.clear();

#ifdef __APPLE__

   // Query the list of interfaces.
   struct ifaddrs *list;
   if (getifaddrs(&list) < 0)
     return;

   struct ifaddrs *cur;
   for(cur = list; cur != NULL; cur = cur->ifa_next)
   {
     if(cur->ifa_addr->sa_family != AF_INET)
       continue;

     // Add the interface.
     m_interfaces.push_back(new CNetworkInterfaceLinux(this, cur->ifa_name));
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
      CStdString interfaceName = p;
      m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName));
   }
   free(line);
   fclose(fp);
#endif
}

std::vector<CStdString> CNetworkLinux::GetNameServers(void)
{
   std::vector<CStdString> result;
#ifndef __APPLE__
   res_init();

   for (int i = 0; i < _res.nscount; i ++)
   {
      CStdString ns = inet_ntoa(((struct sockaddr_in *)&_res.nsaddr_list[0])->sin_addr);
      result.push_back(ns);
   }
#endif
   return result;
}

void CNetworkLinux::SetNameServers(std::vector<CStdString> nameServers)
{
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
      // TODO:
   }
}

std::vector<NetworkAccessPoint> CNetworkInterfaceLinux::GetAccessPoints(void)
{
   std::vector<NetworkAccessPoint> result;

   if (!IsWireless())
      return result;

#ifndef __APPLE__
   // Query the wireless extentsions version number. It will help us when we
   // parse the resulting events
   struct iwreq iwr;
   char rangebuffer[sizeof(iw_range) * 2];    /* Large enough */
   struct iw_range*  range = (struct iw_range*) rangebuffer;

   memset(rangebuffer, 0, sizeof(rangebuffer));
   iwr.u.data.pointer = (caddr_t) rangebuffer;
   iwr.u.data.length = sizeof(rangebuffer);
   iwr.u.data.flags = 0;
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   if (ioctl(m_network->GetSocket(), SIOCGIWRANGE, &iwr) < 0)
   {
      CLog::Log(LOGWARNING, "%-8.16s  Driver has no Wireless Extension version information.",
         GetName().c_str());
      return result;
   }

   // Scan for wireless access points
   memset(&iwr, 0, sizeof(iwr));
   strncpy(iwr.ifr_name, GetName().c_str(), IFNAMSIZ);
   if (ioctl(m_network->GetSocket(), SIOCSIWSCAN, &iwr) < 0)
   {
      CLog::Log(LOGWARNING, "Cannot initiate wireless scan: ioctl[SIOCSIWSCAN]: %s", strerror(errno));
      return result;
   }

   // Get the results of the scanning. Three scenarios:
   //    1. There's not enough room in the result buffer (E2BIG)
   //    2. The scanning is not complete (EAGAIN) and we need to try again. We cap this with 15 seconds.
   //    3. Were'e good.
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

   size_t len = iwr.u.data.length;
   char* pos = (char *) res_buf;
   char* end = (char *) res_buf + len;
   char* custom;
   struct iw_event iwe_buf, *iwe = &iwe_buf;

   CStdString essId;
   int quality = 0;
   EncMode encryption = ENC_NONE;
   bool first = true;

   while (pos + IW_EV_LCP_LEN <= end)
   {
      /* Event data may be unaligned, so make a local, aligned copy
       * before processing. */
      memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
      if (iwe->len <= IW_EV_LCP_LEN)
         break;

      custom = pos + IW_EV_POINT_LEN;
      if (range->we_version_compiled > 18 &&
          (iwe->cmd == SIOCGIWESSID ||
           iwe->cmd == SIOCGIWENCODE ||
           iwe->cmd == IWEVGENIE ||
           iwe->cmd == IWEVCUSTOM))
      {
         /* Wireless extentsions v19 removed the pointer from struct iw_point */
         char *dpos = (char *) &iwe_buf.u.data.length;
         int dlen = dpos - (char *) &iwe_buf;
         memcpy(dpos, pos + IW_EV_LCP_LEN, sizeof(struct iw_event) - dlen);
      }
      else
      {
         memcpy(&iwe_buf, pos, sizeof(struct iw_event));
         custom += IW_EV_POINT_OFF;
      }

      switch (iwe->cmd)
      {
         case SIOCGIWAP:
            if (first)
               first = false;
            else
               result.push_back(NetworkAccessPoint(essId, quality, encryption));
               encryption = ENC_NONE;
            break;

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

         case IWEVQUAL:
             quality = iwe->u.qual.qual;
             break;

         case SIOCGIWENCODE:
             if (!(iwe->u.data.flags & IW_ENCODE_DISABLED) && encryption == ENC_NONE)
                encryption = ENC_WEP;
             break;

         case IWEVGENIE:
         {
            int offset = 0;
            while (offset <= iwe_buf.u.data.length)
            {
               switch ((unsigned char)custom[offset])
               {
                  case 0xdd: /* WPA1 */
                     if (encryption != ENC_WPA2)
                        encryption = ENC_WPA;
                     break;
                  case 0x30: /* WPA2 */
                     encryption = ENC_WPA2;
               }

               offset += custom[offset+1] + 2;
            }
         }
      }

      pos += iwe->len;
   }

   if (!first)
      result.push_back(NetworkAccessPoint(essId, quality, encryption));

   free(res_buf);
   res_buf = NULL;
#endif

   return result;
}

void CNetworkInterfaceLinux::GetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
   ipAddress = "0.0.0.0";
   networkMask = "0.0.0.0";
   defaultGateway = "0.0.0.0";
   essId = "";
   key = "";
   encryptionMode = ENC_NONE;
   assignment = NETWORK_DISABLED;

#ifndef __APPLE__
   FILE* fp = fopen("/etc/network/interfaces", "r");
   if (!fp)
   {
      // TODO
      return;
   }

   char* line = NULL;
   size_t linel = 0;
   CStdString s;
   bool foundInterface = false;

   while (getdelim(&line, &linel, '\n', fp) > 0)
   {
      vector<CStdString> tokens;

      s = line;
      s.TrimLeft(" \t").TrimRight(" \n");

      // skip comments
      if (s.length() == 0 || s.GetAt(0) == '#')
         continue;

      // look for "iface <interface name> inet"
      CUtil::Tokenize(s, tokens, " ");
      if (!foundInterface &&
          tokens.size() >=3 &&
          tokens[0].Equals("iface") &&
          tokens[1].Equals(GetName()) &&
          tokens[2].Equals("inet"))
      {
         if (tokens[3].Equals("dhcp"))
         {
            assignment = NETWORK_DHCP;
            foundInterface = true;
         }
         if (tokens[3].Equals("static"))
         {
            assignment = NETWORK_STATIC;
            foundInterface = true;
         }
      }

      if (foundInterface && tokens.size() == 2)
      {
         if (tokens[0].Equals("address")) ipAddress = tokens[1];
         else if (tokens[0].Equals("netmask")) networkMask = tokens[1];
         else if (tokens[0].Equals("gateway")) defaultGateway = tokens[1];
         else if (tokens[0].Equals("wireless-essid")) essId = tokens[1];
         else if (tokens[0].Equals("wireless-key"))
         {
            key = tokens[1];
            if (key.length() > 2 && key[0] == 's' && key[1] == ':')
               key.erase(0, 2);
            encryptionMode = ENC_WEP;
         }
         else if (tokens[0].Equals("wpa-ssid")) essId = tokens[1];
         else if (tokens[0].Equals("wpa-proto") && tokens[1].Equals("WPA")) encryptionMode = ENC_WPA;
         else if (tokens[0].Equals("wpa-proto") && tokens[1].Equals("WPA2")) encryptionMode = ENC_WPA2;
         else if (tokens[0].Equals("wpa-psk")) key = tokens[1];
         else if (tokens[0].Equals("auto") || tokens[0].Equals("iface") || tokens[0].Equals("mapping")) break;
      }
   }
   free(line);

   // Fallback in case wpa-proto is not set
   if (key != "" && encryptionMode == ENC_NONE)
      encryptionMode = ENC_WPA;

   fclose(fp);
#endif
}

void CNetworkInterfaceLinux::SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
#ifndef __APPLE__
   FILE* fr = fopen("/etc/network/interfaces", "r");
   if (!fr)
   {
      // TODO
      return;
   }

   FILE* fw = fopen("/tmp/interfaces.temp", "w");
   if (!fw)
   {
      // TODO
      fclose(fr);
      return;
   }

   char* line = NULL;
   size_t linel = 0;
   CStdString s;
   bool foundInterface = false;
   bool dataWritten = false;

   while (getdelim(&line, &linel, '\n', fr) > 0)
   {
      vector<CStdString> tokens;

      s = line;
      s.TrimLeft(" \t").TrimRight(" \n");

      // skip comments
      if (!foundInterface && (s.length() == 0 || s.GetAt(0) == '#'))
      {
        fprintf(fw, "%s", line);
        continue;
      }

      // look for "iface <interface name> inet"
      CUtil::Tokenize(s, tokens, " ");
      if (tokens.size() == 2 &&
          tokens[0].Equals("auto") &&
          tokens[1].Equals(GetName()))
      {
         continue;
      }
      else if (!foundInterface &&
          tokens.size() == 4 &&
          tokens[0].Equals("iface") &&
          tokens[1].Equals(GetName()) &&
          tokens[2].Equals("inet"))
      {
         foundInterface = true;
         WriteSettings(fw, assignment, ipAddress, networkMask, defaultGateway, essId, key, encryptionMode);
         dataWritten = true;
      }
      else if (foundInterface &&
               tokens.size() == 4 &&
               tokens[0].Equals("iface"))
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
      // TODO
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

void CNetworkInterfaceLinux::WriteSettings(FILE* fw, NetworkAssignment assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
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

