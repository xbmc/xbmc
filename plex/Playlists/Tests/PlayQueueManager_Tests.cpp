
#include "gtest/gtest.h"
#include "FileItem.h"
#include "Playlists/PlayQueueManager.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

#include "PlayListPlayer.h"

static const CFileItem& getURIItem()
{
  CFileItem* item = new CFileItem;
  item->SetPath("plexserver://abc123/library/sections/2/all");
  item->SetProperty("plexserver", "abc123");
  item->SetProperty("unprocessed_key", "/library/sections/2/all");
  item->SetProperty("librarySectionUUID", "sectionUUID");
  item->m_bIsFolder = true;
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
  return *item;
}

TEST(PlayQueueManagerGetURIFromItem, constructDirectoryURL)
{
  CFileItem item = getURIItem();

  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/directory/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, constructItemURL)
{
  CFileItem item = getURIItem();
  item.m_bIsFolder = false;
  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/item/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, badFileItem)
{
  CStdString uri = CPlayQueueManager::getURIFromItem(CFileItem());
  EXPECT_TRUE(uri.empty());
}

TEST(PlayQueueManagerGetURIFromItem, missingLibrarySectionUUID)
{
  CFileItem item = getURIItem();
  item.ClearProperty("librarySectionUUID");

  CStdString uri = CPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "");
}

TEST(PlayQueueManagerGetURIFromItem, badProtocol)
{
  CFileItem item = getURIItem();
  item.SetPath("file://foobaar");
  EXPECT_TRUE(CPlayQueueManager::getURIFromItem(item).empty());
}

TEST(PlayQueueManagerGetURIFromItem, specifiedUri)
{
  CFileItem item = getURIItem();
  CStdString uri = CPlayQueueManager::getURIFromItem(item, "foobar");
  EXPECT_STREQ(uri, "library://sectionUUID/directory/foobar");
}

static CPlexServerPtr getServer()
{
  CPlexConnectionPtr connection = CPlexConnectionPtr(
  new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "10.0.42.200", 32400, "http", "token"));
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
                                         "library://abc123/directory/library/sections/2/all",
                                         "korv", true, false, 10);

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

TEST(CPlayQueueManagerGetPlaylistFromString, basic)
{
  EXPECT_EQ(PLAYLIST_MUSIC, manager.getPlaylistFromString("audio"));
  EXPECT_EQ(PLAYLIST_VIDEO, manager.getPlaylistFromString("video"));
  EXPECT_EQ(PLAYLIST_PICTURE, manager.getPlaylistFromString("photo"));
  EXPECT_EQ(PLAYLIST_NONE, manager.getPlaylistFromString("bar"));
}

#define newItem(list, a)                                                                           \
  {                                                                                                \
    CFileItemPtr item = CFileItemPtr(new CFileItem(a));                                            \
    item->SetProperty("unprocessed_key", a);                                                       \
    list.Add(item);                                                                                \
  }

TEST(CPlayQueueManagerReconcilePlayQueueChanges, basic)
{
  CFileItemList list;
  newItem(list, "1");
  newItem(list, "2");
  newItem(list, "3");

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  // simulate a sliding window
  // our list is now 2, 3, 4
  list.Remove(0);
  newItem(list, "4");

  // reconcile changes and hope that the list
  // will be 2, 3, 4 as expected
  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);

  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 3);
  EXPECT_STREQ(playlist[0]->GetProperty("unprocessed_key").asString().c_str(), "2");
  EXPECT_STREQ(playlist[1]->GetProperty("unprocessed_key").asString().c_str(), "3");
  EXPECT_STREQ(playlist[2]->GetProperty("unprocessed_key").asString().c_str(), "4");

  g_playlistPlayer.Clear();
}

TEST(CPlayQueueManagerReconcilePlayQueueChanges, noMatching)
{
  CFileItemList list;
  newItem(list, "1");

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  list.Clear();
  newItem(list, "2");
  newItem(list, "3");

  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);
  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 2);
  EXPECT_STREQ(playlist[0]->GetProperty("unprocessed_key").asString().c_str(), "2");
  EXPECT_STREQ(playlist[1]->GetProperty("unprocessed_key").asString().c_str(), "3");

  g_playlistPlayer.Clear();
}

TEST(CPlayQueueManagerReconcilePlayQueueChanges, gapInMiddle)
{
  CFileItemList list;
  newItem(list, "1");
  newItem(list, "2");
  newItem(list, "3");
  newItem(list, "4");

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  list.Remove(2);

  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);
  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 3);
  EXPECT_STREQ(playlist[0]->GetProperty("unprocessed_key").asString().c_str(), "1");
  EXPECT_STREQ(playlist[1]->GetProperty("unprocessed_key").asString().c_str(), "2");
  EXPECT_STREQ(playlist[2]->GetProperty("unprocessed_key").asString().c_str(), "4");

  g_playlistPlayer.Clear();
}

TEST(CPlayQueueManagerGetPlaylistFromType, basic)
{
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_MUSIC), PLAYLIST_MUSIC);
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_VIDEO), PLAYLIST_VIDEO);
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_PHOTO), PLAYLIST_NONE);
}
