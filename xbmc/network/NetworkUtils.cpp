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


#include "NetworkUtils.h"
#include "Application.h"
#include "Util.h"
#include "utils/log.h"

#if defined(TARGET_WINDOWS)
  // best attempt for window headers
  #include <winsock2.h>
  #include <iphlpapi.h>
  #include <IcmpAPI.h>
  #include <WS2tcpip.h>
  // undefine if you want to build without the wlan stuff
  // might be needed for VS2003
  #define HAS_WIN32_WLAN_API
  #ifdef HAS_WIN32_WLAN_API
    #include "Wlanapi.h"
    #pragma comment (lib,"Wlanapi.lib")
  #endif
#else
  #include <arpa/inet.h>
  #include <net/if_arp.h>
  #if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
    #include <sys/ioctl.h>
  #endif
  #include <sys/socket.h>
  #include <sys/wait.h>
#endif

/* slightly modified in_ether taken from the etherboot project (http://sourceforge.net/projects/etherboot) */
static bool in_ether(const char *bufp, unsigned char *addr)
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

  result = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); // non-blocking connect, will fail ..

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

static bool PingHost(unsigned long remote_ip, unsigned int timeout_ms /* = 2000 */)
{
#if defined(TARGET_WINDOWS)
  char SendData[]    = "poke";
  HANDLE hIcmpFile   = IcmpCreateFile();
  BYTE ReplyBuffer [sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];

  SetLastError(ERROR_SUCCESS);

  DWORD dwRetVal = IcmpSendEcho(hIcmpFile, remote_ip, SendData, sizeof(SendData), 
                                NULL, ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);

  DWORD lastErr = GetLastError();
  if (lastErr != ERROR_SUCCESS && lastErr != IP_REQ_TIMED_OUT)
    CLog::Log(LOGERROR, "%s - IcmpSendEcho failed - %s", __FUNCTION__, WUSysMsg(lastErr).c_str());

  IcmpCloseHandle (hIcmpFile);

  if (dwRetVal != 0)
  {
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
    return (pEchoReply->Status == IP_SUCCESS);
  }

  return false;
#else
  char cmd_line [64];

  struct in_addr host_ip; 
  host_ip.s_addr = remote_ip;

#if defined (TARGET_DARWIN_IOS) // no timeout option available
  sprintf(cmd_line, "ping -c 1 %s", inet_ntoa(host_ip));
#elif defined (TARGET_DARWIN) || defined (TARGET_FREEBSD)
  sprintf(cmd_line, "ping -c 1 -t %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));
#else
  sprintf(cmd_line, "ping -c 1 -w %d %s", timeout_ms / 1000 + (timeout_ms % 1000) != 0, inet_ntoa(host_ip));
#endif

  int status = system(cmd_line);

  int result = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

  // http://linux.about.com/od/commands/l/blcmdl8_ping.htm ;
  // 0 reply
  // 1 no reply
  // else some error

  if (result < 0 || result > 1)
    CLog::Log(LOGERROR, "Ping fail : status = %d, errno = %d : '%s'", status, errno, cmd_line);

  return result == 0;
#endif
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
std::string CNetworkUtils::IPTotring(unsigned int ip)
{
  char buffer[16];
  sprintf(buffer, "%i:%i:%i:%i", ip & 0xff, (ip & (0xff << 8)) >> 8, (ip & (0xff << 16)) >> 16, (ip & (0xff << 24)) >> 24);
  std::string returnString = buffer;
  return returnString;
}

bool CNetworkUtils::PingHost(unsigned long ipaddr, unsigned short port, unsigned int timeout_ms, bool readability_check)
{
  if (port == 0) // use icmp ping
    return ::PingHost(ipaddr, timeout_ms);

  struct sockaddr_in addr; 
  addr.sin_family = AF_INET; 
  addr.sin_port = htons(port); 
  addr.sin_addr.s_addr = ipaddr; 

  SOCKET soc = socket(AF_INET, SOCK_STREAM, 0); 

  const char* err_msg = "invalid socket";

  if (soc != INVALID_SOCKET)
  {
    struct timeval tmout; 
    tmout.tv_sec = timeout_ms / 1000; 
    tmout.tv_usec = (timeout_ms % 1000) * 1000; 

    err_msg = ConnectHostPort (soc, addr, tmout, readability_check);

    closesocket(soc);
  }

  if (err_msg && *err_msg)
  {
#ifdef TARGET_WINDOWS
    CStdString sock_err = WUSysMsg(WSAGetLastError());
#else
    CStdString sock_err = strerror(errno);
#endif

    CLog::Log(LOGERROR, "%s(%s:%d) - %s (%s)", __FUNCTION__, inet_ntoa(addr.sin_addr), port, err_msg, sock_err.c_str());
  }

  return err_msg == 0;
}

//creates, binds and listens a tcp socket on the desired port. Set bindLocal to
//true to bind to localhost only. The socket will listen over ipv6 if possible
//and fall back to ipv4 if ipv6 is not available on the platform.
int CNetworkUtils::CreateTCPServerSocket(const int port, const bool bindLocal, const int backlog, const char *callerName)
{
  struct sockaddr_storage addr;
  struct sockaddr_in6 *s6;
  struct sockaddr_in  *s4;
  int    sock;
  bool   v4_fallback = false;

#ifdef WINSOCK_VERSION
  int yes = 1;
  int no = 0;
#else
  unsigned int yes = 1;
  unsigned int no = 0;
#endif
  
  memset(&addr, 0, sizeof(addr));
  
  if (!v4_fallback &&
     (sock = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) >= 0)
  {
    // in case we're on ipv6, make sure the socket is dual stacked
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no)) < 0)
    {
#ifdef _MSC_VER
      CStdString sock_err = WUSysMsg(WSAGetLastError());
#else
      CStdString sock_err = strerror(errno);
#endif
      CLog::Log(LOGWARNING, "%s Server: Only IPv6 supported (%s)", callerName, sock_err.c_str());
    }
  }
  else
  {
    v4_fallback = true;
    sock = socket(PF_INET, SOCK_STREAM, 0);
  }
  
  if (sock == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "%s Server: Failed to create serversocket", callerName);
    return INVALID_SOCKET;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
  
  if (v4_fallback)
  {
    addr.ss_family = AF_INET;
    s4 = (struct sockaddr_in *) &addr;
    s4->sin_port = htons(port);

    if (bindLocal)
      s4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    else
      s4->sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    addr.ss_family = AF_INET6;
    s6 = (struct sockaddr_in6 *) &addr;
    s6->sin6_port = htons(port);

    if (bindLocal)
      s6->sin6_addr = in6addr_loopback;
    else
      s6->sin6_addr = in6addr_any;
  }

  if (::bind( sock, (struct sockaddr *) &addr,
            (addr.ss_family == AF_INET6) ? sizeof(struct sockaddr_in6) :
                                           sizeof(struct sockaddr_in)  ) < 0)
  {
    closesocket(sock);
    CLog::Log(LOGERROR, "%s Server: Failed to bind serversocket", callerName);
    return INVALID_SOCKET;
  }

  if (listen(sock, backlog) < 0)
  {
    closesocket(sock);
    CLog::Log(LOGERROR, "%s Server: Failed to set listen", callerName);
    return INVALID_SOCKET;
  }

  return sock;
}

bool CNetworkUtils::WakeOnLan(const char *mac_addr)
{
  int i, j, packet;
  unsigned char ethaddr[8];
  unsigned char buf[128];
  unsigned char *ptr;

  // Fetch the hardware address
  if (!in_ether(mac_addr, ethaddr))
  {
    CLog::Log(LOGERROR, "%s - Invalid hardware address specified (%s)", __FUNCTION__, mac_addr);
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
  if (setsockopt(packet, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(unsigned int) ) == SOCKET_ERROR)
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
  if (sendto(packet, (char*)buf, 102, 0, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to send magic packet (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "%s - Magic packet send to '%s'", __FUNCTION__, mac_addr);

  return true;
}

std::string CNetworkUtils::GetRemoteMacAddress(unsigned long host_ip)
{
  std::string mac_addr = "";

#ifdef TARGET_WINDOWS
  IPAddr src_ip = inet_addr(g_application.getNetwork().GetDefaultConnectionAddress().c_str());
  BYTE bPhysAddr[6] = {0xff}; // for 6-byte hardware addresses
  ULONG PhysAddrLen = 6;      // default to length of six bytes

  DWORD dwRetVal = SendARP(host_ip, src_ip, &bPhysAddr, &PhysAddrLen);
  if (dwRetVal == NO_ERROR)
  {
    if (PhysAddrLen == 6)
    {
      char macaddress[256] = {0};
      sprintf(macaddress, "%02X:%02X:%02X:%02X:%02X:%02X",
        bPhysAddr[0], bPhysAddr[1], bPhysAddr[2],
        bPhysAddr[3], bPhysAddr[4], bPhysAddr[5]);
      mac_addr = macaddress;
      return mac_addr;
    }
    else
      CLog::Log(LOGERROR, "%s - SendArp completed successfully, but mac address has length != 6 (%d)", __FUNCTION__, PhysAddrLen);
  }
  else
    CLog::Log(LOGERROR, "%s - SendArp failed with error (%d)", __FUNCTION__, dwRetVal);

#elif defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  size_t needed;
  int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO};

  if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &needed, NULL, 0) == 0)
  {   
    char *buf, *next;

    if (buf = (char*)malloc(needed))
    {      
      if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), buf, &needed, NULL, 0) == 0)
      {        
        struct rt_msghdr *rtm;
        struct sockaddr_dl *sdl;
        struct sockaddr_inarp *sin;

        for (next = buf; next < buf + needed; next += rtm->rtm_msglen) 
        {
          rtm = (struct rt_msghdr *)next;
          sin = (struct sockaddr_inarp *)(rtm + 1);
          sdl = (struct sockaddr_dl *)(sin + 1);

          if (host_ip != sin->sin_addr.s_addr || sdl->sdl_alen < 6)
            continue;

          u_char *cp = (u_char*)LLADDR(sdl);

          char macaddress[256] = {0};
          sprintf(macaddress, "%02X:%02X:%02X:%02X:%02X:%02X",
            cp[0], cp1], cp[2], cp[3], cp[4], cp[5]);
          mac_addr = macaddress;
          break;
        }
      }
      free(buf);
    }
  }

#else
  struct arpreq areq = {{0}};
  struct sockaddr_in *sin;

  sin = (struct sockaddr_in*)&areq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = host_ip;

  sin = (struct sockaddr_in*)&areq.arp_ha;
  sin->sin_family = ARPHRD_ETHER;

  strncpy(areq.arp_dev, g_application.getNetwork().GetDefaultConnectionInterfaceName().c_str(), sizeof(areq.arp_dev));
  areq.arp_dev[sizeof(areq.arp_dev)-1] = '\0';

  SOCKET soc = socket(AF_INET, SOCK_STREAM, 0); 
  int result = ioctl(soc, SIOCGARP, (caddr_t) &areq);
  closesocket(soc);

  if (result != 0)
  {
  //CLog::Log(LOGERROR, "%s - GetHostMacAddress/ioctl failed with errno (%d)", __FUNCTION__, errno);
    return mac_addr;
  }

  struct sockaddr *res = &areq.arp_ha;
  // check for no response
  for (int i=0; i<6; ++i)
    if (!res->sa_data[i])
      return mac_addr;

  char macaddress[256] = {0};
  sprintf(macaddress, "%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t) res->sa_data[0], (uint8_t) res->sa_data[1], (uint8_t) res->sa_data[2], 
    (uint8_t) res->sa_data[3], (uint8_t) res->sa_data[4], (uint8_t) res->sa_data[5]);
  mac_addr = macaddress;
#endif

  return mac_addr;
}

bool CNetworkUtils::HasInterfaceForIP(unsigned long address)
{
  if (g_application.getNetwork().IsConnected())
  {
    unsigned long subnet = ntohl(inet_addr(g_application.getNetwork().GetDefaultConnectionNetmask().c_str()));
    unsigned long local  = ntohl(inet_addr(g_application.getNetwork().GetDefaultConnectionAddress().c_str()));
    if( (address & subnet) == (local & subnet) )
      return true;
  }
  return false;
}
