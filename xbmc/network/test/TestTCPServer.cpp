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
#include "utils/ScopeGuard.h"
#include "utils/Variant.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
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
class CFloodAnnouncer : public ANNOUNCEMENT::IAnnouncer
{
public:
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    if (message == "flood")
      m_announced.Set();
    else if (message == "barrier")
      m_barrierDispatched.Set();
  }

  CEvent m_announced;
  CEvent m_barrierDispatched;
};

CEvent requestBlocked{true};
CEvent releaseRequest{true};
std::atomic<bool> blockedRequestReturned{false};
CEvent laterRequestStarted{true};
std::promise<void> requestExited;
std::atomic<bool> stopClaimed{false};

JSONRPC_STATUS Block(const std::string& method,
                     ITransportLayer* transport,
                     IClient* client,
                     const CVariant& parameterObject,
                     CVariant& result)
{
  requestExited.set_value_at_thread_exit();
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
  // Teardown and this callback must not stop the same server concurrently.
  if (!stopClaimed.exchange(true))
  {
    requestExited.set_value_at_thread_exit();
    // As a request changing the services settings does, through StopJSONRPCServer(true)
    CTCPServer::StopServer(true);
    stoppedFromRequest.Set();
  }
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
    stopClaimed = false;
    requestExited = std::promise<void>{};
    m_exited = requestExited.get_future();

    m_added = CJSONServiceDescription::AddMethod(Schema("Test.Block"), Block) &&
              CJSONServiceDescription::AddMethod(Schema("Test.Record"), Record) &&
              CJSONServiceDescription::AddMethod(Schema("Test.Stop"), Stop);
  }

  ~CTestMethods()
  {
    releaseRequest.Set();
    // Self-stop clears the server pointer before its worker exits.
    if (stopClaimed.exchange(true))
      m_exited.wait();
    else
      CTCPServer::StopServer(true);
    if (requestBlocked.Signaled())
      m_exited.wait();
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
  std::future<void> m_exited;
};
} // namespace

namespace JSONRPC
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
    CJSONRPC::Initialize();
    m_methods = std::make_unique<CTestMethods>();
    ASSERT_TRUE(m_methods->Added());
    ASSERT_NO_FATAL_FAILURE(StartServer());
  }

  void TearDown() override
  {
    for (SOCKET client : m_clients)
      closesocket(client);
    m_methods.reset();
    if (m_announcementManager)
    {
      m_announcementManager->Deinitialize();
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);
    }
    CJSONRPC::Cleanup();
#if defined(TARGET_WINDOWS)
    WSACleanup();
#endif
  }

  void StartServer()
  {
    CTCPServer::StopServer(true);
    KODI::UTILS::CScopeGuard<SOCKET, INVALID_SOCKET, void(SOCKET)> listener(
        [](SOCKET socket) { closesocket(socket); }, socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_NE(INVALID_SOCKET, static_cast<SOCKET>(listener)) << SocketError();
#if defined(TARGET_WINDOWS)
    const int exclusive = 1;
    ASSERT_EQ(0, setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                            reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)))
        << SocketError();
#endif
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(0, bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)))
        << SocketError();
    socklen_t length = sizeof(address);
    ASSERT_EQ(0, getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length))
        << SocketError();
    ASSERT_EQ(0, listen(listener, 10)) << SocketError();
    m_port = ntohs(address.sin_port);

    // Transfer the bound socket without allowing another process to claim its port.
    auto server = std::shared_ptr<CTCPServer>(new CTCPServer(m_port, false));
    CTCPServer::ServerInstance = server;
    server->m_self = server;
    server->m_servers.push_back(listener);
    listener.release();
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(server.get());
    server->Create(false);
  }

  SOCKET Connect()
  {
    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == INVALID_SOCKET)
    {
      ADD_FAILURE() << "socket failed: " << SocketError();
      return client;
    }
    m_clients.push_back(client);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(m_port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(client, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
    {
      ADD_FAILURE() << "connect failed: " << SocketError();
      return INVALID_SOCKET;
    }
    return client;
  }

  static int SocketError()
  {
#if defined(TARGET_WINDOWS)
    return WSAGetLastError();
#else
    return errno;
#endif
  }

  static testing::AssertionResult ReadReply(SOCKET client, std::chrono::milliseconds timeout = 5s)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::string received;
    do
    {
      fd_set readable;
      FD_ZERO(&readable);
      FD_SET(client, &readable);
      const auto remaining = std::max(0us, std::chrono::duration_cast<std::chrono::microseconds>(
                                               deadline - std::chrono::steady_clock::now()));
      timeval wait = {static_cast<long>(remaining.count() / 1000000),
                      static_cast<decltype(timeval::tv_usec)>(remaining.count() % 1000000)};
      const int ready = select(static_cast<int>(client) + 1, &readable, nullptr, nullptr, &wait);
      if (ready < 0)
        return testing::AssertionFailure() << "select failed: " << SocketError();
      if (ready == 0)
        break;

      char buffer[4096];
      const int read = recv(client, buffer, sizeof(buffer), 0);
      if (read == 0)
        return testing::AssertionFailure() << "connection closed; response: " << received;
      if (read < 0)
        return testing::AssertionFailure()
               << "recv failed: " << SocketError() << "; response: " << received;
      received.append(buffer, read);
      CVariant reply;
      if (CJSONVariantParser::Parse(received, reply))
      {
        if (reply["jsonrpc"].asString() == "2.0" && reply["id"].asInteger() == 1 &&
            reply.isMember("result") && !reply.isMember("error"))
          return testing::AssertionSuccess() << received;
        return testing::AssertionFailure() << "JSON-RPC rejected the request: " << received;
      }
    } while (std::chrono::steady_clock::now() < deadline);
    return testing::AssertionFailure() << "no complete JSON-RPC response: " << received;
  }

  static testing::AssertionResult Exchange(SOCKET client, const std::string& request)
  {
    if (send(client, request.data(), static_cast<int>(request.size()), 0) !=
        static_cast<int>(request.size()))
      return testing::AssertionFailure() << "send failed: " << SocketError();
    return ReadReply(client);
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

  int m_port{0};
  std::vector<SOCKET> m_clients;
  std::unique_ptr<CTestMethods> m_methods;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_announcementManager;
  CFloodAnnouncer m_progress;
};
} // namespace JSONRPC

TEST_F(TestTCPServer, AnIdleConnectionHoldsNoWorker)
{
  const std::string request = R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})";

  for (int i = 0; i < 4; ++i)
  {
    const SOCKET client = Connect();
    ASSERT_NE(INVALID_SOCKET, client);
    ASSERT_EQ(static_cast<int>(request.size()),
              send(client, request.data(), static_cast<int>(request.size()), 0))
        << "socket error " << SocketError();
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
            send(client, first.data(), static_cast<int>(first.size()), 0))
      << "socket error " << SocketError();
  std::this_thread::sleep_for(200ms);
  EXPECT_TRUE(WaitForNoWorkers()) << "a half-sent request held a worker";

  ASSERT_EQ(static_cast<int>(rest.size()),
            send(client, rest.data(), static_cast<int>(rest.size()), 0))
      << "socket error " << SocketError();
  EXPECT_TRUE(ReadReply(client)) << "the request was not answered once complete";
}

TEST_F(TestTCPServer, APeerThatStopsReadingHoldsUpNoAnnouncement)
{
  // The test environment never starts its announcement thread, so run one for this test
  CTCPServer::StopServer(true);
  m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
  const auto manager = m_announcementManager =
      std::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  CServiceBroker::RegisterAnnouncementManager(manager);
  manager->Start();
  ASSERT_NO_FATAL_FAILURE(StartServer());

  const SOCKET stalled = Connect();
  ASSERT_NE(INVALID_SOCKET, stalled);
  ASSERT_TRUE(Exchange(stalled, R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1})"));

  // Not sent the flood, so only the stalled peer is sent more than it reads
  const SOCKET listener = Connect();
  ASSERT_NE(INVALID_SOCKET, listener);
  ASSERT_TRUE(Exchange(listener, R"({"jsonrpc":"2.0","method":"JSONRPC.SetConfiguration",)"
                                 R"("params":{"notifications":{"Other":false}},"id":1})"));

  // Far more than the socket buffers hold
  manager->AddAnnouncer(&m_progress, ANNOUNCEMENT::Other);
  const CVariant data(std::string(60000, 'x'));
  bool floodDispatched = true;
  for (int i = 0; i < 400; ++i)
  {
    manager->Announce(ANNOUNCEMENT::Other, "test", "flood", data);
    // Time delivery after the flood, rather than timing the serialization of all its data.
    if (!m_progress.m_announced.Wait(5s))
    {
      floodDispatched = false;
      break;
    }
  }
  // A later queue entry proves all callbacks for the flood have finished.
  manager->Announce(ANNOUNCEMENT::Other, "test", "barrier", CVariant{});
  const bool barrierDispatched = m_progress.m_barrierDispatched.Wait(5s);
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

  EXPECT_TRUE(floodDispatched) << "a peer that stopped reading held up announcement dispatch";
  EXPECT_TRUE(barrierDispatched) << "flood announcements did not finish dispatching";
  EXPECT_TRUE(delivered) << "a peer that stopped reading held up an announcement to another";
  EXPECT_TRUE(stoppedInTime) << "StopServer waited on a peer that stopped reading";
}

TEST_F(TestTCPServer, StopWaitsForARunningRequestAndStartsNoOther)
{
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);

  // One write, so the second request is already with the worker when the first blocks it
  const std::string requests = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})"
                               R"({"jsonrpc":"2.0","method":"Test.Record","id":2})";
  ASSERT_EQ(static_cast<int>(requests.size()),
            send(client, requests.data(), static_cast<int>(requests.size()), 0))
      << "socket error " << SocketError();
  ASSERT_TRUE(requestBlocked.Wait(5s)) << ReadReply(client, 0ms).message();

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
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.Stop","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0))
      << "socket error " << SocketError();

  EXPECT_TRUE(stoppedFromRequest.Wait(5s))
      << "stop callback entered: " << stopClaimed << "; " << ReadReply(client, 0ms).message();
}

TEST_F(TestTCPServer, StopWaitsForRequestsAfterTheServerThreadHasExited)
{
  const SOCKET client = Connect();
  ASSERT_NE(INVALID_SOCKET, client);
  const std::string request = R"({"jsonrpc":"2.0","method":"Test.Block","id":1})";
  ASSERT_EQ(static_cast<int>(request.size()),
            send(client, request.data(), static_cast<int>(request.size()), 0))
      << "socket error " << SocketError();
  ASSERT_TRUE(requestBlocked.Wait(5s)) << ReadReply(client, 0ms).message();

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
