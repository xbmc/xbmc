
#include "gtest/gtest.h"
#include "FileItem.h"
#include "Playlists/PlayQueueManager.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

#include "PlayListPlayer.h"

static CFileItemPtr getURIItem()
{
  CFileItemPtr item(new CFileItem);
  item->SetPath("plexserver://abc123/library/sections/2/all");
  item->SetProperty("plexserver", "abc123");
  item->SetProperty("unprocessed_key", "/library/sections/2/all");
  item->SetProperty("librarySectionUUID", "sectionUUID");
  item->m_bIsFolder = true;
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
  return item;
}

TEST(PlayQueueManagerGetURIFromItem, constructDirectoryURL)
{
  CFileItemPtr item = getURIItem();

  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/directory/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, constructItemURL)
{
  CFileItemPtr item = getURIItem();
  item->m_bIsFolder = false;
  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/item/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, badFileItem)
{
  CStdString uri = CPlayQueueManager::getURIFromItem(CFileItemPtr());
  EXPECT_TRUE(uri.empty());
}

TEST(PlayQueueManagerGetURIFromItem, missingLibrarySectionUUID)
{
  CFileItemPtr item = getURIItem();
  item->ClearProperty("librarySectionUUID");

  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "");
}

TEST(PlayQueueManagerGetURIFromItem, badProtocol)
{
  CFileItemPtr item = getURIItem();
  item->SetPath("file://foobaar");
  EXPECT_TRUE(CPlayQueueManager::getURIFromItem(item).empty());
}


static CPlexServerPtr getServer()
{
  CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(
      CPlexConnection::CONNECTION_MANUAL, "10.0.42.200", 32400, "http", "token"));
  CPlexServerPtr server = CPlexServerPtr(new CPlexServer(connection));
  server->SetUUID("abc123");

  return server;
}

static CPlayQueueManager manager;

TEST(PlayQueueManagerGetCreatePlayQueueURL, validItem)
{
  CPlexServerPtr server = getServer();

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
  EXPECT_FALSE(u.HasOption("key"));
}

TEST(PlayQueueManagerGetCreatePlayQueueURL, limit)
{
  CPlexServerPtr server = getServer();

  CURL u = manager.getCreatePlayQueueURL(server, PLEX_MEDIA_TYPE_VIDEO,
                                         "library://abc123/directory/library/sections/2/all", "korv",
                                         true, false, 10);

  EXPECT_STREQ(u.GetOption("type"), "video");
  EXPECT_STREQ(u.GetOption("shuffle"), "1");
  EXPECT_FALSE(u.HasOption("continuous"));
  EXPECT_STREQ(u.GetOption("limit"), "10");
}

TEST(PlayQueueManagerGetCreatePlayQueueURL, invalidServer)
{
  CURL u = manager.getCreatePlayQueueURL(CPlexServerPtr(), PLEX_MEDIA_TYPE_MUSIC, "");
  EXPECT_TRUE(u.Get().empty());
}

TEST(PlayQueueManagerGetCreatePlayQueueURL, haveKey)
{
  CPlexServerPtr server = getServer();
  CURL u = manager.getCreatePlayQueueURL(server, PLEX_MEDIA_TYPE_MUSIC, "uri", "item");
  EXPECT_STREQ(u.GetOption("key"), "item");
}

TEST(CPlayQueueManagerGetPlaylistFromString, music)
{
  EXPECT_EQ(PLAYLIST_MUSIC, manager.getPlaylistFromString("audio"));
}

TEST(CPlayQueueManagerGetPlaylistFromString, video)
{
  EXPECT_EQ(PLAYLIST_VIDEO, manager.getPlaylistFromString("video"));
}

TEST(CPlayQueueManagerGetPlaylistFromString, picture)
{
  EXPECT_EQ(PLAYLIST_PICTURE, manager.getPlaylistFromString("photo"));
}

TEST(CPlayQueueManagerGetPlaylistFromString, invalid)
{
  EXPECT_EQ(PLAYLIST_NONE, manager.getPlaylistFromString("bar"));
}
