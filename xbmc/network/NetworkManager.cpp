/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "NetworkManager.h"
#include "NullNetworkManager.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "libscrobbler/lastfmscrobbler.h"
#include "libscrobbler/librefmscrobbler.h"
#include "linux/ConnmanNetworkManager.h"
#include "linux/PosixNetworkManager.h"
#include "windows/WinNetworkManager.h"
#include "utils/log.h"
#include "utils/RssReader.h"

/* slightly modified in_ether taken from the etherboot project (http://sourceforge.net/projects/etherboot) */
bool in_ether(const char *bufp, unsigned char *addr)
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

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CNetworkManager::CNetworkManager()
{
  m_instance = NULL;
  m_state = NETWORK_CONNECTION_STATE_UNKNOWN;
}

CNetworkManager::~CNetworkManager()
{
  delete m_instance;
}

void CNetworkManager::Initialize()
{
#ifdef HAS_DBUS
//  if (CConnmanNetworkManager::HasConnman())
//    m_instance = new CConnmanNetworkManager();
#endif

#if defined(TARGET_POSIX)
  if (m_instance == NULL)
    m_instance = new CPosixNetworkManager();
#endif

#ifdef TARGET_WINDOWS
  if (m_instance == NULL)
    m_instance = new CWinNetworkManager();
#endif

  if (m_instance == NULL)
    m_instance = new CNullNetworkManager();

  m_defaultConnection = CConnectionPtr(new CNullConnection());
  OnConnectionListChange(m_instance->GetConnections());
}

bool CNetworkManager::PumpNetworkEvents()
{
  return m_instance->PumpNetworkEvents(this);
}

std::string CNetworkManager::GetDefaultConnectionName()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetName();
  else
    return std::string("opps");
}

std::string CNetworkManager::GetDefaultConnectionAddress()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetAddress();
  else
    return std::string("opps");
}

std::string CNetworkManager::GetDefaultConnectionNetmask()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetNetmask();
  else
    return std::string("opps");
}

std::string CNetworkManager::GetDefaultConnectionMacAddress()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetMacAddress();
  else
    return std::string("opps");
}

std::string CNetworkManager::GetDefaultConnectionGateway()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetGateway();
  else
    return std::string("opps");
}

std::string CNetworkManager::GetDefaultConnectionNameServer()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetNameServer();
  else
    return std::string("127.0.0.1");
}

ConnectionType CNetworkManager::GetDefaultConnectionType()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetType();
  else
    return NETWORK_CONNECTION_TYPE_UNKNOWN;
}

IPConfigMethod CNetworkManager::GetDefaultConnectionMethod()
{
  if (m_defaultConnection)
    return m_defaultConnection->GetMethod();
  else
    return IP_CONFIG_DISABLED;
}

ConnectionState CNetworkManager::GetDefaultConnectionState()
{
  return m_state;
}

bool CNetworkManager::IsConnected()
{
  return GetDefaultConnectionState() == NETWORK_CONNECTION_STATE_CONNECTED;
}

bool CNetworkManager::IsAvailable(bool wait)
{
  return true;
}

bool CNetworkManager::CanManageConnections()
{
  return m_instance->CanManageConnections();
}

ConnectionList CNetworkManager::GetConnections()
{
  return m_connections;
}

void CNetworkManager::OnConnectionStateChange(ConnectionState state)
{
  ConnectionState oldState = m_state;
  m_state = state;

  if (m_state != oldState)
    CLog::Log(LOGDEBUG, "NetworkManager: State changed to %s", ConnectionStateToString(m_state));

  if (oldState != NETWORK_CONNECTION_STATE_CONNECTED && m_state == NETWORK_CONNECTION_STATE_CONNECTED)
    StartServices();
  else if (oldState == NETWORK_CONNECTION_STATE_CONNECTED && oldState != m_state)
    StopServices();
}

void CNetworkManager::OnConnectionChange(CConnectionPtr connection)
{
  if (connection->GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
    m_defaultConnection = connection;

  if (g_windowManager.GetWindow(WINDOW_DIALOG_ACCESS_POINTS))
  {
    CAction action(ACTION_CONNECTIONS_REFRESH);
    CApplicationMessenger::Get().SendAction(action, WINDOW_DIALOG_ACCESS_POINTS);
  }
}

void CNetworkManager::OnConnectionListChange(ConnectionList list)
{
  m_connections = list;

  for (unsigned int i = 0; i < m_connections.size(); i++)
  {
    if (m_connections[i]->GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
    {
      m_defaultConnection = m_connections[i];
      OnConnectionStateChange(NETWORK_CONNECTION_STATE_CONNECTED);
      break;
    }
  }

  if (g_windowManager.GetWindow(WINDOW_DIALOG_ACCESS_POINTS))
  {
    CAction action(ACTION_CONNECTIONS_REFRESH);
    CApplicationMessenger::Get().SendAction(action, WINDOW_DIALOG_ACCESS_POINTS);
  }
}

void CNetworkManager::NetworkMessage(EMESSAGE message, int param)
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

bool CNetworkManager::WakeOnLan(const char *mac)
{
/*
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
  if ((packet = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
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
  if (setsockopt(packet, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
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
  if (sendto(packet, (char *)buf, 102, 0, (struct sockaddr *)&saddr, sizeof (saddr)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to send magic packet (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "%s - Magic packet send to '%s'", __FUNCTION__, mac);
*/
  return true;
}

void CNetworkManager::StartServices()
{
  CLog::Log(LOGDEBUG, "NetworkManager: Starting network services");

  // TODO: fix properly
  system("/etc/init.d/S49ntp restart");

#ifdef HAS_TIME_SERVER
  g_application.StartTimeServer();
#endif
#ifdef HAS_WEB_SERVER
  if (!g_application.StartWebServer())
    CGUIDialogKaiToast::QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_UPNP
  g_application.StartUPnP();
#endif
#ifdef HAS_EVENT_SERVER
  if (!g_application.StartEventServer())
    CGUIDialogKaiToast::QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_DBUS_SERVER
  g_application.StartDbusServer();
#endif
#ifdef HAS_JSONRPC
  if (!g_application.StartJSONRPCServer())
    CGUIDialogKaiToast::QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33103), g_localizeStrings.Get(33100));
#endif
#ifdef HAS_ZEROCONF
  g_application.StartZeroconf();
#endif
#ifdef HAS_AIRPLAY
  g_application.StartAirplayServer();
#endif
  CLastfmScrobbler::GetInstance()->Init();
  CLibrefmScrobbler::GetInstance()->Init();
  g_rssManager.Start();
}

void CNetworkManager::StopServices()
{
  CLog::Log(LOGDEBUG, "NetworkManager: Stopping network services");
  StopServices(false);
  CLog::Log(LOGDEBUG, "NetworkManager: Waiting for network services to stop");
  StopServices(true);
}

void CNetworkManager::StopServices(bool wait)
{
  if (wait)
  {
#ifdef HAS_TIME_SERVER
    g_application.StopTimeServer();
#endif
#ifdef HAS_UPNP
    g_application.StopUPnP(wait);
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
  g_application.StopEventServer(wait, false);
#endif
#ifdef HAS_DBUS_SERVER
  g_application.StopDbusServer(wait);
#endif
#ifdef HAS_JSONRPC
  g_application.StopJSONRPCServer(wait);
#endif
#if defined(HAS_AIRPLAY) || defined(HAS_AIRTUNES)
  g_application.StopAirplayServer(wait);
#endif
}

const char *CNetworkManager::ConnectionStateToString(ConnectionState state)
{
  switch (state)
  {
    case NETWORK_CONNECTION_STATE_FAILURE:
      return "failure";
    case NETWORK_CONNECTION_STATE_DISCONNECTED:
      return "disconnect";
    case NETWORK_CONNECTION_STATE_CONNECTING:
      return "connecting";
    case NETWORK_CONNECTION_STATE_CONNECTED:
      return "connected";
    case NETWORK_CONNECTION_STATE_UNKNOWN:
    default:
      return "unknown";
  }
}
