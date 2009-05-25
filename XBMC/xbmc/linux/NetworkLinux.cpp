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

#ifndef WAIT_TIME
#define WAIT_TIME 1000
#endif

using namespace std;

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkLinux* network, CStdString objectPath)

{
  m_network = network;
  m_objectPath = objectPath;
  m_lastUpdate = 0;
  m_DeviceType = NM_DEVICE_TYPE_UNKNOWN;
  m_ConnectionState = NM_DEVICE_STATE_UNKNOWN;

  Update();
}

CNetworkInterfaceLinux::~CNetworkInterfaceLinux(void)
{
}

CStdString& CNetworkInterfaceLinux::GetName(void)
{
   return m_interface;
}

void CNetworkInterfaceLinux::Update()
{
  if (timeGetTime() < (m_lastUpdate + WAIT_TIME))
    return;
//dbus-send --print-reply --system --dest=org.freedesktop.NetworkManager --type=method_call  /org/freedesktop/Hal/devices/net_00_1a_92_e9_d8_0a org.freedesktop.DBus.Properties.GetAll string:'org.freedesktop.NetworkManager.Device'
//Second call uses 'org.freedesktop.NetworkManager.Device.[WIRELESS|WIRED]'
  TStrStrMap properties;
  
  GetAll(properties, m_objectPath.c_str(), NM_DBUS_INTERFACE_DEVICE);
  m_DeviceType      = (NMDeviceType)atoi(properties["DeviceType"].c_str());

  m_interface = properties["Interface"];
  if (IsWireless())
    GetAll(properties, m_objectPath.c_str(), NM_DBUS_INTERFACE_DEVICE_WIRELESS);
  else
    GetAll(properties, m_objectPath.c_str(), NM_DBUS_INTERFACE_DEVICE_WIRED);

  m_MAC = properties["HwAddress"];
  m_ConnectionState = (NMDeviceState)atoi(properties["State"].c_str());

  m_lastUpdate = timeGetTime();
}

void CNetworkInterfaceLinux::GetAll(TStrStrMap& properties, const char *object, const char *interface)
{
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  if (con != NULL)
  {
    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, object, "org.freedesktop.DBus.Properties", "GetAll");
	  if (msg)
    {
      if (dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_INVALID))
      {
        DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
        if (reply)
        {
          DBusMessageIter iter;        
          if (dbus_message_iter_init(reply, &iter))
          {
            if (!dbus_message_has_signature(reply, "a{sv}"))
              CLog::Log(LOGERROR, "DBus: wrong signature on GetAll - should be a{sv} but was %s", dbus_message_iter_get_signature(&iter));
            else
            {
              do
              {
                DBusMessageIter sub;
                dbus_message_iter_recurse(&iter, &sub);
                do
                {
                  DBusMessageIter dict;
                  dbus_message_iter_recurse(&sub, &dict);
                  do
                  {
                    const char *    key     = NULL;
                    CStdString      value;
                    const char *    string  = NULL;
                    dbus_int32_t    int32   = 0;
                    dbus_message_iter_get_basic(&dict, &key);
                    dbus_message_iter_next(&dict);

                    DBusMessageIter variant;
                    dbus_message_iter_recurse(&dict, &variant);
                    int type = dbus_message_iter_get_arg_type(&variant);
                    
                    switch (type)
                    {
                      case DBUS_TYPE_OBJECT_PATH:
                      case DBUS_TYPE_STRING:
                        dbus_message_iter_get_basic(&variant, &string);
                        value.Format("%s", string);
                        break;

                      case DBUS_TYPE_BYTE:
                      case DBUS_TYPE_UINT32:
                      case DBUS_TYPE_INT32:
                        dbus_message_iter_get_basic(&variant, &int32);
                        value.Format("%i", (int)int32);
                        break;
                    }
                    if (value.length() > 0)
                      properties.insert(TStrStrPair(key, value));

                  } while (dbus_message_iter_next(&dict));

                } while (dbus_message_iter_next(&sub));

              } while (dbus_message_iter_next(&iter));
            }
          }
          dbus_message_unref(reply);
        }
        else
          CLog::Log(LOGERROR, "DBus: Failed to get reply");
      }
      dbus_message_unref(msg);
    }
    else
    {
      CLog::Log(LOGERROR, "DBus: Failed to create message: %s", error.message);
      dbus_error_free (&error);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", error.message);
    dbus_error_free (&error);
  }
}

bool CNetworkInterfaceLinux::IsWireless()
{
#ifdef __APPLE__
  return false;
#else
  return m_DeviceType == NM_DEVICE_TYPE_WIFI;
#endif

   return true;
}

bool CNetworkInterfaceLinux::IsEnabled()
{
   struct ifreq ifr;
   strcpy(ifr.ifr_name, m_interface.c_str());
   if (ioctl(m_network->GetSocket(), SIOCGIFFLAGS, &ifr) < 0)
      return false;

   return ((ifr.ifr_flags & IFF_UP) == IFF_UP);
}

bool CNetworkInterfaceLinux::IsConnected()
{
  Update();
  return m_ConnectionState == NM_DEVICE_STATE_ACTIVATED;
}

CStdString CNetworkInterfaceLinux::GetMacAddress()
{
  CStdString result = "";

#ifdef __APPLE__
  result.Format("00:00:00:00:00:00");
#else
  result.Format("%s", m_MAC.c_str());
#endif

  return result;
}

CStdString CNetworkInterfaceLinux::GetCurrentIPAddress(void)
{
  CStdString result = "";

  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interface.c_str());
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
   strcpy(ifr.ifr_name, m_interface.c_str());
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
   strcpy(wrq.ifr_name,  m_interface.c_str());
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

      if (strcmp(iface, m_interface.c_str()) == 0 &&
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
//dbus-send --print-reply --system --dest=org.freedesktop.NetworkManager --type=method_call  /org/freedesktop/NetworkManager org.freedesktop.NetworkManager.GetDevices
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  
  if (con != NULL)
  {
    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, NM_DBUS_PATH, NM_DBUS_INTERFACE, "GetDevices");
	  if (msg)
    {
      DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
      if (reply)
      {
        char** interfaces = NULL;
        int    length     = 0;
        
        if (dbus_message_get_args (reply, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &interfaces, &length, DBUS_TYPE_INVALID))
			  {
          for (int i = 0; i < length; i++)
          {
            m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaces[i]));
          }
          dbus_free_string_array(interfaces);
        }
        dbus_message_unref(reply);
      }
      else
        CLog::Log(LOGERROR, "DBus: Failed to get reply from NetworkManager::GetDevices");

      dbus_message_unref(msg);
    }
    else
    {
      CLog::Log(LOGERROR, "DBus: Failed to create message: %s", error.message);
      dbus_error_free (&error);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "DBus: Could not get system bus: %s", error.message);
    dbus_error_free (&error);
  }
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
// dbus-send --print-reply --system --dest=org.freedesktop.NetworkManager --type=method_call /org/freedesktop/Hal/devices/net_00_13_46_74_37_1c org.freedesktop.NetworkManager.Device.Wireless.GetAccessPoints
  std::vector<NetworkAccessPoint> result;

  if (!IsWireless())
    return result;

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (con != NULL)
  {
    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, m_objectPath.c_str(), NM_DBUS_INTERFACE_DEVICE_WIRELESS, "GetAccessPoints");
	  if (msg)
    {
      DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &error);
      if (reply)
      {
        char** networks     = NULL;
        int    num_networks = 0;

	      if (dbus_message_get_args (reply, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &networks, &num_networks, DBUS_TYPE_INVALID))
        {
          for (int i = 0; i < num_networks; i++)
            AddNetworkAccessPoint(result, networks[i], con);

      		dbus_free_string_array (networks);
        }
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
//dbus-send --print-reply --system --dest=org.freedesktop.NetworkManager --type=method_call  /org/freedesktop/NetworkManager/AccessPoint/5 org.freedesktop.DBus.Properties.GetAll string:'org.freedesktop.NetworkManager.AccessPoint'
  TStrStrMap properties;
  
  GetAll(properties, NetworkPath, NM_DBUS_INTERFACE_ACCESS_POINT);

// CStdString essId = properties["Ssid"]; //Doesn't work atm.
  CStdString essId = NetworkPath;
  int strength = atoi(properties["Strength"]);
  int enc = atoi(properties["WpaFlags"].c_str());
  EncMode encryption = ENC_NONE;

  if (enc & NM_802_11_AP_SEC_PAIR_TKIP && enc & NM_802_11_AP_SEC_PAIR_CCMP)
    encryption = ENC_WPA2;
  else if (enc & NM_802_11_AP_SEC_PAIR_TKIP)
    encryption = ENC_WPA;
  else if (enc & NM_802_11_AP_SEC_PAIR_WEP40 || enc & NM_802_11_AP_SEC_PAIR_WEP104  || atoi(properties["Flags"].c_str()))
    encryption = ENC_WEP;

  if (!properties["Mode"].Equals("1")) // Skip AdHoc connections.
  {
    NetworkAccessPoint ap(essId, (int)strength, encryption);
    apv.push_back(ap);
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
/*        print "  Dev Mode:", self.IW_MODE[self.devpi.Get(NMI, "Mode")]
        wcaps = self.devpi.Get(NMI, "WirelessCapabilities")
        print "  Wifi Capabilities:", bitmask_str(self.NM_802_11_DEVICE_CAP, wcaps)
        for P in ["HwAddress", "Bitrate", "ActiveAccessPoint"]:
            print "  %s: %s" % (P, self.devpi.Get(NMI, P))
        if options.ap:
            print "  Access Points"
            for ap in self.APs():
                ap.Dump()


  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con= dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (con != NULL)
  {
    DBusMessage *msg = dbus_message_new_method_call (NM_DBUS_SERVICE, m_objectPath.c_str(), NM_DBUS_INTERFACE, "getProperties");
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
  }*/
#endif
}

void CNetworkInterfaceLinux::SetSettings(NetworkAssignment& assignment, CStdString& ipAddress, CStdString& networkMask, CStdString& defaultGateway, CStdString& essId, CStdString& key, EncMode& encryptionMode)
{
#ifndef __APPLE__
        /*self.nmi.setActiveDevice(device.opath, ssid_str(conn.Ssid()),
                                    reply_handler=self.silent_handler,
                                    error_handler=self.err_handler,
                                    )

  */
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


int main(void)
{
  CStdString mac;
  CNetworkLinux x;
  x.GetNameServers();
  std::vector<CNetworkInterface*>& ifaces = x.GetInterfaceList();
  CNetworkInterface* i1 = ifaces[0];
  
  CNetworkInterface *wlan = NULL;
  
  for (int i = 0; i < ifaces.size(); i++)
  {
    i1 = ifaces[i];
    printf("%s en=%d run=%d wi=%d mac=%s ip=%s nm=%s gw=%s ess=%s\n", i1->GetName().c_str(), i1->IsEnabled(), i1->IsConnected(), i1->IsWireless(), i1->GetMacAddress().c_str(), i1->GetCurrentIPAddress().c_str(), i1->GetCurrentNetmask().c_str(), i1->GetCurrentDefaultGateway().c_str(), i1->GetCurrentWirelessEssId().c_str());
    
    if (i1->IsWireless())
      wlan = i1;
  }

  if (wlan)
  {
    std::vector<NetworkAccessPoint> aps = wlan->GetAccessPoints();
    printf("WLAN: %s has %i networks\n", wlan->GetName().c_str(), aps.size());
    for (int i = 0; i < aps.size(); i++)
    {
      printf("%s %d\n", aps[i].getEssId().c_str(), aps[i].getEncryptionMode());
    }
  }
  return 0;
}

