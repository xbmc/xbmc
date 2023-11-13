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
  EXPECT_TRUE(PingHost("127.0.0.1"));
  EXPECT_FALSE(PingHost("10.254.254.254"));
}
