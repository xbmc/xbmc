#include "gtest/gtest.h"
#include "PlexPlayQueueServer.h"
#include "Client/PlexServer.h"
#include "Tests/PlexTestUtils.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"

TEST(PlayQueueServerTestIsSupported, basic)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("abc123");
  server->SetVersion("0.9.9.9.0-abc123");

  EXPECT_TRUE(CPlexPlayQueueServer::isSupported(server));
}

TEST(PlayQueueServerTestIsSupported, secondary)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("abc123");
  server->SetVersion("0.9.9.9.0-abc123");
  server->SetServerClass("secondary");
  EXPECT_FALSE(CPlexPlayQueueServer::isSupported(server));
}

TEST(PlayQueueServerTestIsSupported, oldversion)
{
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer);
  server->SetUUID("abc123");
  server->SetVersion("0.9.9.0.0-abc123");
  EXPECT_FALSE(CPlexPlayQueueServer::isSupported(server));
}

class PlayQueueServerTest : public PlexServerManagerTestUtility
{
public:
  virtual void SetUp()
  {
    PlexServerManagerTestUtility::SetUp();
    pq = new CPlexPlayQueueServer(g_plexApplication.serverManager->FindByUUID("abc123"));
  }

  virtual void TearDown()
  {
    delete pq;
    PlexServerManagerTestUtility::TearDown();
  }

  CPlexPlayQueueServer* pq;
};


TEST_F(PlayQueueServerTest, GetPlayQueueURL_validItem)
{
  CURL u = pq->getPlayQueueURL(PLEX_MEDIA_TYPE_MUSIC, "library://abc123/item/library/sections/2/all");

  EXPECT_STREQ(u.GetProtocol(), "plexserver");
  EXPECT_STREQ(u.GetHostName(), "abc123");
  EXPECT_STREQ(u.GetFileName(), "playQueues");
  EXPECT_STREQ(u.GetOption("uri"), "library://abc123/item/library/sections/2/all");
  EXPECT_STREQ(u.GetOption("type"), "audio");
  EXPECT_FALSE(u.HasOption("shuffle"));
  EXPECT_FALSE(u.HasOption("limit"));
  EXPECT_FALSE(u.HasOption("continuous"));
  EXPECT_FALSE(u.HasOption("key"));
  EXPECT_FALSE(u.HasOption("next"));
}

TEST_F(PlayQueueServerTest, GetPlayQueueURL_limit)
{
  CURL u =
  pq->getPlayQueueURL(PLEX_MEDIA_TYPE_VIDEO,
                     "library://abc123/directory/library/sections/2/all", "korv", true, false, 10);

  EXPECT_STREQ(u.GetOption("type"), "video");
  EXPECT_STREQ(u.GetOption("shuffle"), "1");
  EXPECT_FALSE(u.HasOption("continuous"));
  EXPECT_STREQ(u.GetOption("limit"), "10");
}

TEST_F(PlayQueueServerTest, GetPlayQueueURL_haveKey)
{
  CURL u = pq->getPlayQueueURL(PLEX_MEDIA_TYPE_MUSIC, "uri", "item");
  EXPECT_STREQ(u.GetOption("key"), "item");
}

TEST_F(PlayQueueServerTest, GetPlayQueueURL_hasNext)
{
  CURL u = pq->getPlayQueueURL(PLEX_MEDIA_TYPE_MUSIC, "uri", "item", false, false, 0, true);
  EXPECT_STREQ(u.GetOption("key"), "item");
  EXPECT_TRUE(u.HasOption("next"));
}
