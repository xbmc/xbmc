/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "network/Network.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>

#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)
#include <errno.h>

#include <sys/socket.h>
#include <unistd.h>
#endif

class TestNetwork : public testing::Test
{
public:
  TestNetwork() = default;
  ~TestNetwork() = default;

  bool PingHost(const std::string& ip) const
  {
    static auto& network = CServiceBroker::GetNetwork();

    return network.PingHost(inet_addr(ip.c_str()), GetPort(), GetTimeout());
  }

  unsigned int GetPort() const { return m_port; }
  unsigned int GetTimeout() const { return m_timeoutMs; }

private:
  unsigned int m_port{0};
  unsigned int m_timeoutMs{100};
};

TEST_F(TestNetwork, PingHost)
{
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)
  // CNetworkLinux::PingHost() uses an unprivileged Linux "ping socket"
  // (SOCK_DGRAM + IPPROTO_ICMP), which the kernel refuses unless the
  // calling process's group is within net.ipv4.ping_group_range - disabled
  // by default on most distributions (see icmp(7)).
  int probe = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (probe < 0 && (errno == EACCES || errno == EPERM))
  {
    GTEST_SKIP() << "Unprivileged ICMP ping sockets are not permitted on this host (see "
                    "net.ipv4.ping_group_range in icmp(7))";
  }
  if (probe >= 0)
    close(probe);
#endif

  EXPECT_TRUE(PingHost("127.0.0.1"));
  EXPECT_FALSE(PingHost("10.254.254.254"));
}
