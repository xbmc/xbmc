/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkLinux.h"

#include "utils/FileHandle.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <chrono>
#include <errno.h>
#include <limits>
#include <utility>

#include <arpa/inet.h>
#include <linux/if_addr.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace KODI::UTILS::POSIX;

namespace
{

constexpr unsigned int ICMP_PACKET_SIZE{64};
constexpr unsigned int TTL{64};

struct IcmpPacket
{
  icmphdr header;
  uint8_t data[ICMP_PACKET_SIZE - sizeof(icmphdr)];

  uint16_t Checksum()
  {
    auto data = reinterpret_cast<const uint16_t*>(this);
    unsigned int length = sizeof(IcmpPacket);

    unsigned int sum;

    for (sum = 0; length > 1; length -= 2)
    {
      sum += *data++;
    }

    if (length == 1)
    {
      sum += *data;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);

    sum += (sum >> 16);

    return ~sum;
  }
};

} // namespace

CNetworkInterfaceLinux::CNetworkInterfaceLinux(CNetworkPosix* network,
                                               std::string interfaceName,
                                               char interfaceMacAddrRaw[6])
  : CNetworkInterfacePosix(network, std::move(interfaceName), interfaceMacAddrRaw)
{
}

std::string CNetworkInterfaceLinux::GetCurrentDefaultGateway() const
{
  std::string result;

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
    n = sscanf(line, "%15s %127s %127s", iface, dst, gateway);

    if (n < 3)
      continue;

    if (strcmp(iface, m_interfaceName.c_str()) == 0 && strcmp(dst, "00000000") == 0 &&
        strcmp(gateway, "00000000") != 0)
    {
      unsigned char gatewayAddr[4];
      int len = CNetworkBase::ParseHex(gateway, gatewayAddr);
      if (len == 4)
      {
        struct in_addr in;
        in.s_addr = (gatewayAddr[0] << 24) | (gatewayAddr[1] << 16) | (gatewayAddr[2] << 8) |
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

bool CNetworkInterfaceLinux::GetHostMacAddress(unsigned long host_ip, std::string& mac) const
{
  struct arpreq areq;
  struct sockaddr_in* sin;

  memset(&areq, 0x0, sizeof(areq));

  sin = (struct sockaddr_in*)&areq.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = host_ip;

  sin = (struct sockaddr_in*)&areq.arp_ha;
  sin->sin_family = ARPHRD_ETHER;

  strncpy(areq.arp_dev, m_interfaceName.c_str(), sizeof(areq.arp_dev));
  areq.arp_dev[sizeof(areq.arp_dev) - 1] = '\0';

  int result = ioctl(m_network->GetSocket(), SIOCGARP, (caddr_t)&areq);

  if (result != 0)
  {
    //  CLog::Log(LOGERROR, "{} - GetHostMacAddress/ioctl failed with errno ({})", __FUNCTION__, errno);
    return false;
  }

  struct sockaddr* res = &areq.arp_ha;
  mac = StringUtils::Format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", (uint8_t)res->sa_data[0],
                            (uint8_t)res->sa_data[1], (uint8_t)res->sa_data[2],
                            (uint8_t)res->sa_data[3], (uint8_t)res->sa_data[4],
                            (uint8_t)res->sa_data[5]);

  for (int i = 0; i < 6; ++i)
    if (res->sa_data[i])
      return true;

  return false;
}

std::unique_ptr<CNetworkBase> CNetworkBase::GetNetwork()
{
  return std::make_unique<CNetworkLinux>();
}

CNetworkLinux::CNetworkLinux() : CNetworkPosix()
{
  queryInterfaceList();
}

void CNetworkLinux::GetMacAddress(const std::string& interfaceName, char rawMac[6])
{
  memset(rawMac, 0, 6);

  struct ifreq ifr;
  strcpy(ifr.ifr_name, interfaceName.c_str());
  if (ioctl(GetSocket(), SIOCGIFHWADDR, &ifr) >= 0)
  {
    memcpy(rawMac, ifr.ifr_hwaddr.sa_data, 6);
  }
}

void CNetworkLinux::queryInterfaceList()
{
  char macAddrRaw[6];
  m_interfaces.clear();

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
    std::string interfaceName = p;
    GetMacAddress(interfaceName, macAddrRaw);

    // only add interfaces with non-zero mac addresses
    if (macAddrRaw[0] || macAddrRaw[1] || macAddrRaw[2] || macAddrRaw[3] || macAddrRaw[4] ||
        macAddrRaw[5])
      m_interfaces.push_back(new CNetworkInterfaceLinux(this, interfaceName, macAddrRaw));
  }
  free(line);
  fclose(fp);
}

std::vector<std::string> CNetworkLinux::GetNameServers()
{
  std::vector<std::string> result;

  res_init();

  for (int i = 0; i < _res.nscount; i++)
  {
    if (_res.nsaddr_list[i].sin_family != AF_INET)
      continue;
    std::string ns = inet_ntoa(_res.nsaddr_list[i].sin_addr);
    result.push_back(ns);
  }
  return result;
}

// Gets the Highest Rank Permanent IPv6 Address on the current interface.
std::string CNetworkInterfaceLinux::GetCurrentIPv6Address() const
{
  std::string address;
  unsigned int ifindex = if_nametoindex(m_interfaceName.c_str());

  if (ifindex == 0)
    return address;

  // Open Netlink Socket
  int sock = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);

  if (sock < 0)
    return address;

  struct
  {
    struct nlmsghdr nlhdr;
    struct ifaddrmsg addrmsg;
  } message{};

  message.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  message.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
  message.nlhdr.nlmsg_type = RTM_GETADDR;
  message.addrmsg.ifa_family = AF_INET6;

  // Send the netlink message
  if (send(sock, &message, message.nlhdr.nlmsg_len, 0) < 0)
  {
    close(sock);
    return address;
  }

  unsigned int bestRank = 0;
  bool done = false;

  ssize_t length;
  alignas(struct nlmsghdr) char buffer[8192];

  // Receive until we have all the data from the kernel
  while (!done && (length = recv(sock, buffer, sizeof(buffer), 0)) > 0)
  {
    // Loop through every message in the response
    for (struct nlmsghdr* nlh = reinterpret_cast<struct nlmsghdr*>(buffer); NLMSG_OK(nlh, length);
         nlh = NLMSG_NEXT(nlh, length))
    {
      if (nlh->nlmsg_type == NLMSG_DONE || nlh->nlmsg_type == NLMSG_ERROR)
      {
        done = true;
        break;
      }

      // Anything other than an address record is something we didn't ask
      // for — skip it.
      if (nlh->nlmsg_type != RTM_NEWADDR)
        continue;

      // Extract the per-address fixed fields. NLMSG_DATA gives us
      // the start of the payload (an ifaddrmsg).
      struct ifaddrmsg* ifa = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));

      // Skip addresses on other interfaces — the dump returns all of them.
      // Skip anything that isn't globally scoped. RT_SCOPE_UNIVERSE (0) is
      // global; RT_SCOPE_LINK (253) is link-local fe80::/10; RT_SCOPE_HOST
      // is loopback.
      if (ifa->ifa_index != ifindex || ifa->ifa_scope != RT_SCOPE_UNIVERSE)
        continue;

      uint32_t flags = ifa->ifa_flags;
      const struct in6_addr* addr = nullptr;
      int rta_len = IFA_PAYLOAD(nlh); // bytes of attributes after ifaddrmsg

      // Loop through every attribute on the message and get the actual address
      // and the flags. Flags tell us the permanent addresses.
      for (struct rtattr* rta = IFA_RTA(ifa); RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len))
      {
        switch (rta->rta_type)
        {
          case IFA_ADDRESS: // the 16-byte in6_addr
            addr = static_cast<const struct in6_addr*>(RTA_DATA(rta));
            break;
          case IFA_FLAGS: // 32-bit superset of ifa_flags
            flags = *static_cast<const uint32_t*>(RTA_DATA(rta));
            break;
        }
      }

      // Remove anything that is not a permanent address
      if (!addr ||
          (flags & (IFA_F_TENTATIVE | IFA_F_DEPRECATED | IFA_F_DADFAILED | IFA_F_TEMPORARY)))
        continue;

      // There are still more types of permanent addresses, apply a rank to the types.
      // STABLE_PRIVACY: explicitly RFC 7217 — kernel-vouched stable
      // PERMANENT:      manually configured or EUI-64 SLAAC
      // MANAGETEMPADDR: the stable parent that generates temporaries
      // Anything else global that passed the filter
      unsigned int rank = 1;
      if (flags & IFA_F_STABLE_PRIVACY)
        rank = 4;
      else if (flags & IFA_F_PERMANENT)
        rank = 3;
      else if (flags & IFA_F_MANAGETEMPADDR)
        rank = 2;

      // Keep the highest rank
      if (rank <= bestRank)
        continue;

      // Format the winning candidate as a printable string and remember it.
      char str6[INET6_ADDRSTRLEN];
      if (inet_ntop(AF_INET6, addr, str6, sizeof(str6)) == nullptr)
        continue;

      address = str6;
      bestRank = rank;
    }
  }

  close(sock);
  return address;
}

// Gets the IPv6 default gateway on the current interface. If multiple default
// routes exist (e.g. router advertisements with different metrics), returns
// the one with the lowest metric (most preferred).
std::string CNetworkInterfaceLinux::GetCurrentIPv6DefaultGateway() const
{
  std::string gateway;
  unsigned int ifindex = if_nametoindex(m_interfaceName.c_str());

  if (ifindex == 0)
    return gateway;

  // Open Netlink Socket
  int sock = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);

  if (sock < 0)
    return gateway;

  struct
  {
    struct nlmsghdr nlhdr;
    struct rtmsg rtm;
  } message{};

  message.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  message.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
  message.nlhdr.nlmsg_type = RTM_GETROUTE;
  message.rtm.rtm_family = AF_INET6;

  // Send the netlink message
  if (send(sock, &message, message.nlhdr.nlmsg_len, 0) < 0)
  {
    close(sock);
    return gateway;
  }

  uint32_t bestMetric = std::numeric_limits<uint32_t>::max();
  bool done = false;

  ssize_t length;
  alignas(struct nlmsghdr) char buffer[8192];

  // Receive until we have all the data from the kernel
  while (!done && (length = recv(sock, buffer, sizeof(buffer), 0)) > 0)
  {
    // Loop through every message in the response
    for (struct nlmsghdr* nlh = reinterpret_cast<struct nlmsghdr*>(buffer); NLMSG_OK(nlh, length);
         nlh = NLMSG_NEXT(nlh, length))
    {
      if (nlh->nlmsg_type == NLMSG_DONE || nlh->nlmsg_type == NLMSG_ERROR)
      {
        done = true;
        break;
      }

      // Anything other than a route record is something we didn't ask for — skip it.
      if (nlh->nlmsg_type != RTM_NEWROUTE)
        continue;

      // Extract the per-route fixed fields. NLMSG_DATA gives us the start of
      // the payload (an rtmsg).
      struct rtmsg* rtm = static_cast<struct rtmsg*>(NLMSG_DATA(nlh));

      // Default routes only — destination prefix length 0, in the main routing
      // table, unicast. RT_TABLE_MAIN is where normal default routes live;
      // alternate tables (local, policy-based) are skipped.
      if (rtm->rtm_dst_len != 0 || rtm->rtm_table != RT_TABLE_MAIN || rtm->rtm_type != RTN_UNICAST)
        continue;

      const struct in6_addr* gw = nullptr;
      int oif = 0;
      uint32_t metric = 0;
      int rta_len = RTM_PAYLOAD(nlh); // bytes of attributes after rtmsg

      // Loop through every attribute on the message and pull out the gateway
      // address, output interface, and metric.
      for (struct rtattr* rta = RTM_RTA(rtm); RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len))
      {
        switch (rta->rta_type)
        {
          case RTA_GATEWAY: // the 16-byte in6_addr next-hop
            gw = static_cast<const struct in6_addr*>(RTA_DATA(rta));
            break;
          case RTA_OIF: // output interface index
            oif = *static_cast<const int*>(RTA_DATA(rta));
            break;
          case RTA_PRIORITY: // route metric (lower = preferred)
            metric = *static_cast<const uint32_t*>(RTA_DATA(rta));
            break;
        }
      }

      // Skip routes that aren't on our interface or have no gateway.
      if (!gw || static_cast<unsigned int>(oif) != ifindex)
        continue;

      // Lower metric = preferred. Strict < so the first route at a given
      // metric wins (kernel order is stable).
      if (metric >= bestMetric)
        continue;

      // Format the winning candidate as a printable string and remember it.
      char str6[INET6_ADDRSTRLEN];
      if (inet_ntop(AF_INET6, gw, str6, sizeof(str6)) == nullptr)
        continue;

      gateway = str6;
      bestMetric = metric;
    }
  }

  close(sock);
  return gateway;
}

std::vector<std::string> CNetworkLinux::GetIPv6NameServers()
{
  std::vector<std::string> result;

  res_init();

#if defined(__GLIBC__)
  for (int i = 0; i < MAXNS; i++)
  {
    const sockaddr_in6* sa6 = _res._u._ext.nsaddrs[i];
    if (!sa6)
      continue;

    char buf[INET6_ADDRSTRLEN] = {};
    if (inet_ntop(AF_INET6, &sa6->sin6_addr, buf, sizeof(buf)))
      result.emplace_back(buf);
  }
#endif
  return result;
}

bool CNetworkLinux::PingHost(unsigned long remote_ip, unsigned int timeout_ms)
{
  CFileHandle fd(socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_ICMP));
  if (!fd)
  {
    CLog::Log(LOGERROR, "socket failed: {} ({})", strerror(errno), errno);
    return false;
  }

  int ret = setsockopt(fd, SOL_IP, IP_TTL, &TTL, sizeof(TTL));
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "setsockopt failed: {} ({})", strerror(errno), errno);
    return false;
  }

  CFileHandle epfd(epoll_create1(EPOLL_CLOEXEC));
  if (!epfd)
  {
    CLog::Log(LOGERROR, "epoll_create1 failed: {} ({})", strerror(errno), errno);
    return false;
  }

  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = fd;
  ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "epoll_ctl failed: {} ({})", strerror(errno), errno);
    return false;
  }

  IcmpPacket packet = {};

  packet.header.type = ICMP_ECHO;
  packet.header.un.echo.id = getpid();

  packet.header.un.echo.sequence = 0;
  packet.header.checksum = packet.Checksum();

  sockaddr_in addr = {};
  addr.sin_addr.s_addr = remote_ip;
  addr.sin_family = AF_INET;

  const auto start = std::chrono::steady_clock::now();

  ret = sendto(fd, &packet, sizeof(packet), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  if (ret < 1)
  {
    CLog::Log(LOGERROR, "sendto failed: {} ({})", strerror(errno), errno);
    return false;
  }

  event = {};
  ret = epoll_wait(epfd, &event, 1, timeout_ms);
  if (ret < 1)
  {
    if (ret == 0)
    {
      CLog::Log(LOGERROR, "timed out while waiting to receive ({} ms)", timeout_ms);
    }
    else
    {
      CLog::Log(LOGERROR, "epoll_wait failed: {} ({})", strerror(errno), errno);
    }

    return false;
  }

  if (event.events & EPOLLIN)
  {
    socklen_t length = sizeof(addr);
    ret = recvfrom(fd, &packet, sizeof(packet), 0, reinterpret_cast<sockaddr*>(&addr), &length);
    if (ret < 1)
    {
      CLog::Log(LOGERROR, "recvfrom failed: {} ({})", strerror(errno), errno);
      return false;
    }
  }

  const auto end = std::chrono::steady_clock::now();

  if (!(packet.header.type == ICMP_ECHOREPLY && packet.header.code == 0))
  {
    CLog::Log(LOGERROR, "unexpected ping reply: type={} code={}", packet.header.type,
              packet.header.code);
    return false;
  }
  else
  {
    const auto duration =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start);

    CLog::Log(LOGDEBUG, "PING {}: icmp_seq={} ttl={} time={:0.3f} ms", inet_ntoa(addr.sin_addr),
              packet.header.un.echo.sequence, TTL, duration.count());
    return true;
  }
}
