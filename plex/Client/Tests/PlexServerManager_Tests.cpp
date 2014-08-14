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
