/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "interfaces/AnnouncementManager.h"
#include "network/Network.h"
#include "network/TCPServer.h"
#include "utils/Variant.h"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>

using namespace JSONRPC;
using namespace std::chrono_literals;

namespace
{
class TestTCPServer : public testing::Test
{
protected:
  void SetUp() override
  {
#if defined(TARGET_WINDOWS)
    WSADATA wsaData;
    ASSERT_EQ(0, WSAStartup(MAKEWORD(2, 2), &wsaData));
#endif
    for (int port = 39190; port < 39290; ++port)
    {
      if (CTCPServer::StartServer(port, false))
      {
        m_port = port;
        break;
      }
    }
    ASSERT_NE(0, m_port) << "no free port for the JSON-RPC server";
  }

  void TearDown() override
  {
    for (SOCKET client : m_clients)
      closesocket(client);
    CTCPServer::StopServer(true);
#if defined(TARGET_WINDOWS)
    WSACleanup();
#endif
  }

  SOCKET Connect()
  {
    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == INVALID_SOCKET)
      return client;
    m_clients.push_back(client);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(m_port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(client, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
      return INVALID_SOCKET;
    return client;
  }

  static bool ReadReply(SOCKET client)
  {
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(client, &readable);
    timeval timeout = {5, 0};
    if (select(static_cast<int>(client) + 1, &readable, nullptr, nullptr, &timeout) != 1)
      return false;

    char buffer[4096];
    return recv(client, buffer, sizeof(buffer), 0) > 0;
  }

  static bool WaitForNoWorkers()
  {
    for (auto waited = 0ms; waited < 5s; waited += 10ms)
    {
      if (CTCPServer::GetActiveWorkers() == 0)
        return true;
      std::this_thread::sleep_for(10ms);
    }
    return false;
  }

  int m_port{0};
  std::vector<SOCKET> m_clients;
};
} // namespace

TEST_F(TestTCPServer, AnIdleConnectionHoldsNoWorker)
{
  const std::string request = R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})";

  for (int i = 0; i < 4; ++i)
  {
    const SOCKET client = Connect();
    ASSERT_NE(INVALID_SOCKET, client);
    ASSERT_EQ(static_cast<int>(request.size()),
              send(client, request.data(), static_cast<int>(request.size()), 0));
    ASSERT_TRUE(ReadReply(client)) << "connection " << i << " got no reply";
  }

  // Every request has been answered and every connection is still open
  EXPECT_TRUE(WaitForNoWorkers()) << CTCPServer::GetActiveWorkers() << " workers still running";
}

TEST_F(TestTCPServer, AnIncompleteRequestHoldsNoWorker)
{
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);

  // Answered once complete, so the first half has been read and handed to a worker by then
  const std::string first = R"({"jsonrpc":"2.0","method":)";
  const std::string rest = R"("JSONRPC.Ping","id":1})";
  ASSERT_EQ(static_cast<int>(first.size()),
            send(client, first.data(), static_cast<int>(first.size()), 0));
  std::this_thread::sleep_for(200ms);
  EXPECT_TRUE(WaitForNoWorkers()) << "a half-sent request held a worker";

  ASSERT_EQ(static_cast<int>(rest.size()),
            send(client, rest.data(), static_cast<int>(rest.size()), 0));
  EXPECT_TRUE(ReadReply(client)) << "the request was not answered once complete";
}

TEST_F(TestTCPServer, StopsWhileAnAnnouncementIsBlockedOnAPeerThatStoppedReading)
{
  // The test environment never starts its announcement thread, so run one for this test
  CTCPServer::StopServer(true);
  const auto previous = CServiceBroker::GetAnnouncementManager();
  const auto manager = std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  CServiceBroker::RegisterAnnouncementManager(manager);
  manager->Start();
  ASSERT_TRUE(CTCPServer::StartServer(m_port, false));

  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0));
  ASSERT_TRUE(ReadReply(client));

  // Far more than the socket buffers hold, so the announcement thread blocks in send()
  const CVariant data(std::string(60000, 'x'));
  for (int i = 0; i < 400; ++i)
    manager->Announce(ANNOUNCEMENT::Other, "test", "flood", data);
  std::this_thread::sleep_for(500ms);

  std::promise<void> stopped;
  auto done = stopped.get_future();
  std::thread stopper(
      [&stopped]
      {
        CTCPServer::StopServer(true);
        stopped.set_value();
      });
  const bool stoppedInTime = done.wait_for(5s) == std::future_status::ready;

  // Closing the peer fails the blocked send, so a hang ends here rather than in the suite
  closesocket(client);
  m_clients.clear();
  stopper.join();
  manager->Deinitialize();
  CServiceBroker::RegisterAnnouncementManager(previous);

  EXPECT_TRUE(stoppedInTime) << "StopServer waited on an announcement to a stalled peer";
}
