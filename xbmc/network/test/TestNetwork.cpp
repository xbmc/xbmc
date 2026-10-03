/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "network/Network.h"

#include <future>
#include <mutex>

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

namespace
{
void ExpectInterfaceListLocked(std::mutex& mutex)
{
  EXPECT_FALSE(std::async(std::launch::async,
                          [&mutex]
                          {
                            std::unique_lock lock(mutex, std::try_to_lock);
                            return lock.owns_lock();
                          })
                   .get());
}

class CLockCheckingInterface : public CNetworkInterface
{
public:
  explicit CLockCheckingInterface(std::mutex& mutex) : m_mutex(mutex) {}

  bool IsEnabled() const override { return true; }
  bool IsConnected() const override
  {
    ExpectInterfaceListLocked(m_mutex);
    return true;
  }
  std::string GetMacAddress() const override { return {}; }
  void GetMacAddressRaw(char[6]) const override {}
  bool GetHostMacAddress(unsigned long, std::string&) const override { return false; }
  std::string GetCurrentIPAddress() const override
  {
    ExpectInterfaceListLocked(m_mutex);
    return "192.0.2.1";
  }
  std::string GetCurrentNetmask() const override
  {
    ExpectInterfaceListLocked(m_mutex);
    return "255.255.255.0";
  }
  std::string GetCurrentDefaultGateway() const override
  {
    ExpectInterfaceListLocked(m_mutex);
    return {};
  }

private:
  std::mutex& m_mutex;
};

class CLockCheckingNetwork : public CNetworkBase
{
public:
  std::unique_lock<std::mutex> LockInterfaceList() override { return std::unique_lock(m_mutex); }
  std::vector<CNetworkInterface*>& GetInterfaceList() override
  {
    ExpectInterfaceListLocked(m_mutex);
    return m_interfaces;
  }
  bool GetHostName(std::string&) override { return false; }
  bool PingHost(unsigned long, unsigned int) override { return false; }
  std::vector<std::string> GetNameServers() override { return {}; }

private:
  std::mutex m_mutex;
  CLockCheckingInterface m_interface{m_mutex};
  std::vector<CNetworkInterface*> m_interfaces{&m_interface};
};
} // namespace

TEST_F(TestNetwork, InterfaceListRemainsLockedDuringReads)
{
  CLockCheckingNetwork network;
  EXPECT_TRUE(network.IsAvailable());
  EXPECT_TRUE(network.IsLocalHost("192.0.2.1"));
  EXPECT_TRUE(network.HasInterfaceForIP(0xc0000202));
  EXPECT_NE(network.GetFirstConnectedInterface(), nullptr);
}
