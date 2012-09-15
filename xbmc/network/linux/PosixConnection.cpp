/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "PosixConnection.h"
#include "Util.h"
#include "linux/XTimeUtils.h"
#include "utils/StdString.h"
#include "utils/log.h"

// temp until keychainManager is working
#include "settings/GUISettings.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <resolv.h>
#include <string.h>
#include <vector>

#if defined(TARGET_LINUX)
  #include <linux/if.h>
  #include <linux/wireless.h>
  #include <linux/sockios.h>
  #include <net/if_arp.h>
#endif

#ifdef TARGET_ANDROID
  #include "android/bionic_supplement/bionic_supplement.h"
  #include "sys/system_properties.h"
#endif

//-----------------------------------------------------------------------
int PosixParseHex(char *str, unsigned char *addr)
{
  int len = 0;

  while (*str)
  {
    int tmp;
    if (str[1] == 0)
      return -1;
    if (sscanf(str, "%02x", (unsigned int *)&tmp) != 1)
      return -1;
    addr[len] = tmp;
    len++;
    str += 2;
  }

  return len;
}

bool PosixCheckHex(const std::string &passphrase)
{
  // we could get fooled by strings that only
  // have 0-9, A, B, C, D, E, F in them :)
  for (size_t i = 0; i < passphrase.size(); i++)
  {
    switch (passphrase[i])
    {
      default:
        return false;
        break;
      case '0':case '1':case '2':case '3':case '4':
      case '5':case '6':case '7':case '8':case '9':
      case 'a':case 'A':case 'b':case 'B':
      case 'c':case 'C':case 'd':case 'D':
      case 'e':case 'E':case 'f':case 'F':
        break;
    }
  }

  return true;
}

bool PosixGuessIsHexPassPhrase(const std::string &passphrase, EncryptionType encryption)
{
  if (encryption == NETWORK_CONNECTION_ENCRYPTION_WEP)
  {
    // wep hex 256-bit is 58 characters.
    if (passphrase.size() == 58)
      return true;

    // wep hex 152-bit is 32 characters.
    if (passphrase.size() == 32)
      return true;

    // wep hex 128-bit is 26 characters.
    if (passphrase.size() == 26)
      return true;

    // wep hex 64-bit is 10 characters.
    if (passphrase.size() == 10)
      return true;
  }
  else
  {
    // wap/wpa2 hex has a length of 64.
    if (passphrase.size() == 64)
      return true;
  }

  // anthing else is wep/wap/wpa2 ascii
  return false;
}

bool IsWireless(int socket, const char *interface)
{
  struct iwreq wrq;
  memset(&wrq, 0x00, sizeof(iwreq));

  strcpy(wrq.ifr_name, interface);
  if (ioctl(socket, SIOCGIWNAME, &wrq) < 0)
    return false;

   return true;
}

bool PosixCheckInterfaceUp(const std::string &interface)
{
  std::string iface_state;
  std::string sysclasspath("/sys/class/net/" + interface + "/operstate");

  int fd = open(sysclasspath.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    char buffer[256] = {0};
    read(fd, buffer, 255);
    close(fd);
    // make sure we can treat this as a c-str.
    buffer[255] = 0;
    iface_state = buffer;
  }
  if (iface_state.find("up") != std::string::npos)
    return true;
  else
    return false;
}

std::string PosixGetDefaultGateway(const std::string &interface)
{
  std::string result = "";

  FILE* fp = fopen("/proc/net/route", "r");
  if (!fp)
    return result;

  char* line     = NULL;
  size_t linel   = 0;
  int n, linenum = 0;
  char   dst[128], iface[16], gateway[128];
  while (getdelim(&line, &linel, '\n', fp) > 0)
  {
    // skip first two lines
    if (linenum++ < 1)
      continue;

    // search where the word begins
    n = sscanf(line, "%16s %128s %128s", iface, dst, gateway);

    if (n < 3)
      continue;

    if (strcmp(iface,   interface.c_str()) == 0 &&
        strcmp(dst,     "00000000") == 0 &&
        strcmp(gateway, "00000000") != 0)
    {
      unsigned char gatewayAddr[4];
      int len = PosixParseHex(gateway, gatewayAddr);
      if (len == 4)
      {
        struct in_addr in;
        in.s_addr = (gatewayAddr[0] << 24) |
          (gatewayAddr[1] << 16) |
          (gatewayAddr[2] << 8)  |
          (gatewayAddr[3]);
        result = inet_ntoa(in);
        break;
      }
    }
  }
  free(line);
  fclose(fp);

  return result;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CPosixConnection::CPosixConnection(bool managed,
  int socket, const char *interface, const char *macaddress,
  const char *essid, ConnectionType type, EncryptionType encryption, int signal)
{
  m_managed = managed;
  m_socket  = socket;

  m_type       = type;
  m_essid      = essid;
  m_signal     = signal;
  m_interface  = interface;
  m_macaddress = macaddress;
  m_encryption = encryption;

  m_state = GetState();
}

CPosixConnection::~CPosixConnection()
{
}

std::string CPosixConnection::GetName() const
{
  return m_essid;
}

std::string CPosixConnection::GetAddress() const
{
  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interface.c_str());
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl(m_socket, SIOCGIFADDR, &ifr) >= 0)
    return inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
  else
    return "";
}

std::string CPosixConnection::GetNetmask() const
{
  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interface.c_str());
  ifr.ifr_addr.sa_family = AF_INET;

  if (ioctl(m_socket, SIOCGIFNETMASK, &ifr) >= 0)
    return inet_ntoa((*((struct sockaddr_in*)&ifr.ifr_addr)).sin_addr);
  else
    return "";
}

std::string CPosixConnection::GetGateway() const
{
  return PosixGetDefaultGateway(m_interface);
}

std::string CPosixConnection::GetNameServer() const
{
  std::string nameserver("127.0.0.1");

#if defined(TARGET_ANDROID)
  char ns[PROP_VALUE_MAX];

  if (__system_property_get("net.dns1", ns))
    nameserver = ns;
#else
  res_init();
  for (int i = 0; i < _res.nscount; i ++)
  {
      nameserver = inet_ntoa(((struct sockaddr_in *)&_res.nsaddr_list[0])->sin_addr);
      break;
  }
#endif
  return nameserver;
}

std::string CPosixConnection::GetMacAddress() const
{
  CStdString result;
  result.Format("00:00:00:00:00:00");

  struct ifreq ifr;
  strcpy(ifr.ifr_name, m_interface.c_str());
  if (ioctl(m_socket, SIOCGIFHWADDR, &ifr) >= 0)
  {
    result.Format("%02X:%02X:%02X:%02X:%02X:%02X",
      ifr.ifr_hwaddr.sa_data[0], ifr.ifr_hwaddr.sa_data[1],
      ifr.ifr_hwaddr.sa_data[2], ifr.ifr_hwaddr.sa_data[3],
      ifr.ifr_hwaddr.sa_data[4], ifr.ifr_hwaddr.sa_data[5]);
  }

  return result.c_str();
}

ConnectionType CPosixConnection::GetType() const
{
  return m_type;
}

ConnectionState CPosixConnection::GetState() const
{
  int zero = 0;
  struct ifreq ifr;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  // check if the interface is up.
  strcpy(ifr.ifr_name, m_interface.c_str());
  if (ioctl(m_socket, SIOCGIFFLAGS, &ifr) < 0)
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  // check for running and not loopback
  if (!(ifr.ifr_flags & IFF_RUNNING) || (ifr.ifr_flags & IFF_LOOPBACK))
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  // check for an ip address
  if (ioctl(m_socket, SIOCGIFADDR, &ifr) < 0)
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  if (ifr.ifr_addr.sa_data == NULL)
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  // return only interfaces which have an ip address
  if (memcmp(ifr.ifr_addr.sa_data + sizeof(short), &zero, sizeof(int)) == 0)
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
  {
    // for wifi, we need to check we have a wifi driver name.
    struct iwreq wrq;
    strcpy(wrq.ifr_name, m_interface.c_str());
    if (ioctl(m_socket, SIOCGIWNAME, &wrq) < 0)
      return NETWORK_CONNECTION_STATE_DISCONNECTED;

    // since the wifi interface can be connected to
    // any wifi access point, we need to compare the assigned
    // essid to our connection essid. If they match, then
    // this connection is up.
    char essid[IFNAMSIZ];
    memset(&wrq, 0x00, sizeof(struct iwreq));
    wrq.u.essid.pointer = (caddr_t)essid;
    wrq.u.essid.length  = sizeof(essid);
    strncpy(wrq.ifr_name, m_interface.c_str(), IFNAMSIZ);
    if (ioctl(m_socket, SIOCGIWESSID, &wrq) < 0)
      return NETWORK_CONNECTION_STATE_DISCONNECTED;

    if (wrq.u.essid.length <= 0)
      return NETWORK_CONNECTION_STATE_DISCONNECTED;

    std::string test_essid(essid, wrq.u.essid.length);
    if (m_essid.find(test_essid) == std::string::npos)
      return NETWORK_CONNECTION_STATE_DISCONNECTED;
  }

  // finally, we need to see if we have a gateway assigned to our interface.
  std::string default_gateway = PosixGetDefaultGateway(m_interface);
  if (default_gateway.size() <= 0)
    return NETWORK_CONNECTION_STATE_DISCONNECTED;

  // passing the above tests means we are connected.
  return NETWORK_CONNECTION_STATE_CONNECTED;
}

unsigned int CPosixConnection::GetSpeed() const
{
  int speed = 100;
  return speed;
}

IPConfigMethod CPosixConnection::GetMethod() const
{
  return m_method;
}

unsigned int CPosixConnection::GetStrength() const
{
  /*
  int strength = 100;
  if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
  {
    struct iwreq wreq;
    // wireless tools says this is large enough
    char   buffer[sizeof(struct iw_range) * 2];
    int max_qual_level = 0;
    double max_qual = 92.0;

    // Fetch the range
    memset(buffer, 0x00, sizeof(iw_range) * 2);
    memset(&wreq,  0x00, sizeof(struct iwreq));
    wreq.u.data.pointer = (caddr_t)buffer;
    wreq.u.data.length  = sizeof(buffer);
    wreq.u.data.flags   = 0;
    strncpy(wreq.ifr_name, m_interface.c_str(), IFNAMSIZ);
    if (ioctl(m_socket, SIOCGIWRANGE, &wreq) >= 0)
    {
      struct iw_range *range = (struct iw_range*)buffer;
      if (range->max_qual.qual > 0)
        max_qual = range->max_qual.qual;
      if (range->max_qual.level > 0)
        max_qual_level = range->max_qual.level;
    }
    printf("CPosixConnection::GetStrength, max_qual(%d), qual(%d)\n", max_qual_level, max_qual_level);

    struct iw_statistics stats;
    memset(&wreq, 0x00, sizeof(struct iwreq));
    // Fetch the stats
    wreq.u.data.pointer = (caddr_t)&stats;
    wreq.u.data.length  = sizeof(stats);
    wreq.u.data.flags   = 1;     // Clear updated flag
    strncpy(wreq.ifr_name, m_interface.c_str(), IFNAMSIZ);
    if (ioctl(m_socket, SIOCGIWSTATS, &wreq) < 0) {
      return strength;
    }

    // this is not correct :)
    strength = (100 * wreq.u.qual.qual)/256;

    printf("CPosixConnection::GetStrength, strength(%d), qual(%d)\n", strength, wreq.u.qual.qual);
  }
  return strength;
  */
  return m_signal;
}

EncryptionType CPosixConnection::GetEncryption() const
{
  return m_encryption;
}

bool CPosixConnection::Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig)
{
  if (!m_managed)
    return true;

  std::string passphrase("");

  if (storage && m_type == NETWORK_CONNECTION_TYPE_WIFI)
  {
    if (m_encryption != NETWORK_CONNECTION_ENCRYPTION_NONE)
    {
      if (!storage->GetPassphrase(m_essid, passphrase))
        return false;
      if (passphrase.size() <= 0)
        return false;
    }
  }
  else
  {
    passphrase = g_guiSettings.GetString("network.passphrase");
    /*
    CVariant secret;
    if (m_keyringManager->FindSecret("network", m_essid, secret) && secret.isString())
    {
      passphrase = secret.asString();
      return true;
    }
    */
  }

  if (DoConnection(ipconfig, passphrase) && GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
  {
    // if we connect, save out the essid
    g_guiSettings.SetString("network.essid", m_essid.c_str());
    // quick update of some internal vars
    m_method  = ipconfig.m_method;
    m_address = ipconfig.m_address;
    m_netmask = ipconfig.m_netmask;
    m_gateway = ipconfig.m_gateway;
    return true;
  }
  else
  {
    if (storage)
      storage->InvalidatePassphrase(m_essid);
  }

  return false;
}

//-----------------------------------------------------------------------
bool CPosixConnection::PumpNetworkEvents()
{
  bool state_changed = false;

  ConnectionState state = GetState();
  if (m_state != state)
  {
    m_state = state;
    state_changed = true;
  }

  return state_changed;
}

bool CPosixConnection::DoConnection(const CIPConfig &ipconfig, std::string passphrase)
{
  FILE *fr = fopen("/etc/network/interfaces", "r");
  if (!fr)
    return false;

  char *line = NULL;
  size_t line_length = 0;
  std::vector<std::string> interfaces_lines;
  while (getdelim(&line, &line_length, '\n', fr) > 0)
    interfaces_lines.push_back(line);
  fclose(fr);

  std::vector<std::string> new_interfaces_lines;
  std::vector<std::string> ifdown_interfaces;
  for (size_t i = 0; i < interfaces_lines.size(); i++)
  {
    //printf("CPosixConnection::SetSettings, interfaces_lines:%s", interfaces_lines[i].c_str());

    // comments are always skipped and copied over
    if (interfaces_lines[i].find("#") != std::string::npos)
    {
      new_interfaces_lines.push_back(interfaces_lines[i]);
      continue;
    }

    // always copy the auto section over
    if (interfaces_lines[i].find("auto") != std::string::npos)
    {
      new_interfaces_lines.push_back(interfaces_lines[i]);
      continue;
    }

    // always copy the loopback iface section over
    if (interfaces_lines[i].find("iface lo") != std::string::npos)
    {
      new_interfaces_lines.push_back(interfaces_lines[i]);
      continue;
    }

    // look for "iface <interface name> inet"
    if (interfaces_lines[i].find("iface") != std::string::npos)
    {
      // we will take all interfaces down, then bring up ours.
      // so remember all iface names. This could be more robust :)
      std::string ifdown_interface = interfaces_lines[i];
      std::string::size_type start = ifdown_interface.find("iface") + sizeof("iface");
      std::string::size_type end   = ifdown_interface.find(" inet", start);
      ifdown_interfaces.push_back(ifdown_interface.substr(start, end - start));

      if (interfaces_lines[i].find(m_interface) == std::string::npos)
      {
        // this is not our interface section (ethX or wlanX).
        // we always copy the iface line over.
        new_interfaces_lines.push_back(interfaces_lines[i]);
        for (size_t j = i + 1; j < interfaces_lines.size(); j++)
        {
          // if next line is an 'iface', we are done.
          if (interfaces_lines[j].find("iface") != std::string::npos)
          {
            // back up by one so next pass in
            // loop starts at correct place.
            i = j - 1;
            break;
          }
          else
          {
            // copy these iface details over, we do not care about them.
            new_interfaces_lines.push_back(interfaces_lines[j]);
          }
        }
        continue;
      }
      else
      {
        std::string tmp;
        // is this our interface section (ethX or wlanX)
        // check requested method first.
        if (ipconfig.m_method == IP_CONFIG_STATIC)
        {
          std::string method("static");
          tmp = "iface " + m_interface + " inet " + method + "\n";
          new_interfaces_lines.push_back(tmp);
          tmp = "  address " + ipconfig.m_address + "\n";
          new_interfaces_lines.push_back(tmp);
          tmp = "  netmask " + ipconfig.m_netmask + "\n";
          new_interfaces_lines.push_back(tmp);
          tmp = "  gateway " + ipconfig.m_gateway + "\n";
          new_interfaces_lines.push_back(tmp);
        }
        else
        {
          std::string method("dhcp");
          if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
          {
            // the wpa_action script will take care of
            // launching udhcpc after the AP connects.
            method = "manual";
          }
          tmp = "iface " + m_interface + " inet " + method + "\n";
          new_interfaces_lines.push_back(tmp);
        }

        // fill in the wifi details if needed
        if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
        {
          // quote the essid, spaces are legal characters
          tmp = "  wpa-ssid \"" + m_essid + "\"\n";
          new_interfaces_lines.push_back(tmp);

          tmp = "  wpa-ap-scan 1\n";
          new_interfaces_lines.push_back(tmp);

          tmp = "  wpa-scan-ssid 1\n";
          new_interfaces_lines.push_back(tmp);

          if (m_encryption == NETWORK_CONNECTION_ENCRYPTION_NONE)
          {
            tmp = "  wpa-key-mgmt NONE\n";
            new_interfaces_lines.push_back(tmp);
          }
          else if (m_encryption == NETWORK_CONNECTION_ENCRYPTION_WEP)
          {
            tmp = "  wpa-key-mgmt NONE\n";
            new_interfaces_lines.push_back(tmp);

            // if ascii, then quote it, if hex, no quotes
            if (PosixGuessIsHexPassPhrase(passphrase, m_encryption))
              tmp = "  wpa-wep-key0 " + passphrase + "\n";
            else
              tmp = "  wpa-wep-key0 \"" + passphrase + "\"\n";
            new_interfaces_lines.push_back(tmp);
            tmp = "  wpa-wep-tx-keyidx 0\n";
            new_interfaces_lines.push_back(tmp);
          }
          else if (m_encryption == NETWORK_CONNECTION_ENCRYPTION_WPA ||
            m_encryption == NETWORK_CONNECTION_ENCRYPTION_WPA2)
          {
            // if ascii, then quote it, if hex, no quotes
            if (PosixGuessIsHexPassPhrase(passphrase, m_encryption))
              tmp = "  wpa-psk " + passphrase + "\n";
            else
              tmp = "  wpa-psk \"" + passphrase + "\"\n";
            new_interfaces_lines.push_back(tmp);
            if (m_encryption == NETWORK_CONNECTION_ENCRYPTION_WPA)
              tmp = "  wpa-proto WPA\n";
            else
              tmp = "  wpa-proto WPA2\n";
            new_interfaces_lines.push_back(tmp);
          }
        }
      }
    }
  }

  // write out the new /etc/network/interfaces as a temp file.
  // we do a rename on it later as that is atomic.
  FILE* fw = fopen("/etc/network/interfaces.temp", "w");
  if (!fw)
    return false;
  for (size_t i = 0; i < new_interfaces_lines.size(); i++)
  {
    //printf("CPosixConnection::SetSettings, new_interfaces_lines:%s", new_interfaces_lines[i].c_str());
    fwrite(new_interfaces_lines[i].c_str(), new_interfaces_lines[i].size(), 1, fw);
  }
  fclose(fw);

  // take down all interfaces using the current /etc/network/interfaces
  int rtn_error;
  std::string cmd;
  for (size_t i = 0; i < ifdown_interfaces.size(); i++)
  {
    // check if interface is actually 'up'
    //if (PosixCheckInterfaceUp(ifdown_interfaces[i]))
    {
      cmd = "/sbin/ifdown " + ifdown_interfaces[i];
      rtn_error = system(cmd.c_str());
      // ifdown was succesful but we got 'No child processes' which is harmless
      if (rtn_error != 0 && errno != ECHILD)
        CLog::Log(LOGERROR, "NetworkManager: Unable to stop interface %s, %s", ifdown_interfaces[i].c_str(), strerror(errno));
      else
        CLog::Log(LOGDEBUG, "NetworkManager: Stopped interface %s", ifdown_interfaces[i].c_str());
    }
  }

  // Rename the file (remember, you can not rename across devices)
  if (rename("/etc/network/interfaces.temp", "/etc/network/interfaces") < 0)
    return false;

  // bring up this interface using the new /etc/network/interfaces
  cmd = "/sbin/ifup " + m_interface;
  rtn_error = system(cmd.c_str());
  // ifup was succesful but we got 'No child processes' which is harmless
  if (rtn_error != 0 && errno != ECHILD)
    CLog::Log(LOGERROR, "NetworkManager: Unable to start interface %s, %s", m_interface.c_str(), strerror(errno));
  else
    CLog::Log(LOGDEBUG, "NetworkManager: Started interface %s", m_interface.c_str());

  // wait for wap to connect to the AP and udhcp to fetch an IP
  if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
  {
    for (int i = 0; i < 60; ++i)
    {
      if (GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
        break;
      Sleep(1000);
    }
  }

  return true;
}
