
#include "gtest/gtest.h"
#include "FileItem.h"
#include "Playlists/PlayQueueManager.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

TEST(PlayQueueManager, getURIFromItem)
{
  CFileItemPtr item(new CFileItem);
  item->SetPath("plexserver://abc123/library/sections/2/all");
  item->SetProperty("plexserver", "abc123");
  item->SetProperty("unprocessed_key", "/library/sections/2/all");
  item->m_bIsFolder = true;
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);

  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://abc123/directory/library/sections/2/all");

  item->m_bIsFolder = false;
  uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://abc123/item/library/sections/2/all");

  uri = CPlayQueueManager::getURIFromItem(CFileItemPtr());
  EXPECT_TRUE(uri.empty());
}

TEST(PlayQueueManager, getCreatePlayQueueURL)
{
  CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(
      CPlexConnection::CONNECTION_MANUAL, "10.0.42.200", 32400, "http", "token"));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(connection));
  server->SetUUID("abc123");

  CPlayQueueManager manager;

  CURL u = manager.getCreatePlayQueueURL(server, PLEX_MEDIA_TYPE_MUSIC,
                                         "library://abc123/item/library/sections/2/all");

  EXPECT_STREQ(u.GetProtocol(), "plexserver");
  EXPECT_STREQ(u.GetHostName(), "abc123");
  EXPECT_STREQ(u.GetFileName(), "playQueues");
  EXPECT_STREQ(u.GetOption("uri"), "library://abc123/item/library/sections/2/all");
  EXPECT_STREQ(u.GetOption("type"), "audio");
  EXPECT_FALSE(u.HasOption("shuffle"));
  EXPECT_FALSE(u.HasOption("limit"));
  EXPECT_FALSE(u.HasOption("continuous"));

  u = manager.getCreatePlayQueueURL(server, PLEX_MEDIA_TYPE_VIDEO,
                                    "library://abc123/directory/library/sections/2/all", "korv",
                                    true, false, 10);

  EXPECT_STREQ(u.GetOption("type"), "video");
  EXPECT_STREQ(u.GetOption("shuffle"), "1");
  EXPECT_FALSE(u.HasOption("continuous"));
  EXPECT_STREQ(u.GetOption("limit"), "10");

  u = manager.getCreatePlayQueueURL(CPlexServerPtr(), PLEX_MEDIA_TYPE_MUSIC, "");
  EXPECT_TRUE(u.Get().empty());

}
