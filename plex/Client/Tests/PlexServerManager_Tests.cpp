//
//  PlexServerManager_Tests.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 08/08/14.
//
//

#include "PlexTest.h"
#include "Client/PlexServerManager.h"

class PlexServerManagerFakeMock : public CPlexServerManager
{
public:
  MOCK_METHOD2(NotifyAboutServer, void(CPlexServerPtr, bool));
};

class PlexServerManagerTest : public ::testing::Test
{
  void SetUp()
  {
    g_plexApplication.serverManager = CPlexServerManagerPtr(new PlexServerManagerFakeMock);
  }
  
  void TearDown()
  {
    g_plexApplication.serverManager.reset();
  }
};

TEST_F(PlexServerManagerTest, update)
{
  PlexServerManagerFakeMock* serverManager = (PlexServerManagerFakeMock*)g_plexApplication.serverManager.get();
  // simulate local GDM discovery
  CPlexServerPtr server = PlexTestUtils::serverWithConnection();
  EXPECT_CALL(*serverManager, NotifyAboutServer(server, true)).Times(1);

  serverManager->UpdateFromDiscovery(server);
}