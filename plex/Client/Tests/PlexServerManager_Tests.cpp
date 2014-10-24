//
//  PlexServerManager_Tests.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 08/08/14.
//
//

#include "PlexTest.h"
#include "PlexTypes.h"
#include "Client/PlexServerManager.h"

class PlexServerManagerFakeMock : public CPlexServerManager
{
public:
  MOCK_METHOD2(NotifyAboutServer, void(const CPlexServerPtr&, bool));
  MOCK_METHOD1(UpdateReachability, void(bool));
};

class PlexServerManagerTest : public ::testing::Test
{
public:
  void SetUp()
  {
    serverMgr = new PlexServerManagerFakeMock;
    g_plexApplication.serverManager = CPlexServerManagerPtr(serverMgr);
  }
  
  void TearDown()
  {
    serverMgr = NULL;
    g_plexApplication.serverManager.reset();
  }

  PlexServerManagerFakeMock* serverMgr;
};

TEST_F(PlexServerManagerTest, update)
{
  // simulate local GDM discovery
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  EXPECT_CALL(*serverMgr, NotifyAboutServer(server, true)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(1);

  serverMgr->UpdateFromDiscovery(server);

  PlexServerList list;
  list.push_back(server);

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  EXPECT_TRUE(serverMgr->GetBestServer());
  EXPECT_EQ(1, serverMgr->GetAllServers().size());
}

TEST_F(PlexServerManagerTest, updateAndRemove)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();

  EXPECT_CALL(*serverMgr, NotifyAboutServer(server, true)).Times(1);
  EXPECT_CALL(*serverMgr, NotifyAboutServer(server, false)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(2);

  serverMgr->UpdateFromDiscovery(server);

  PlexServerList list;
  list.push_back(server);

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  EXPECT_TRUE(serverMgr->GetBestServer());
  EXPECT_TRUE(server->Equals(serverMgr->GetBestServer()));

  PlexServerList emptyList;
  serverMgr->UpdateFromConnectionType(emptyList, CPlexConnection::CONNECTION_DISCOVERED);

  EXPECT_FALSE(serverMgr->FindByUUID(server->GetUUID()));
  EXPECT_FALSE(serverMgr->GetBestServer());
}

// This caused a nasty crash
TEST_F(PlexServerManagerTest, updateNoBestServer)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer("", "foo", true));

  PlexServerList list;
  list.push_back(server);

  EXPECT_CALL(*serverMgr, NotifyAboutServer(server, false)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(1);

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MANUAL);
}

TEST_F(PlexServerManagerTest, updateFromTwoSources)
{
  // a server that has a GDM connection
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   "10.10.10.10", 32400));
  CPlexServerPtr localServer = PlexTestUtils::serverWithConnection(conn);
  EXPECT_CALL(*serverMgr, NotifyAboutServer(localServer, true)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(2);

  PlexServerList list;
  list.push_back(localServer);

  // GDM update
  serverMgr->UpdateFromDiscovery(localServer);
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  // now we need a myPlex connection update
  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX,
                                                "80.80.80.80", 32400, "http", "token-myplex"));
  CPlexServerPtr onlineServer = PlexTestUtils::serverWithConnection(conn);
  PlexServerList onlineList;
  onlineList.push_back(onlineServer);
  serverMgr->UpdateFromConnectionType(onlineList, CPlexConnection::CONNECTION_MYPLEX);

  EXPECT_EQ(1, serverMgr->GetAllServers().size());
  EXPECT_TRUE(serverMgr->GetBestServer());

  std::vector<CPlexConnectionPtr> connections;
  serverMgr->GetBestServer()->GetConnections(connections);
  EXPECT_EQ(2, connections.size());

  EXPECT_STREQ("token-myplex", serverMgr->GetBestServer()->GetAccessToken());
}

TEST_F(PlexServerManagerTest, plexLocalAddressDuplicate)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   "10.10.10.10", 32400));
  CPlexServerPtr localServer = PlexTestUtils::serverWithConnection(conn);
  PlexServerList list;
  list.push_back(localServer);

  EXPECT_CALL(*serverMgr, NotifyAboutServer(localServer, true)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(2);

  serverMgr->UpdateFromDiscovery(localServer);
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX,
                                                "10.10.10.10", 32400, "http", "token"));
  CPlexServerPtr localAddressServer = PlexTestUtils::serverWithConnection(conn);
  list.clear();
  list.push_back(localAddressServer);

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MYPLEX);

  CPlexServerPtr bestServer = serverMgr->GetBestServer();
  EXPECT_TRUE(bestServer);
  EXPECT_STREQ(bestServer->GetAccessToken(), "token");

  std::vector<CPlexConnectionPtr> conns;
  bestServer->GetConnections(conns);

  EXPECT_EQ(1, conns.size());
}

TEST_F(PlexServerManagerTest, getAllServers)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  CPlexServerPtr server2 = PlexTestUtils::serverWithConnection();
  server2->SetUUID("123abc");

  PlexServerList list;
  list.push_back(server);
  list.push_back(server2);

  EXPECT_CALL(*serverMgr, UpdateReachability(false));

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  PlexServerList servers = serverMgr->GetAllServers();
  EXPECT_EQ(2, servers.size());
}

TEST_F(PlexServerManagerTest, getAllOwnedServers)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  CPlexServerPtr server2 = PlexTestUtils::serverWithConnection();
  server2->SetUUID("123abc");
  server2->SetOwned(false);

  PlexServerList list;
  list.push_back(server);
  list.push_back(server2);

  EXPECT_CALL(*serverMgr, UpdateReachability(false));

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  PlexServerList servers = serverMgr->GetAllServers(CPlexServerManager::SERVER_OWNED);
  EXPECT_EQ(1, servers.size());
}

TEST_F(PlexServerManagerTest, getAllOwnedActiveServers)
{
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();

  // server without connection.
  CPlexServerPtr server2 = CPlexServerPtr(new CPlexServer("abc3241", "test", true));

  PlexServerList list;
  list.push_back(server);
  list.push_back(server2);

  EXPECT_CALL(*serverMgr, UpdateReachability(false));
  EXPECT_CALL(*serverMgr, NotifyAboutServer(testing::_, testing::_)).Times(testing::AnyNumber());

  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  PlexServerList servers = serverMgr->GetAllServers(CPlexServerManager::SERVER_OWNED, true);
  EXPECT_EQ(1, servers.size());
  EXPECT_TRUE(servers.at(0)->GetOwned());
}

// This test simulates something that we saw in the wild after
// adding the merging of local & remote connections.
// When signed out of plex.tv it will actually retain the token
// on the local connection. We test for that now.
TEST_F(PlexServerManagerTest, localDuplicateRemoveTokenConnection)
{
  CPlexConnectionPtr conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_DISCOVERED,
                                                                   "10.10.10.10", 32400));
  CPlexServerPtr localServer = PlexTestUtils::serverWithConnection(conn);
  PlexServerList list;
  list.push_back(localServer);
  
  EXPECT_CALL(*serverMgr, NotifyAboutServer(localServer, true)).Times(1);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(3);
  
  serverMgr->UpdateFromDiscovery(localServer);
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);
  
  conn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX,
                                                "10.10.10.10", 32400, "http", "token"));
  CPlexServerPtr localAddressServer = PlexTestUtils::serverWithConnection(conn);
  list.clear();
  list.push_back(localAddressServer);
  
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MYPLEX);
  
  std::vector<CPlexConnectionPtr> conns;
  serverMgr->GetBestServer()->GetConnections(conns);
  
  EXPECT_STREQ("token", serverMgr->GetBestServer()->GetAccessToken());
  EXPECT_EQ(1, conns.size());
  
  // everything setup now. now remove the myPlex connection
  list.clear();
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MYPLEX);

  conns.clear();
  serverMgr->GetBestServer()->GetConnections(conns);
  EXPECT_EQ(1, conns.size());
  EXPECT_EQ(CPlexConnection::CONNECTION_DISCOVERED, conns[0]->m_type);
  EXPECT_TRUE(serverMgr->GetBestServer()->GetAccessToken().empty());
}

// This simulates a logout from myPlex event
TEST_F(PlexServerManagerTest, removeAllServers)
{
  PlexServerList list;
  list.push_back(PlexTestUtils::serverWithConnection("abc123", "10.0.42.2"));
  list.push_back(PlexTestUtils::serverWithConnection("abc321", "10.0.66.2"));

  EXPECT_CALL(*serverMgr, NotifyAboutServer(testing::AnyOf(list[0], list[1]), true)).Times(2);
  EXPECT_CALL(*serverMgr, UpdateReachability(false)).Times(1);

  // then both needs to be removed
  EXPECT_CALL(*serverMgr, NotifyAboutServer(testing::AnyOf(list[0], list[1]), false)).Times(2);

  serverMgr->UpdateFromDiscovery(list[0]);
  serverMgr->UpdateFromDiscovery(list[1]);
  serverMgr->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_DISCOVERED);

  serverMgr->RemoveAllServers();

  EXPECT_EQ(0, serverMgr->GetAllServers().size());
}
