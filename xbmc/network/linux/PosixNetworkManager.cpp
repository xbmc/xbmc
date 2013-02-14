/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PosixNetworkManager.h"
#include "PosixConnection.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/Key.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "network/NetworkUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#if defined(TARGET_ANDROID)
#include "android/bionic_supplement/bionic_supplement.h"
#endif

#define ARPHRD_80211    801   // wifi

// CPosixNetworkManager and CPosixConnection rely on the debian/ubuntu method of using
// /etc/network/interfaces and pre-up/post-down scripts to handle bringing connection
// to wired/wireless networks. The pre-up/post-down scripts handle wireless/wpa though
// /etc/network/interfaces extensions "wireless-" and "wpa-". Basically, ifup/ifdown will
// tokenize these as shell vars and passes them to the pre-up/post-down scripts for handling.
// Note: This method,of course, requires root permissions.
//
// /etc/network/interfaces examples:
//    auto wlan0
//    iface wlan0 inet dhcp
//        wireless-essid [ESSID]
//        wireless-mode [MODE]
//
// or
//    auto wlan0
//    iface wlan0 inet dhcp
//        wpa-ssid mynetworkname
//        wpa-psk mysecretpassphrase
//
// Then, 'ifup wlan0' will bring up wlan0 with the proper wifi setup and dhdp fetch.
//
// NOTE: BusyBox will call pre-up/post-down scripts BUT it does not pass $PHASE so the
//   if the script is the same for pre-up/post-down it will not be able to tell what to do.
//   The simple fix is to add the following to /etc/wpa_supplicant/ifupdown.sh
//
//   case $0 in
//       *if-up.d*) PHASE="up";;
//       *if-down.d*) PHASE="down";;
//       *if-pre-up.d*) PHASE="pre-up";;
//       *if-post-down.d*) PHASE="post-down";;
//   esac
//
//   Also you need to merge post-up -> pre-up and pre-down -> post-down as those phases
//   do not exist under Busybox.
//
//
// CPosixNetworkManager and CPosixConnection work by detecting the avaliable network
// interfaces, then for wlan0, doing a wifi scan for access points using ioctl calls.
// A CPosixConnection object is created for each wired interface and each wifi access point.
//  after creation, the 1st two fields are retained as the internal connection name for
//  passphrase look up which is only relevent for wifi conections.
//
// Switching connections is performed by a CPosixConnection method in three steps.
//  1) use ifdown <interface> take down every interface except loopback.
//  2) if the desired connection is wifi, then
//       rewrite /etc/network/interfaces, only changing "wireless-" or "wpa-" items.
//  3) use ifup <interface> to bring up the desired active connection.
//
//  TODO: handle static in addition to dhcp settings.
//

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CPosixNetworkManager::CPosixNetworkManager()
{
  CLog::Log(LOGDEBUG, "NetworkManager: PosixNetworkManager created");
  m_socket = socket(AF_INET, SOCK_DGRAM, 0);
  m_post_failed = false;
}

CPosixNetworkManager::~CPosixNetworkManager()
{
  if (m_socket != -1)
    close(m_socket);
}

bool CPosixNetworkManager::CanManageConnections()
{
  return g_advancedSettings.m_enableNetworkManager;
}

ConnectionList CPosixNetworkManager::GetConnections()
{
  FindNetworkInterfaces(); 
  if (CanManageConnections())
    RestoreSavedConnection();
  else
    RestoreSystemConnection();
  return m_connections;
}

bool CPosixNetworkManager::PumpNetworkEvents(INetworkEventsCallback *callback)
{
  bool result = false;

  // check for a failed startup connection
  if (m_post_failed)
  {
    // since network startup happens before windowing and rendering are up
    // wait until we can retrive WINDOW_DIALOG_KAI_TOAST before posting.
    CGUIDialogKaiToast *toast = (CGUIDialogKaiToast*)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
    if (toast)
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(13279), g_localizeStrings.Get(1001));
       m_post_failed = false;
    }
  }

  // pump  our connection
  for (size_t i = 0; i < m_connections.size(); i++)
  {
    if (((CPosixConnection*)m_connections[i].get())->PumpNetworkEvents())
    {
      //some connection state changed (connected or disconnected)
      if (((CPosixConnection*)m_connections[i].get())->GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
      {
        // callback to CNetworkManager to setup the
        // m_defaultConnection and update GUI state if showing.
        callback->OnConnectionChange(m_connections[i]);
        // callback to start services
        callback->OnConnectionStateChange(NETWORK_CONNECTION_STATE_CONNECTED);
        result = true;
      }
    }
  }

  return result;
}

bool CPosixNetworkManager::SendWakeOnLan(const char *mac)
{
  // Fetch the hardware address
  unsigned char ethaddr[8] = {0};
  if (!CNetworkUtils::in_ether(mac, ethaddr))
  {
    CLog::Log(LOGERROR, "%s - Invalid hardware address specified (%s)", __FUNCTION__, mac);
    return false;
  }

  // Setup the socket
  int packet = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (packet < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to create socket (%s)", __FUNCTION__, strerror (errno));
    return false;
  }

  // Set socket options
  struct sockaddr_in saddr = {0};
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  saddr.sin_port = htons(9);

  unsigned int value = 1;
  if (setsockopt(packet, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(unsigned int)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "%s - Unable to set socket options (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  // Build the magic packet (6 x 0xff + 16 x MAC address)
  unsigned char buf[128] = {0};
  unsigned char *ptr = buf;

  for (int i = 0; i < 6; i++)
    *ptr++ = 0xff;

  for (int j = 0; j < 16; j++)
  {
    for (int i = 0; i < 6; i++)
      *ptr++ = ethaddr[i];
  }

  // Send the magic packet
  if (sendto(packet, (char*)buf, 102, 0, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to send magic packet (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "%s - Magic packet send to '%s'", __FUNCTION__, mac);

  return true;
}

//-----------------------------------------------------------------------
void CPosixNetworkManager::RestoreSavedConnection()
{
  CLog::Log(LOGDEBUG, "NetworkManager: Restoring saved connection");

  std::string saved_name       = g_guiSettings.GetString("network.essid");

  CIPConfig saved_ipconfig;
  saved_ipconfig.m_method      = (IPConfigMethod)g_guiSettings.GetInt("network.method");
  saved_ipconfig.m_address     = g_guiSettings.GetString("network.address");
  saved_ipconfig.m_netmask     = g_guiSettings.GetString("network.netmask");
  saved_ipconfig.m_gateway     = g_guiSettings.GetString("network.gateway");
  saved_ipconfig.m_nameserver  = g_guiSettings.GetString("network.nameserver");

  for (size_t i = 0; i < m_connections.size(); i++)
  {
    std::string connection_name = ((CPosixConnection*)m_connections[i].get())->GetName();
    if (connection_name.find(saved_name) != std::string::npos)
    {
      if (!((CPosixConnection*)m_connections[i].get())->Connect(NULL, saved_ipconfig))
      {
        // best we can do is try an existing system connection
        RestoreSystemConnection();
      }
      break;
    }
  }
}

void CPosixNetworkManager::RestoreSystemConnection()
{
  CLog::Log(LOGDEBUG, "NetworkManager: Defaulting to system connection");
  // nothing to do here, CNetworkManager will activate the first
  // connection with a state of NETWORK_CONNECTION_STATE_CONNECTED;

  // humm, how do we check ?
  // m_post_failed = true;

}

void CPosixNetworkManager::FindNetworkInterfaces()
{
  m_connections.clear();

  FILE *fp = fopen("/proc/net/dev", "r");
  if (!fp)
    return;

  int n, linenum = 0;
  char   *line = NULL;
  size_t linel = 0;
  char   *interfaceName;
  bool   managed = CanManageConnections();

  while (getdelim(&line, &linel, '\n', fp) > 0)
  {
    // skip first two lines
    if (linenum++ < 2)
      continue;

    // search where the word begins
    interfaceName = line;
    while (isspace(*interfaceName))
      ++interfaceName;

    // read word until :
    n = strcspn(interfaceName, ": \t");
    interfaceName[n] = 0;

#if defined(TARGET_ANDROID)
    // only test ethX and wlanX interfaces,
    // anything else is non-standard and we do not care about it.
    if (strncmp(interfaceName, "eth", 3) != 0 && strncmp(interfaceName, "wlan", 4) != 0)
      continue;
#endif

    // make sure the device has ethernet encapsulation
    struct ifreq ifr;
    memset(&ifr, 0x00, sizeof(ifr));
    strcpy(ifr.ifr_name, interfaceName);
    std::string essid = "Wired";
    ConnectionType connection = NETWORK_CONNECTION_TYPE_WIRED;
    EncryptionType encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;

    if (ioctl(m_socket, SIOCGIFHWADDR, &ifr) >= 0)
    {
#if defined(TARGET_ANDROID)
      // Android cannot SIOCSIWSCAN (permissions error)
      // So just flag as wifi with unknown encryption and use it.
      if (IsWireless(m_socket, interfaceName))
      {
        essid = "Wifi";
        connection = NETWORK_CONNECTION_TYPE_WIFI;
        encryption = NETWORK_CONNECTION_ENCRYPTION_UNKNOWN;
      }
#else
      if (IsWireless(m_socket, interfaceName))
      {
        // get the list of access points on this interface, try this 3 times
        int retryCount = 0;
        while (!FindWifiConnections(interfaceName) && retryCount < 3)
          retryCount++;
      }
      else
#endif
      {
        // and ignore loopback, we also include ARPHRD_80211 but that will only
        // apply if we are running on android.
        if ((ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER || ifr.ifr_hwaddr.sa_family == ARPHRD_80211)
           && !(ifr.ifr_flags & IFF_LOOPBACK))
        {
          char macaddress[1024] = {0};
          if (ioctl(m_socket, SIOCGIFHWADDR, &ifr) >= 0)
          {
            // format up 'wire.<mac address>.<interface name>
            sprintf(macaddress, "%02X:%02X:%02X:%02X:%02X:%02X",
              ifr.ifr_hwaddr.sa_data[0], ifr.ifr_hwaddr.sa_data[1],
              ifr.ifr_hwaddr.sa_data[2], ifr.ifr_hwaddr.sa_data[3],
              ifr.ifr_hwaddr.sa_data[4], ifr.ifr_hwaddr.sa_data[5]);
          }
          /*
          CLog::Log(LOGDEBUG, "CPosixNetworkManager::FindNetworkInterfaces, "
            "interfaceName(%s), macaddress(%s), essid(%s)",
            interfaceName, macaddress, essid.c_str());
          */
          m_connections.push_back(CConnectionPtr(new CPosixConnection(managed,
             m_socket, interfaceName, macaddress, essid.c_str(), connection, encryption, 100)));
        }
      }
    }
  }

  free(line);
  fclose(fp);
}

bool CPosixNetworkManager::FindWifiConnections(const char *interfaceName)
{
  bool managed = CanManageConnections();
  // Query the wireless extentsions version number. It will help us when we
  // parse the resulting events
  struct iwreq iwr;
  char rangebuffer[sizeof(iw_range) * 2];    /* Large enough */
  struct iw_range*  range = (struct iw_range*) rangebuffer;

  memset(rangebuffer, 0x00, sizeof(rangebuffer));
  iwr.u.data.pointer = (caddr_t) rangebuffer;
  iwr.u.data.length = sizeof(rangebuffer);
  iwr.u.data.flags = 0;
  strncpy(iwr.ifr_name, interfaceName, IFNAMSIZ);
  if (ioctl(m_socket, SIOCGIWRANGE, &iwr) < 0)
  {
    CLog::Log(LOGWARNING, "%-8.16s  Driver has no Wireless Extension version information.",
      interfaceName);
    return false;
  }

  // Scan for wireless access points
  memset(&iwr, 0x00, sizeof(iwr));
  strncpy(iwr.ifr_name, interfaceName, IFNAMSIZ);
  if (ioctl(m_socket, SIOCSIWSCAN, &iwr) < 0)
  {
    CLog::Log(LOGWARNING, "Cannot initiate wireless scan: ioctl[SIOCSIWSCAN]: %s", strerror(errno));
    return false;
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
      return false;
    }

    strncpy(iwr.ifr_name, interfaceName, IFNAMSIZ);
    iwr.u.data.pointer = res_buf;
    iwr.u.data.length = res_buf_len;
    iwr.u.data.flags = 0;
    int x = ioctl(m_socket, SIOCGIWSCAN, &iwr);
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
      return false;
    }
  }

  size_t len = iwr.u.data.length;           // total length of the wireless events from the scan results
  unsigned char* pos = res_buf;             // pointer to the current event (about 10 per wireless network)
  unsigned char* end = res_buf + len;       // marks the end of the scan results
  unsigned char* custom;                    // pointer to the event payload
  struct iw_event iwe_buf, *iwe = &iwe_buf; // buffer to hold individual events

  bool first = true;
  char essid[IW_ESSID_MAX_SIZE+1];
  char macaddress[256];
  int  quality = 0, signalLevel = 0;
  EncryptionType encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;

  while (pos + IW_EV_LCP_LEN <= end)
  {
    // Event data may be unaligned, so make a local, aligned copy before processing.

    // copy event prefix (size of event minus IOCTL fixed payload)
    memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
    if (iwe->len <= IW_EV_LCP_LEN)
      break;

    // if the payload is nontrivial (i.e. > 16 octets) assume it comes after a pointer
    custom = pos + IW_EV_POINT_LEN;
    if (range->we_version_compiled > 18 &&
      (iwe->cmd == SIOCGIWESSID  ||
       iwe->cmd == SIOCGIWENCODE ||
       iwe->cmd == IWEVGENIE     ||
       iwe->cmd == IWEVCUSTOM))
    {
      // Wireless extentsions v19 removed the pointer from struct iw_point
      char *data_pos = (char*)&iwe_buf.u.data.length;
      int data_len = data_pos - (char*)&iwe_buf;
      memcpy(data_pos, pos + IW_EV_LCP_LEN, sizeof(struct iw_event) - data_len);
    }
    else
    {
      memcpy(&iwe_buf, pos, sizeof(struct iw_event));
      custom += IW_EV_POINT_OFF;
    }

    switch (iwe->cmd)
    {

      // Get the access point MAC addresses
      case SIOCGIWAP:
      {
        // this is the 1st cmp we get, so we have to play games
        // and push back our parsed results on the next one, but
        // we need to save the macaddress so we push the right one.
        char cur_macaddress[256] = {0};
        // macAddress is big-endian, write in byte chunks
        sprintf(cur_macaddress, "%02X:%02X:%02X:%02X:%02X:%02X",
          iwe->u.ap_addr.sa_data[0], iwe->u.ap_addr.sa_data[1],
          iwe->u.ap_addr.sa_data[2], iwe->u.ap_addr.sa_data[3],
          iwe->u.ap_addr.sa_data[4], iwe->u.ap_addr.sa_data[5]);

        if (first)
        {
          first = false;
          memcpy(macaddress, cur_macaddress, sizeof(macaddress));
        }
        else
        {
          m_connections.push_back(CConnectionPtr(new CPosixConnection(managed,
            m_socket, interfaceName, macaddress, essid,
            NETWORK_CONNECTION_TYPE_WIFI, encryption, quality)));
          memcpy(macaddress, cur_macaddress, sizeof(macaddress));
        }
        // reset encryption for parsing next access point
        encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;
        signalLevel = 0;
        break;
      }

      // Get ESSID
      case SIOCGIWESSID:
      {
        memset(essid, 0x00, sizeof(essid));
        if ((custom) && (iwe->u.essid.length))
          memcpy(essid, custom, iwe->u.essid.length);
        break;
      }

      // Quality part of statistics
      case IWEVQUAL:
      {
        quality = iwe->u.qual.qual;
        signalLevel = iwe->u.qual.level;
        break;
      }

      // Get encoding token & mode
      case SIOCGIWENCODE:
      {
        if (!(iwe->u.data.flags & IW_ENCODE_DISABLED) && encryption == NETWORK_CONNECTION_ENCRYPTION_NONE)
          encryption = NETWORK_CONNECTION_ENCRYPTION_WEP;
        break;
      }

      // Generic IEEE 802.11 information element (IE) for WPA, RSN, WMM, ...
      case IWEVGENIE:
      {
        int offset = 0;
        // Loop on each IE, each IE is minimum 2 bytes
        while (offset <= iwe_buf.u.data.length - 2)
        {
          switch (custom[offset])
          {
            case 0xdd: // WPA1
              if (encryption != NETWORK_CONNECTION_ENCRYPTION_WPA2)
                encryption = NETWORK_CONNECTION_ENCRYPTION_WPA;
              break;
            case 0x30: // WPA2
              encryption = NETWORK_CONNECTION_ENCRYPTION_WPA2;
              break;
          }
          offset += custom[offset+1] + 2;
        }
      }
    }
    pos += iwe->len;
  }

  if (!first)
  {
    m_connections.push_back(CConnectionPtr(new CPosixConnection(managed,
      m_socket, interfaceName, macaddress, essid,
      NETWORK_CONNECTION_TYPE_WIFI, encryption, quality)));
  }

  free(res_buf);
  res_buf = NULL;
  return true;
}
