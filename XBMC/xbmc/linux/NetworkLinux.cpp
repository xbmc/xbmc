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
#ifdef __APPLE__
#include <sys/sockio.h>
#include <net/if.h>
#include <ifaddrs.h>
#endif
#include <net/if_arp.h>
#include "PlatformDefs.h"
#include "NetworkLinux.h"
#include "Util.h"
#include "log.h"

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
   memset(&ifr,0,sizeof(struct ifreq));
   strcpy(ifr.ifr_name, m_interfaceName.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   // ignore loopback
   int iRunning = ( (ifr.ifr_flags & IFF_RUNNING) && (!(ifr.ifr_flags & IFF_LOOPBACK)));

   if (ioctl(m_network->GetSocket(), SIOCGIFADDR, &ifr) < 0)
      return false;

   // return only interfaces which has ip address
   return iRunning && (*(int*)&ifr.ifr_addr.sa_data != 0) ;
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
      result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
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
      result = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
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

   if (line)
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

      // make sure the device has ethernet encapsulation
      struct ifreq ifr;
      strcpy(ifr.ifr_name, p);
      if (ioctl(GetSocket(), SIOCGIFHWADDR, &ifr) >= 0 && ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)
      {
         // save the result
         CStdString interfaceName = p;
     m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName));
      }
   }

   if (line)
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

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (con != NULL)
  {
    CStdString NetworkPath;
    NetworkPath.Format("%s/%s", NM_DBUS_PATH_DEVICES, m_interfaceName.c_str());

    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, NetworkPath.c_str(), NM_DBUS_INTERFACE, "getProperties");
	  if (msg)
    {
      DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
      if (reply)
      {
        const char*   obj_path          = NULL;
        const char*   interface         = NULL;
        NMDeviceType  type              = DEVICE_TYPE_UNKNOWN;
        const char*   udi               = NULL;
        dbus_bool_t   active            = false;
        NMActStage    act_stage         = NM_ACT_STAGE_UNKNOWN;
        const char*   ipv4_address      = NULL;
        const char*   subnetmask        = NULL;
        const char*   broadcast         = NULL;
        const char*   hw_address        = NULL;
        const char*   route             = NULL;
        const char*   pri_dns           = NULL;
        const char*   sec_dns           = NULL;
        dbus_int32_t  mode              = 0;
        dbus_int32_t  strength          = -1;
        dbus_bool_t   link_active       = false;
        dbus_int32_t  speed             = 0;
        dbus_uint32_t capabilities      = NM_DEVICE_CAP_NONE;
        dbus_uint32_t capabilities_type = NM_DEVICE_CAP_NONE;
        char**        networks          = NULL;
        int           num_networks      = 0;
        const char*   active_net_path   = NULL;

	      if (dbus_message_get_args (reply, NULL, 
            DBUS_TYPE_OBJECT_PATH,             &obj_path,
						DBUS_TYPE_STRING,                  &interface,
						DBUS_TYPE_UINT32,                  &type,
						DBUS_TYPE_STRING,                  &udi,
						DBUS_TYPE_BOOLEAN,                 &active,
						DBUS_TYPE_UINT32,                  &act_stage,
						DBUS_TYPE_STRING,                  &ipv4_address,
						DBUS_TYPE_STRING,                  &subnetmask,
						DBUS_TYPE_STRING,                  &broadcast,
						DBUS_TYPE_STRING,                  &hw_address,
						DBUS_TYPE_STRING,                  &route,
						DBUS_TYPE_STRING,                  &pri_dns,
						DBUS_TYPE_STRING,                  &sec_dns,
						DBUS_TYPE_INT32,                   &mode,
						DBUS_TYPE_INT32,                   &strength,
						DBUS_TYPE_BOOLEAN,                 &link_active,
						DBUS_TYPE_INT32,                   &speed,
						DBUS_TYPE_UINT32,                  &capabilities,
						DBUS_TYPE_UINT32,                  &capabilities_type,
						DBUS_TYPE_STRING,                  &active_net_path,
						DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &networks, &num_networks, DBUS_TYPE_INVALID))
        {
          for (unsigned int i = 0; i < num_networks; i++)
            AddNetworkAccessPoint(result, networks[i], con);

        }
    		dbus_free_string_array (networks);
      	dbus_message_unref (reply);
      }
		  dbus_message_unref (msg);
	  }
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", error.message);
    dbus_error_free (&error);
  }

  return result;
}

void CNetworkInterfaceLinux::AddNetworkAccessPoint(std::vector<NetworkAccessPoint> &apv, const char *NetworkPath, DBusConnection *con)
{
  DBusError error;
  dbus_error_init (&error);
  DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, NetworkPath, NM_DBUS_INTERFACE, "getProperties");
  if (msg)
  {
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
    if (reply)
    {
      const char*  obj_path     = NULL;
      const char*  essid        = NULL;
      const char*  hw_address   = NULL;
      dbus_int32_t strength     = -1;
      double       freq         = 0;
      dbus_int32_t rate         = 0;
      dbus_int32_t mode         = 0;
      dbus_int32_t capabilities = NM_802_11_CAP_NONE;
      dbus_bool_t  broadcast    = true;

      if (dbus_message_get_args (reply, NULL, 
              DBUS_TYPE_OBJECT_PATH, &obj_path,
              DBUS_TYPE_STRING,      &essid,
              DBUS_TYPE_STRING,      &hw_address,
              DBUS_TYPE_INT32,       &strength,
              DBUS_TYPE_DOUBLE,      &freq,
              DBUS_TYPE_INT32,       &rate,
              DBUS_TYPE_INT32,       &mode,
              DBUS_TYPE_INT32,       &capabilities,
              DBUS_TYPE_BOOLEAN,     &broadcast, DBUS_TYPE_INVALID))
      {
        CLog::Log(LOGERROR, "DBus: Failed to get getProperties of Network on %s", NetworkPath);
      }
      EncMode encryption = ENC_NONE;
      if (capabilities & NM_802_11_CAP_PROTO_WEP)
        encryption = ENC_WEP;
      else if (capabilities & NM_802_11_CAP_PROTO_WPA)
        encryption = ENC_WPA;
      else if (capabilities & NM_802_11_CAP_PROTO_WPA2)
        encryption = ENC_WPA2;

      CStdString essId = essid;
      NetworkAccessPoint ap(essId, (int)strength, encryption);
      apv.push_back(ap);

      dbus_message_unref(reply);
    }
    dbus_message_unref(msg);
  }
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
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (con != NULL)
  {
    CStdString NetworkPath;
    NetworkPath.Format("%s/%s", NM_DBUS_PATH_DEVICES, m_interfaceName.c_str());

    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, NetworkPath.c_str(), NM_DBUS_INTERFACE, "getProperties");
	  if (msg)
    {
      DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
      if (reply)
      {
        const char*   obj_path          = NULL;
        const char*   interface         = NULL;
        NMDeviceType  type              = DEVICE_TYPE_UNKNOWN;
        const char*   udi               = NULL;
        dbus_bool_t   active            = false;
        NMActStage    act_stage         = NM_ACT_STAGE_UNKNOWN;
        const char*   ipv4_address      = NULL;
        const char*   subnetmask        = NULL;
        const char*   broadcast         = NULL;
        const char*   hw_address        = NULL;
        const char*   route             = NULL;
        const char*   pri_dns           = NULL;
        const char*   sec_dns           = NULL;
        dbus_int32_t  mode              = 0;
        dbus_int32_t  strength          = -1;
        dbus_bool_t   link_active       = false;
        dbus_int32_t  speed             = 0;
        dbus_uint32_t capabilities      = NM_DEVICE_CAP_NONE;
        dbus_uint32_t capabilities_type = NM_DEVICE_CAP_NONE;
        char**        networks          = NULL;
        int           num_networks      = 0;
        const char*   active_net_path   = NULL;

	      if (dbus_message_get_args (reply, NULL, 
            DBUS_TYPE_OBJECT_PATH,             &obj_path,
						DBUS_TYPE_STRING,                  &interface,
						DBUS_TYPE_UINT32,                  &type,
						DBUS_TYPE_STRING,                  &udi,
						DBUS_TYPE_BOOLEAN,                 &active,
						DBUS_TYPE_UINT32,                  &act_stage,
						DBUS_TYPE_STRING,                  &ipv4_address,
						DBUS_TYPE_STRING,                  &subnetmask,
						DBUS_TYPE_STRING,                  &broadcast,
						DBUS_TYPE_STRING,                  &hw_address,
						DBUS_TYPE_STRING,                  &route,
						DBUS_TYPE_STRING,                  &pri_dns,
						DBUS_TYPE_STRING,                  &sec_dns,
						DBUS_TYPE_INT32,                   &mode,
						DBUS_TYPE_INT32,                   &strength,
						DBUS_TYPE_BOOLEAN,                 &link_active,
						DBUS_TYPE_INT32,                   &speed,
						DBUS_TYPE_UINT32,                  &capabilities,
						DBUS_TYPE_UINT32,                  &capabilities_type,
						DBUS_TYPE_STRING,                  &active_net_path,
						DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &networks, &num_networks, DBUS_TYPE_INVALID))
        {
          ipAddress = ipv4_address;
          networkMask = subnetmask;
          defaultGateway = route;
          if (type == DEVICE_TYPE_802_11_WIRELESS)
          {
            std::vector<NetworkAccessPoint> result;
            AddNetworkAccessPoint(result, active_net_path, con);
            //         key = "";
            essId = result[0].getEssId();
            encryptionMode = result[0].getEncryptionMode();
          }
          assignment = NETWORK_DHCP;
        }
    		dbus_free_string_array (networks);
      	dbus_message_unref (reply);
      }
		  dbus_message_unref (msg);
	  }
  }
  {
    CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", error.message);
    dbus_error_free (&error);
  }
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

   if (line)
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

   CLog::Log(LOGINFO, "Stopping interface %s", GetName().c_str());
   std::string cmd = "/sbin/ifdown " + GetName();
   system(cmd.c_str());

   if (assignment != NETWORK_DISABLED)
   {
      CLog::Log(LOGINFO, "Starting interface %s", GetName().c_str());
      cmd = "/sbin/ifup " + GetName();
      system(cmd.c_str());
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

/*
int main(void)
{
  CStdString mac;
  CNetworkLinux x;
  x.GetNameServers();
  std::vector<CNetworkInterface*>& ifaces = x.GetInterfaceList();
  CNetworkInterface* i1 = ifaces[0];
  printf("%s en=%d run=%d wi=%d mac=%s ip=%s nm=%s gw=%s ess=%s\n", i1->GetName().c_str(), i1->IsEnabled(), i1->IsConnected(), i1->IsWireless(), i1->GetMacAddress().c_str(), i1->GetCurrentIPAddress().c_str(), i1->GetCurrentNetmask().c_str(), i1->GetCurrentDefaultGateway().c_str(), i1->GetCurrentWirelessEssId().c_str());
  i1 = ifaces[1];
  printf("%s en=%d run=%d wi=%d mac=%s ip=%s nm=%s gw=%s ess=%s\n", i1->GetName().c_str(), i1->IsEnabled(), i1->IsConnected(), i1->IsWireless(), i1->GetMacAddress().c_str(), i1->GetCurrentIPAddress().c_str(), i1->GetCurrentNetmask().c_str(), i1->GetCurrentDefaultGateway().c_str(), i1->GetCurrentWirelessEssId().c_str());

  return 0;
}
*/
