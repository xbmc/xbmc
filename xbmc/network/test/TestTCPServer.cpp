/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "network/TCPServer.h"
#include "threads/Event.h"
#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include <atomic>
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
CEvent requestBlocked{true};
CEvent releaseRequest{true};
std::atomic<bool> blockedRequestReturned{false};
CEvent laterRequestStarted{true};
CEvent largeReplyStarted{true};
constexpr size_t LARGE_REPLY_SIZE = 8 * 1024 * 1024;

JSONRPC_STATUS LargeReply(const std::string& method,
                          ITransportLayer* transport,
                          IClient* client,
                          const CVariant& parameterObject,
                          CVariant& result)
{
  result = std::string(LARGE_REPLY_SIZE, 'x');
  largeReplyStarted.Set();
  return OK;
}

JSONRPC_STATUS Block(const std::string& method,
                     ITransportLayer* transport,
                     IClient* client,
                     const CVariant& parameterObject,
                     CVariant& result)
{
  requestBlocked.Set();
  releaseRequest.Wait();
  blockedRequestReturned = true;
  result = "OK";
  return OK;
}

JSONRPC_STATUS Record(const std::string& method,
                      ITransportLayer* transport,
                      IClient* client,
                      const CVariant& parameterObject,
                      CVariant& result)
{
  laterRequestStarted.Set();
  result = "OK";
  return OK;
}

CEvent stoppedFromRequest{true};

JSONRPC_STATUS Stop(const std::string& method,
                    ITransportLayer* transport,
                    IClient* client,
                    const CVariant& parameterObject,
                    CVariant& result)
{
  // As a request changing the services settings does, through StopJSONRPCServer(true)
  CTCPServer::StopServer(true);
  stoppedFromRequest.Set();
  result = "OK";
  return OK;
}

// Test.Block holds its worker until releaseRequest is set; Test.Record only says it ran;
// Test.Stop stops the server it arrived on
class CTestMethods
{
public:
  CTestMethods()
  {
    requestBlocked.Reset();
    releaseRequest.Reset();
    blockedRequestReturned = false;
    laterRequestStarted.Reset();
    stoppedFromRequest.Reset();
    largeReplyStarted.Reset();

    CJSONRPC::Initialize();
    m_added = CJSONServiceDescription::AddMethod(Schema("Test.Block"), Block) &&
              CJSONServiceDescription::AddMethod(Schema("Test.Record"), Record) &&
              CJSONServiceDescription::AddMethod(Schema("Test.Stop"), Stop) &&
              CJSONServiceDescription::AddMethod(Schema("Test.LargeReply"), LargeReply);
  }

  ~CTestMethods()
  {
    releaseRequest.Set();
    CTCPServer::StopServer(true);
    CJSONRPC::Cleanup();
  }

  bool Added() const { return m_added; }

private:
  static std::string Schema(const std::string& name)
  {
    return R"(")" + name +
           R"(": {"type": "method", "description": "", "transport": "Response",
                  "permission": "ReadData", "params": [], "returns": "string"})";
  }

  bool m_added{false};
};

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

  static bool Exchange(SOCKET client, const std::string& request)
  {
    return send(client, request.data(), static_cast<int>(request.size()), 0) ==
               static_cast<int>(request.size()) &&
           ReadReply(client);
  }

  //! \brief Read everything the server sends until it includes the text, for at most the time
  static bool ReadUntil(SOCKET client, const std::string& text, std::chrono::milliseconds timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::string received;
    while (std::chrono::steady_clock::now() < deadline)
    {
      fd_set readable;
      FD_ZERO(&readable);
      FD_SET(client, &readable);
      timeval wait = {0, 100000};
      if (select(static_cast<int>(client) + 1, &readable, nullptr, nullptr, &wait) != 1)
        continue;

      char buffer[4096];
      const int read = recv(client, buffer, sizeof(buffer), 0);
      if (read <= 0)
        return false;
      received.append(buffer, read);
      if (received.find(text) != std::string::npos)
        return true;
      // Keep only what could hold the start of the text
      if (received.size() > text.size())
        received.erase(0, received.size() - text.size());
    }
    return false;
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

  static bool ReadToEOF(SOCKET client, std::string& received)
  {
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (std::chrono::steady_clock::now() < deadline)
    {
      fd_set readable;
      FD_ZERO(&readable);
      FD_SET(client, &readable);
      timeval timeout = {0, 100000};
      if (select(static_cast<int>(client) + 1, &readable, nullptr, nullptr, &timeout) != 1)
        continue;

      char buffer[65536];
      const int count = recv(client, buffer, sizeof(buffer), 0);
      if (count <= 0)
        return count == 0;
      received.append(buffer, count);
    }
    return false;
  }

  int m_port{0};
  std::vector<SOCKET> m_clients;
};

class CTestAnnouncementManager : public ANNOUNCEMENT::CAnnouncementManager
{
public:
  using CAnnouncementManager::DoAnnounce;
};

class TestTCPServerAnnouncements : public TestTCPServer
{
protected:
  void SetUp() override
  {
    m_previous = CServiceBroker::GetAnnouncementManager();
    m_manager = std::make_shared<CTestAnnouncementManager>();
    CServiceBroker::RegisterAnnouncementManager(m_manager);
    TestTCPServer::SetUp();
  }

  void TearDown() override
  {
    TestTCPServer::TearDown();
    CServiceBroker::RegisterAnnouncementManager(m_previous);
  }

  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previous;
  std::shared_ptr<CTestAnnouncementManager> m_manager;
};
} // namespace

TEST_F(TestTCPServerAnnouncements, WriteHalfCloseDrainsOutputWithoutAWorker)
{
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  ASSERT_TRUE(Exchange(client, R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})"));
  ASSERT_TRUE(WaitForNoWorkers());

  const std::string payload(LARGE_REPLY_SIZE, 'x');
  m_manager->DoAnnounce(ANNOUNCEMENT::Other, "test", "queued", CVariant{payload});
  ASSERT_EQ(0, shutdown(client, SHUT_WR));
  std::this_thread::sleep_for(200ms);
  std::string received;
  EXPECT_TRUE(ReadToEOF(client, received));
  CVariant announcement;
  ASSERT_TRUE(CJSONVariantParser::Parse(received, announcement));
  EXPECT_EQ(payload, announcement["params"]["data"].asString());
  EXPECT_TRUE(WaitForNoWorkers());
}

TEST_F(TestTCPServer, WriteHalfClosePreservesRunningAndQueuedRequests)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string first = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})";
  ASSERT_EQ(static_cast<int>(first.size()),
            send(client, first.data(), static_cast<int>(first.size()), 0));
  ASSERT_TRUE(requestBlocked.Wait(5s));

  const std::string second = R"({"jsonrpc":"2.0","method":"Test.Record","id":2})";
  ASSERT_EQ(static_cast<int>(second.size()),
            send(client, second.data(), static_cast<int>(second.size()), 0));
  ASSERT_EQ(0, shutdown(client, SHUT_WR));

  // Give the server a chance to observe EOF while the first request is still running.
  fd_set readable;
  FD_ZERO(&readable);
  FD_SET(client, &readable);
  timeval timeout = {0, 200000};
  EXPECT_EQ(0, select(static_cast<int>(client) + 1, &readable, nullptr, nullptr, &timeout));
  const auto releasedAt = std::chrono::steady_clock::now();
  releaseRequest.Set();

  std::string received;
  EXPECT_TRUE(ReadToEOF(client, received));
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                  releasedAt)
                .count(),
            500)
      << "EOF waited for the idle select timeout after the replies were sent";
  EXPECT_TRUE(WaitForNoWorkers());
  EXPECT_TRUE(laterRequestStarted.Wait(5s));

  // These two replies contain only scalar fields.
  const auto end = received.find('}');
  ASSERT_NE(std::string::npos, end);
  CVariant reply;
  ASSERT_TRUE(CJSONVariantParser::Parse(received.substr(0, end + 1), reply));
  EXPECT_EQ(1, reply["id"].asInteger());
  EXPECT_EQ("OK", reply["result"].asString());
  ASSERT_TRUE(CJSONVariantParser::Parse(received.substr(end + 1), reply));
  EXPECT_EQ(2, reply["id"].asInteger());
  EXPECT_EQ("OK", reply["result"].asString());
}

TEST_F(TestTCPServer, WriteHalfCloseDrainsALargeReply)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.LargeReply","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0));
  ASSERT_TRUE(largeReplyStarted.Wait(5s));
  ASSERT_EQ(0, shutdown(client, SHUT_WR));
  std::this_thread::sleep_for(200ms);
  EXPECT_GT(CTCPServer::GetActiveWorkers(), 0u) << "the unread reply should still be draining";

  std::string received;
  EXPECT_TRUE(ReadToEOF(client, received));
  EXPECT_TRUE(WaitForNoWorkers());
  CVariant reply;
  ASSERT_TRUE(CJSONVariantParser::Parse(received, reply));
  EXPECT_EQ(1, reply["id"].asInteger());
  EXPECT_EQ(std::string(LARGE_REPLY_SIZE, 'x'), reply["result"].asString());
}

TEST_F(TestTCPServer, WebSocketWriteHalfClosePreservesARunningRequest)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string handshake = "GET /jsonrpc HTTP/1.1\r\nHost: localhost\r\n"
                                "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                                "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                                "Sec-WebSocket-Version: 13\r\n\r\n";
  ASSERT_EQ(static_cast<int>(handshake.size()),
            send(client, handshake.data(), static_cast<int>(handshake.size()), 0));
  ASSERT_TRUE(ReadUntil(client, "101 Switching Protocols", 5s));

  const std::string request = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})";
  // A final text frame with a four-byte, all-zero client mask.
  std::string frame{static_cast<char>(0x81), static_cast<char>(0x80 | request.size())};
  frame.append(4, '\0');
  frame += request;
  ASSERT_EQ(static_cast<int>(frame.size()),
            send(client, frame.data(), static_cast<int>(frame.size()), 0));
  ASSERT_TRUE(requestBlocked.Wait(5s));
  ASSERT_EQ(0, shutdown(client, SHUT_WR));
  std::this_thread::sleep_for(200ms);
  releaseRequest.Set();

  std::string received;
  EXPECT_TRUE(ReadToEOF(client, received));
  EXPECT_TRUE(WaitForNoWorkers());
  ASSERT_GE(received.size(), 2u);
  EXPECT_EQ(0x81, static_cast<unsigned char>(received[0]));
  const auto length = static_cast<unsigned char>(received[1]);
  ASSERT_LT(length, 126u);
  ASSERT_GE(received.size(), 2u + length);
  CVariant reply;
  ASSERT_TRUE(CJSONVariantParser::Parse(received.substr(2, length), reply));
  EXPECT_EQ(1, reply["id"].asInteger());
  EXPECT_EQ("OK", reply["result"].asString());
}

TEST_F(TestTCPServer, WriteHalfCloseWithNoCompleteRequestCloses)
{
  for (const std::string& request : {std::string{}, std::string{R"({"jsonrpc":"2.0","method":)"}})
  {
    const SOCKET client = Connect();
    ASSERT_NE(INVALID_SOCKET, client);
    if (!request.empty())
    {
      ASSERT_EQ(static_cast<int>(request.size()),
                send(client, request.data(), static_cast<int>(request.size()), 0));
    }
    ASSERT_EQ(0, shutdown(client, SHUT_WR));
    std::string received;
    EXPECT_TRUE(ReadToEOF(client, received));
    EXPECT_TRUE(received.empty());
    EXPECT_TRUE(WaitForNoWorkers());
  }
}

TEST_F(TestTCPServer, StopAbortsAWriteHalfClosedNonreadingPeer)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.LargeReply","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0));
  ASSERT_TRUE(largeReplyStarted.Wait(5s));
  ASSERT_EQ(0, shutdown(client, SHUT_WR));
  std::this_thread::sleep_for(200ms);

  std::promise<void> stopped;
  auto done = stopped.get_future();
  std::thread stopper(
      [&stopped]
      {
        CTCPServer::StopServer(true);
        stopped.set_value();
      });
  const bool stoppedInTime = done.wait_for(5s) == std::future_status::ready;
  closesocket(client);
  m_clients.clear();
  stopper.join();
  EXPECT_TRUE(stoppedInTime);
}

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

TEST_F(TestTCPServer, APeerThatStopsReadingHoldsUpNoAnnouncement)
{
  // The test environment never starts its announcement thread, so run one for this test
  CTCPServer::StopServer(true);
  const auto previous = CServiceBroker::GetAnnouncementManager();
  const auto manager = std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  CServiceBroker::RegisterAnnouncementManager(manager);
  manager->Start();
  ASSERT_TRUE(CTCPServer::StartServer(m_port, false));

  const SOCKET stalled = Connect();
  ASSERT_NE(INVALID_SOCKET, stalled);
  ASSERT_TRUE(Exchange(stalled, R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})"));

  // Not sent the flood, so only the stalled peer is sent more than it reads
  const SOCKET listener = Connect();
  ASSERT_NE(INVALID_SOCKET, listener);
  ASSERT_TRUE(Exchange(listener, R"({"jsonrpc":"2.0","method":"JSONRPC.SetConfiguration",)"
                                 R"("params":{"notifications":{"Other":false}},"id":1})"));

  // Far more than the socket buffers hold
  const CVariant data(std::string(60000, 'x'));
  for (int i = 0; i < 400; ++i)
    manager->Announce(ANNOUNCEMENT::Other, "test", "flood", data);
  manager->Announce(ANNOUNCEMENT::Info, "test", "marker", CVariant{});

  const bool delivered = ReadUntil(listener, "marker", 5s);

  std::promise<void> stopped;
  auto done = stopped.get_future();
  std::thread stopper(
      [&stopped]
      {
        CTCPServer::StopServer(true);
        stopped.set_value();
      });
  const bool stoppedInTime = done.wait_for(5s) == std::future_status::ready;

  // Closing the peer fails a send blocked on it, so a hang ends here rather than in the suite
  closesocket(stalled);
  closesocket(listener);
  m_clients.clear();
  stopper.join();
  manager->Deinitialize();
  CServiceBroker::RegisterAnnouncementManager(previous);

  EXPECT_TRUE(delivered) << "a peer that stopped reading held up an announcement to another";
  EXPECT_TRUE(stoppedInTime) << "StopServer waited on a peer that stopped reading";
}

TEST_F(TestTCPServer, StopWaitsForARunningRequestAndStartsNoOther)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());

  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);

  // One write, so the second request is already with the worker when the first blocks it
  const std::string requests = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})"
                               R"({"jsonrpc":"2.0","method":"Test.Record","id":2})";
  ASSERT_EQ(static_cast<int>(requests.size()),
            send(client, requests.data(), static_cast<int>(requests.size()), 0));
  ASSERT_TRUE(requestBlocked.Wait(5s));

  std::promise<void> stopped;
  auto done = stopped.get_future();
  std::thread stopper(
      [&stopped]
      {
        CTCPServer::StopServer(true);
        stopped.set_value();
      });
  // The application tears down what a request reaches once StopServer(true) returns
  const bool stoppedEarly = done.wait_for(4s) == std::future_status::ready;
  releaseRequest.Set();
  const bool stoppedOnRelease = done.wait_for(5s) == std::future_status::ready;
  stopper.join();

  EXPECT_FALSE(stoppedEarly) << "StopServer returned while a request was still running";
  EXPECT_TRUE(stoppedOnRelease) << "StopServer did not return once the request finished";
  EXPECT_FALSE(laterRequestStarted.Wait(1s)) << "a request started after StopServer was called";
}

TEST_F(TestTCPServer, ARequestCanStopTheServerItArrivedOn)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());

  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.Stop","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0));

  EXPECT_TRUE(stoppedFromRequest.Wait(5s)) << "StopServer waited for the request calling it";

  // Let the request return before its method is removed
  std::this_thread::sleep_for(200ms);
}

TEST_F(TestTCPServer, StopWaitsForRequestsAfterTheServerThreadHasExited)
{
  CTestMethods methods;
  ASSERT_TRUE(methods.Added());

  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0));
  ASSERT_TRUE(requestBlocked.Wait(5s));

  // Application shutdown signals every service to stop before it waits for any of them
  CTCPServer::StopServer(false);
  for (auto waited = 0ms; CTCPServer::IsRunning() && waited < 5s; waited += 10ms)
    std::this_thread::sleep_for(10ms);
  ASSERT_FALSE(CTCPServer::IsRunning());

  std::thread releaser(
      []
      {
        std::this_thread::sleep_for(200ms);
        releaseRequest.Set();
      });
  CServiceBroker::GetNetwork().GetServices().StopJSONRPCServer(true);
  const bool returned = blockedRequestReturned;
  releaser.join();
  // Nothing may still be in a request once its method is removed
  CTCPServer::StopServer(true);

  EXPECT_TRUE(returned) << "StopJSONRPCServer(true) did not wait for a running request";
}
