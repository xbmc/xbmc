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

#include "system.h"
#include "Network.h"
#include "Application.h"
#include "libscrobbler/lastfmscrobbler.h"
#include "libscrobbler/librefmscrobbler.h"
#include "utils/RssReader.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"

using namespace std;

/* slightly modified in_ether taken from the etherboot project (http://sourceforge.net/projects/etherboot) */
bool in_ether (const char *bufp, unsigned char *addr)
{
  if (strlen(bufp) != 17)
    return false;

  char c;
  const char *orig;
  unsigned char *ptr = addr;
  unsigned val;

  int i = 0;
  orig = bufp;

  while ((*bufp != '\0') && (i < 6))
  {
    val = 0;
    c = *bufp++;

    if (isdigit(c))
      val = c - '0';
    else if (c >= 'a' && c <= 'f')
      val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      val = c - 'A' + 10;
    else
      return false;

    val <<= 4;
    c = *bufp;
    if (isdigit(c))
      val |= c - '0';
    else if (c >= 'a' && c <= 'f')
      val |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      val |= c - 'A' + 10;
    else if (c == ':' || c == '-' || c == 0)
      val >>= 4;
    else
      return false;

    if (c != 0)
      bufp++;

    *ptr++ = (unsigned char) (val & 0377);
    i++;

    if (*bufp == ':' || *bufp == '-')
      bufp++;
  }

  if (bufp - orig != 17)
    return false;

  return true;
}


/* Returns the quality, normalized as a percentage, of the network access point */
const int NetworkAccessPoint::getQuality()
{
  // Cisco dBm lookup table (partially nonlinear)
  // Source: Converting Signal Strength Percentage to dBm Values, 2002
  int quality;
  if (m_dBm >= -10) quality = 100;
  else if (m_dBm >= -20) quality = 85 + (m_dBm + 20);
  else if (m_dBm >= -30) quality = 77 + (m_dBm + 30);
  else if (m_dBm >= -60) quality = 48 + (m_dBm + 60);
  else if (m_dBm >= -98) quality = 13 + (m_dBm + 98);
  else if (m_dBm >= -112) quality = 1 + (m_dBm + 112);
  else quality = 0;
  return quality;
}

/* Returns a Google Gears specific JSON string */
const CStdString NetworkAccessPoint::toJson()
{
  CStdString jsonBuffer;
  if (m_macAddress.IsEmpty())
      return "{}";
  jsonBuffer.Format("{\"mac_address\":\"%s\"", m_macAddress.c_str());
  if (!m_essId.IsEmpty())
    jsonBuffer.AppendFormat(",\"ssid\":\"%s\"", m_essId.c_str());
  if (m_dBm < 0)
    jsonBuffer.AppendFormat(",\"signal_strength\":%d", m_dBm);
  if (m_channel != 0)
    jsonBuffer.AppendFormat(",\"channel\":%d", m_channel);
  jsonBuffer.append("}");
  return jsonBuffer;
}

/* Translates a 802.11a+b frequency into corresponding channel */
int NetworkAccessPoint::FreqToChannel(float frequency)
{
  int IEEE80211Freq[] = {2412, 2417, 2422, 2427, 2432,
                          2437, 2442, 2447, 2452, 2457,
                          2462, 2467, 2472, 2484,
                          5180, 5200, 5210, 5220, 5240, 5250,
                          5260, 5280, 5290, 5300, 5320,
                          5745, 5760, 5765, 5785, 5800, 5805, 5825};
  int IEEE80211Ch[] = {     1,    2,    3,    4,    5,
                            6,    7,    8,    9,   10,
                            11,   12,   13,   14,
                            36,   40,   42,   44,   48,   50,
                            52,   56,   58,   60,   64,
                          149,  152,  153,  157,  160,  161,  165};

  int mod_chan = (int)(frequency / 1000000 + 0.5f);
  for (unsigned int i = 0; i < sizeof(IEEE80211Freq) / sizeof(int); ++i)
  {
      if (IEEE80211Freq[i] == mod_chan)
        return IEEE80211Ch[i];
  }
  return 0; // unknown
}


CNetwork::CNetwork()
{
   g_application.getApplicationMessenger().NetworkMessage(SERVICES_UP, 0);
}

CNetwork::~CNetwork()
{
   g_application.getApplicationMessenger().NetworkMessage(SERVICES_DOWN, 0);
}

int CNetwork::ParseHex(char *str, unsigned char *addr)
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

CNetworkInterface* CNetwork::GetFirstConnectedInterface()
{
   vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->IsConnected())
         return iface;
      ++iter;
   }

   return NULL;
}

bool CNetwork::HasInterfaceForIP(unsigned long address)
{
   unsigned long subnet;
   unsigned long local;
   vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->IsConnected())
      {
         subnet = ntohl(inet_addr(iface->GetCurrentNetmask()));
         local = ntohl(inet_addr(iface->GetCurrentIPAddress()));
         if( (address & subnet) == (local & subnet) )
            return true;
      }
      ++iter;
   }

   return false;
}

bool CNetwork::IsAvailable(bool wait /*= false*/)
{
  if (wait)
  {
    // NOTE: Not implemented in linuxport branch as 99.9% of the time
    //       we have the network setup already.  Trunk code has a busy
    //       wait for 5 seconds here.
  }

  vector<CNetworkInterface*>& ifaces = GetInterfaceList();
  return (ifaces.size() != 0);
}

bool CNetwork::IsConnected()
{
   return GetFirstConnectedInterface() != NULL;
}

CNetworkInterface* CNetwork::GetInterfaceByName(CStdString& name)
{
   vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->GetName().Equals(name))
         return iface;
      ++iter;
   }

   return NULL;
}

void CNetwork::NetworkMessage(EMESSAGE message, int param)
{
  switch( message )
  {
    case SERVICES_UP:
    {
      CLog::Log(LOGDEBUG, "%s - Starting network services",__FUNCTION__);
      StartServices();
    }
    break;
    case SERVICES_DOWN:
    {
      CLog::Log(LOGDEBUG, "%s - Signaling network services to stop",__FUNCTION__);
      StopServices(false); //tell network services to stop, but don't wait for them yet
      CLog::Log(LOGDEBUG, "%s - Waiting for network services to stop",__FUNCTION__);
      StopServices(true); //wait for network services to stop
    }
    break;
  }
}

bool CNetwork::WakeOnLan(const char* mac)
{
  int i, j, packet;
  unsigned char ethaddr[8];
  unsigned char buf [128];
  unsigned char *ptr;

  // Fetch the hardware address
  if (!in_ether(mac, ethaddr))
  {
    CLog::Log(LOGERROR, "%s - Invalid hardware address specified (%s)", __FUNCTION__, mac);
    return false;
  }

  // Setup the socket
  if ((packet = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to create socket (%s)", __FUNCTION__, strerror (errno));
    return false;
  }
 
  // Set socket options
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  saddr.sin_port = htons(9);

  unsigned int value = 1;
  if (setsockopt (packet, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "%s - Unable to set socket options (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }
 
  // Build the magic packet (6 x 0xff + 16 x MAC address)
  ptr = buf;
  for (i = 0; i < 6; i++)
    *ptr++ = 0xff;

  for (j = 0; j < 16; j++)
    for (i = 0; i < 6; i++)
      *ptr++ = ethaddr[i];
 
  // Send the magic packet
  if (sendto (packet, (char *)buf, 102, 0, (struct sockaddr *)&saddr, sizeof (saddr)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to send magic packet (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "%s - Magic packet send to '%s'", __FUNCTION__, mac);
  return true;
}

void CNetwork::StartServices()
{
#ifdef HAS_TIME_SERVER
  g_application.StartTimeServer();
#endif
#ifdef HAS_WEB_SERVER
  if (!g_application.StartWebServer())
    g_application.m_guiDialogKaiToast.QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_UPNP
  g_application.StartUPnP();
#endif
#ifdef HAS_EVENT_SERVER
  if (!g_application.StartEventServer())
    g_application.m_guiDialogKaiToast.QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_DBUS_SERVER
  g_application.StartDbusServer();
#endif
#ifdef HAS_JSONRPC
  if (!g_application.StartJSONRPCServer())
    g_application.m_guiDialogKaiToast.QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33103), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_ZEROCONF
  g_application.StartZeroconf();
#endif
  CLastfmScrobbler::GetInstance()->Init();
  CLibrefmScrobbler::GetInstance()->Init();
  g_rssManager.Start();
}

void CNetwork::StopServices(bool bWait)
{
  if (bWait)
  {
#ifdef HAS_TIME_SERVER
    g_application.StopTimeServer();
#endif
#ifdef HAS_UPNP
    g_application.StopUPnP(bWait);
#endif
#ifdef HAS_ZEROCONF
    g_application.StopZeroconf();
#endif
#ifdef HAS_WEB_SERVER
    g_application.StopWebServer();
#endif    
    CLastfmScrobbler::GetInstance()->Term();
    CLibrefmScrobbler::GetInstance()->Term();
    // smb.Deinit(); if any file is open over samba this will break.

    g_rssManager.Stop();
  }

#ifdef HAS_EVENT_SERVER
  g_application.StopEventServer(bWait, false);
#endif
#ifdef HAS_DBUS_SERVER
  g_application.StopDbusServer(bWait);
#endif
#ifdef HAS_JSONRPC
    g_application.StopJSONRPCServer(bWait);
#endif
}
