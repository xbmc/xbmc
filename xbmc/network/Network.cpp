/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Network.h"
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "network/NetworkServices.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#include "utils/CharsetConverter.h"
#endif
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"

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

CNetworkBase::CNetworkBase() :
  m_services(new CNetworkServices())
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_NETWORKMESSAGE, SERVICES_UP, 0);
}

CNetworkBase::~CNetworkBase()
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_NETWORKMESSAGE, SERVICES_DOWN, 0);
}

int CNetworkBase::ParseHex(char *str, unsigned char *addr)
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

bool CNetworkBase::GetHostName(std::string& hostname)
{
  char hostName[128];
  if (gethostname(hostName, sizeof(hostName)))
    return false;

#ifdef TARGET_WINDOWS
  std::string hostStr;
  g_charsetConverter.systemToUtf8(hostName, hostStr);
  hostname = hostStr;
#else
  hostname = hostName;
#endif
  return true;
}

bool CNetworkBase::IsLocalHost(const std::string& hostname)
{
  if (hostname.empty())
    return false;

  if (StringUtils::StartsWith(hostname, "127.")
      || (hostname == "::1")
      || StringUtils::EqualsNoCase(hostname, "localhost"))
    return true;

  std::string myhostname;
  if (GetHostName(myhostname)
      && StringUtils::EqualsNoCase(hostname, myhostname))
    return true;

  std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
  std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
  while (iter != ifaces.end())
  {
    CNetworkInterface* iface = *iter;
    if (iface && iface->GetCurrentIPAddress() == hostname)
      return true;

     ++iter;
  }

  return false;
}

CNetworkInterface* CNetworkBase::GetFirstConnectedInterface()
{
  CNetworkInterface* fallbackInterface = nullptr;
  for (CNetworkInterface* iface : GetInterfaceList())
  {
    if (iface && iface->IsConnected())
    {
      if (!iface->GetCurrentDefaultGateway().empty())
        return iface;
      else if (fallbackInterface == nullptr)
        fallbackInterface = iface;
    }
  }

  return fallbackInterface;
}

bool CNetworkBase::HasInterfaceForIP(unsigned long address)
{
   unsigned long subnet;
   unsigned long local;
   std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
   std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
   while (iter != ifaces.end())
   {
      CNetworkInterface* iface = *iter;
      if (iface && iface->IsConnected())
      {
         subnet = ntohl(inet_addr(iface->GetCurrentNetmask().c_str()));
         local = ntohl(inet_addr(iface->GetCurrentIPAddress().c_str()));
         if( (address & subnet) == (local & subnet) )
            return true;
      }
      ++iter;
   }

   return false;
}

bool CNetworkBase::IsAvailable(void)
{
  std::vector<CNetworkInterface*>& ifaces = GetInterfaceList();
  return (ifaces.size() != 0);
}

bool CNetworkBase::IsConnected()
{
   return GetFirstConnectedInterface() != NULL;
}

void CNetworkBase::NetworkMessage(EMESSAGE message, int param)
{
  switch( message )
  {
    case SERVICES_UP:
      CLog::Log(LOGDEBUG, "{} - Starting network services", __FUNCTION__);
      m_services->Start();
      break;

    case SERVICES_DOWN:
      CLog::Log(LOGDEBUG, "{} - Signaling network services to stop", __FUNCTION__);
      m_services->Stop(false); // tell network services to stop, but don't wait for them yet
      CLog::Log(LOGDEBUG, "{} - Waiting for network services to stop", __FUNCTION__);
      m_services->Stop(true); // wait for network services to stop
      break;
  }
}

bool CNetworkBase::WakeOnLan(const char* mac)
{
  int i, j, packet;
  unsigned char ethaddr[8];
  unsigned char buf [128];
  unsigned char *ptr;

  // Fetch the hardware address
  if (!in_ether(mac, ethaddr))
  {
    CLog::Log(LOGERROR, "{} - Invalid hardware address specified ({})", __FUNCTION__, mac);
    return false;
  }

  // Setup the socket
  if ((packet = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    CLog::Log(LOGERROR, "{} - Unable to create socket ({})", __FUNCTION__, strerror(errno));
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
    CLog::Log(LOGERROR, "{} - Unable to set socket options ({})", __FUNCTION__, strerror(errno));
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
    CLog::Log(LOGERROR, "{} - Unable to send magic packet ({})", __FUNCTION__, strerror(errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "{} - Magic packet send to '{}'", __FUNCTION__, mac);
  return true;
}

// ping helper
static const char* ConnectHostPort(SOCKET soc, const struct sockaddr_in& addr, struct timeval& timeOut, bool tryRead)
{
  // set non-blocking
#ifdef TARGET_WINDOWS
  u_long nonblocking = 1;
  int result = ioctlsocket(soc, FIONBIO, &nonblocking);
#else
  int result = fcntl(soc, F_SETFL, fcntl(soc, F_GETFL) | O_NONBLOCK);
#endif

  if (result != 0)
    return "set non-blocking option failed";

  result = connect(soc, (const struct sockaddr *)&addr, sizeof(addr)); // non-blocking connect, will fail ..

  if (result < 0)
  {
#ifdef TARGET_WINDOWS
    if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
    if (errno != EINPROGRESS)
#endif
      return "unexpected connect fail";

    { // wait for connect to complete
      fd_set wset;
      FD_ZERO(&wset);
      FD_SET(soc, &wset);

      result = select(FD_SETSIZE, 0, &wset, 0, &timeOut);
    }

    if (result < 0)
      return "select fail";

    if (result == 0) // timeout
      return ""; // no error

    { // verify socket connection state
      int err_code = -1;
      socklen_t code_len = sizeof (err_code);

      result = getsockopt(soc, SOL_SOCKET, SO_ERROR, (char*) &err_code, &code_len);

      if (result != 0)
        return "getsockopt fail";

      if (err_code != 0)
        return ""; // no error, just not connected
    }
  }

  if (tryRead)
  {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(soc, &rset);

    result = select(FD_SETSIZE, &rset, 0, 0, &timeOut);

    if (result > 0)
    {
      char message [32];

      result = recv(soc, message, sizeof(message), 0);
    }

    if (result == 0)
      return ""; // no reply yet

    if (result < 0)
      return "recv fail";
  }

  return 0; // success
}

bool CNetworkBase::PingHost(unsigned long ipaddr, unsigned short port, unsigned int timeOutMs, bool readability_check)
{
  if (port == 0) // use icmp ping
    return PingHost (ipaddr, timeOutMs);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ipaddr;

  SOCKET soc = socket(AF_INET, SOCK_STREAM, 0);

  const char* err_msg = "invalid socket";

  if (soc != INVALID_SOCKET)
  {
    struct timeval tmout;
    tmout.tv_sec = timeOutMs / 1000;
    tmout.tv_usec = (timeOutMs % 1000) * 1000;

    err_msg = ConnectHostPort (soc, addr, tmout, readability_check);

    (void) closesocket (soc);
  }

  if (err_msg && *err_msg)
  {
#ifdef TARGET_WINDOWS
    std::string sock_err = CWIN32Util::WUSysMsg(WSAGetLastError());
#else
    std::string sock_err = strerror(errno);
#endif

    CLog::Log(LOGERROR, "{}({}:{}) - {} ({})", __FUNCTION__, inet_ntoa(addr.sin_addr), port,
              err_msg, sock_err);
  }

  return err_msg == 0;
}

//creates, binds and listens tcp sockets on the desired port. Set bindLocal to
//true to bind to localhost only.
std::vector<SOCKET> CreateTCPServerSocket(const int port, const bool bindLocal, const int backlog, const char *callerName)
{
#ifdef WINSOCK_VERSION
  int yes = 1;
#else
  unsigned int yes = 1;
#endif

  std::vector<SOCKET> sockets;
  struct addrinfo* results = nullptr;

  std::string sPort = std::to_string(port);
  struct addrinfo hints = {};
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;

  int error = getaddrinfo(bindLocal ? "localhost" : nullptr, sPort.c_str(), &hints, &results);
  if (error != 0)
    return sockets;

  for (struct addrinfo* result = results; result != nullptr; result = result->ai_next)
  {
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET)
      continue;

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&yes), sizeof(yes));

    if (bind(sock, result->ai_addr, result->ai_addrlen) != 0)
    {
      closesocket(sock);
      CLog::Log(LOGDEBUG, "{} Server: Failed to bind {} serversocket", callerName,
                result->ai_family == AF_INET6 ? "IPv6" : "IPv4");
      continue;
    }

    if (listen(sock, backlog) == 0)
      sockets.push_back(sock);
    else
    {
      closesocket(sock);
      CLog::Log(LOGERROR, "{} Server: Failed to set listen", callerName);
    }
  }
  freeaddrinfo(results);

  if (sockets.empty())
    CLog::Log(LOGERROR, "{} Server: Failed to create serversocket(s)", callerName);

  return sockets;
}

void CNetworkBase::WaitForNet()
{
  const int timeout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_WAITFORNETWORK);
  if (timeout <= 0)
    return; // wait for network is disabled

  // check if we have at least one network interface to wait for
  if (!IsAvailable())
    return;

  CLog::Log(LOGINFO, "{}: Waiting for a network interface to come up (Timeout: {} s)", __FUNCTION__,
            timeout);

  const static int intervalMs = 200;
  const int numMaxTries = (timeout * 1000) / intervalMs;

  for(int i=0; i < numMaxTries; ++i)
  {
    if (i > 0)
      KODI::TIME::Sleep(std::chrono::milliseconds(intervalMs));

    if (IsConnected())
    {
      CLog::Log(LOGINFO, "{}: A network interface is up after waiting {} ms", __FUNCTION__,
                i * intervalMs);
      return;
    }
  }

  CLog::Log(LOGINFO, "{}: No network interface did come up within {} s... Giving up...",
            __FUNCTION__, timeout);
}

std::string CNetworkBase::GetIpStr(const struct sockaddr* sa)
{
  std::string result;
  if (!sa)
    return result;

  char buffer[INET6_ADDRSTRLEN] = {};
  switch (sa->sa_family)
  {
  case AF_INET:
    inet_ntop(AF_INET, &reinterpret_cast<const struct sockaddr_in *>(sa)->sin_addr, buffer, INET_ADDRSTRLEN);
    break;
  case AF_INET6:
    inet_ntop(AF_INET6, &reinterpret_cast<const struct sockaddr_in6 *>(sa)->sin6_addr, buffer, INET6_ADDRSTRLEN);
    break;
  default:
    return result;
  }

  result = buffer;
  return result;
}

std::string CNetworkBase::GetMaskByPrefixLength(uint8_t prefixLength)
{
  if (prefixLength > 32)
    return "";

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(~((1 << (32u - prefixLength)) - 1));;
  return CNetworkBase::GetIpStr(reinterpret_cast<struct sockaddr*>(&sa));
}
