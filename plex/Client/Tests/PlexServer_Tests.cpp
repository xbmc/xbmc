#include "PlexTest.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

TEST(PlexServerGetLocalConnection, basic)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
  EXPECT_STREQ(localConn->GetAddress().Get(), "http://10.10.10.10:32400/");
}

TEST(PlexServerGetLocalConnection, noLocalConnection)
{
  // non local connection
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "8.8.8.8", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));

  EXPECT_FALSE(server->GetLocalConnection());
}

TEST(PlexServerGetLocalConnection, localConnectionActive)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(conn));
  server->SetActiveConnection(conn);

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
}

TEST(PlexServerGetLocalConnection, needSearchForLocalConnection)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer());

  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "8.8.8.8", 32400));
  server->AddConnection(conn);

  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.10.10.10", 32400));
  server->AddConnection(conn);

  CPlexConnectionPtr localConn = server->GetLocalConnection();
  EXPECT_TRUE(localConn);
  EXPECT_STREQ(localConn->GetAddress().Get(), "http://10.10.10.10:32400/");
}

class PlexFakeConnection : public CPlexConnection
{
public:
  PlexFakeConnection() : fakestate(CONNECTION_STATE_REACHABLE), delay(1)
  {

  }

  PlexFakeConnection(int type, const CStdString& host, int port, const CStdString& schema="http",
                     const CStdString& token="")
    : CPlexConnection(type, host, port, schema, token)
  {
    fakestate = CONNECTION_STATE_REACHABLE;
    delay = 1;
  }

  ConnectionState TestReachability(CPlexServerPtr server)
  {
    Sleep(delay);
    return fakestate;
  }

  ConnectionState fakestate;
  int delay;
};


TEST(PlexServerConnectionTest, basic)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  server->AddConnection(CPlexConnectionPtr(new PlexFakeConnection));
  EXPECT_TRUE(server->UpdateReachability());
}

TEST(PlexServerConnectionTest, testTwoDelay)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  CPlexConnectionPtr conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                      "10.0.0.1",
                                                                      32400));
  server->AddConnection(conn);
  conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                   "10.0.0.2",
                                                   32400));
  ((PlexFakeConnection*)conn.get())->delay = 100;
  server->AddConnection(conn);

  EXPECT_TRUE(server->UpdateReachability());
  EXPECT_STREQ(server->GetActiveConnectionURL().Get(), "http://10.0.0.1:32400/");
}

TEST(PlexServerConnectionTest, testTwoFail)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  CPlexConnectionPtr conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                      "10.0.0.1",
                                                                      32400));
  ((PlexFakeConnection*)conn.get())->fakestate = CPlexConnection::CONNECTION_STATE_UNREACHABLE;
  server->AddConnection(conn);
  conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                   "10.0.0.2",
                                                   32400));
  ((PlexFakeConnection*)conn.get())->delay = 100;
  server->AddConnection(conn);

  EXPECT_TRUE(server->UpdateReachability());
  EXPECT_STREQ(server->GetActiveConnectionURL().Get(), "http://10.0.0.2:32400/");
}

TEST(PlexServerConnectionTest, testDelayedResponse)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  CPlexConnectionPtr conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                      "10.0.0.1",
                                                                      32400));
  server->AddConnection(conn);
  conn = CPlexConnectionPtr(new PlexFakeConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                   "10.0.0.2",
                                                   32400));
  ((PlexFakeConnection*)conn.get())->delay = 500;
  server->AddConnection(conn);

  EXPECT_TRUE(server->UpdateReachability());
  EXPECT_STREQ(server->GetActiveConnectionURL().Get(), "http://10.0.0.1:32400/");


  // Now trigger a new connection test, this must be with a long delay
  // so we'll make sure that the old connection test hits at the same
  // time, since this casued a crash before
  ((PlexFakeConnection*)server->GetActiveConnection().get())->delay = 700;

  EXPECT_TRUE(server->UpdateReachability());
  EXPECT_STREQ(server->GetActiveConnectionURL().Get(), "http://10.0.0.2:32400/");
  
  server->CancelReachabilityTests();
}

TEST(PlexServerMerge, basic)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  CPlexServerPtr server2 = PlexTestUtils::serverWithConnection();
  server2->SetName("test name");
  
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX, "10.10.10.10", 32400, "http", "token"));
  server2->AddConnection(conn);
  
  server->Merge(server2);
  
  EXPECT_STREQ("test name", server->GetName());
  EXPECT_STREQ("token", server->GetAccessToken());
  
  std::vector<CPlexConnectionPtr> connections;
  server->GetConnections(connections);
  EXPECT_EQ(2, connections.size());
}


// This test simulates a normal connection merging. A server that has been discovered by local GDM
// and then adds a myplex connection with a token. We expect it to have the myPlex token after all is
// done
TEST(PlexServerMerge, mergedConnection)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   "10.0.0.1", 32400));
  CPlexServerPtr server = PlexTestUtils::serverWithConnection(conn);

  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX,
                                                "10.0.0.1", 32400, "http", "token2"));
  CPlexServerPtr server2 = PlexTestUtils::serverWithConnection(conn);
  
  server->Merge(server2);
  EXPECT_EQ("token2", server->GetAccessToken());
  
  std::vector<CPlexConnectionPtr> connections;
  server->GetConnections(connections);
  EXPECT_EQ(1, connections.size());
}

TEST(PlexServerBuildURL, noConnections)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  EXPECT_EQ("", (std::string)server->BuildURL("/foo").Get());
}

TEST(PlexServerBuildURL, noActiveConnection)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("abc123", "test", true));
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED, "10.0.0.1", 32400, "http"));
  server->AddConnection(conn);

  EXPECT_EQ("", (std::string)server->BuildURL("/foo").Get());
}

TEST(PlexServerBuildURL, working)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  EXPECT_EQ("http://10.0.0.1:32400/foo?X-Plex-Token=token", (std::string)server->BuildURL("/foo").Get());
}
