#include "PlexTest.h"
#include "FileItem.h"
#include "Playlists/PlexPlayQueueManager.h"
#include "Client/PlexServer.h"
#include "Client/PlexConnection.h"

#include "PlayListPlayer.h"
#include "Tests/PlexTestUtils.h"
#include "music/tags/MusicInfoTag.h"

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

  CStdString uri = CPlexPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/directory/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, constructItemURL)
{
  CFileItem item = getURIItem();
  item.m_bIsFolder = false;
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://sectionUUID/item/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, badFileItem)
{
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(CFileItem());
  EXPECT_TRUE(uri.empty());
}

TEST(PlayQueueManagerGetURIFromItem, missingLibrarySectionUUID)
{
  CFileItem item = getURIItem();
  item.ClearProperty("librarySectionUUID");

  CStdString uri = CPlexPlayQueueManager::getURIFromItem(item);
  EXPECT_STREQ(uri, "library://whatever/directory/%2flibrary%2fsections%2f2%2fall");
}

TEST(PlayQueueManagerGetURIFromItem, badProtocol)
{
  CFileItem item = getURIItem();
  item.SetPath("file://foobaar");
  EXPECT_TRUE(CPlexPlayQueueManager::getURIFromItem(item).empty());
}

TEST(PlayQueueManagerGetURIFromItem, specifiedUri)
{
  CFileItem item = getURIItem();
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(item, "foobar");
  EXPECT_STREQ(uri, "library://sectionUUID/directory/foobar");
}

class PlayQueueManagerTest : public PlexServerManagerTestUtility
{
protected:
  virtual void SetUp()
  {
    PlexServerManagerTestUtility::SetUp();
    manager = CPlexPlayQueueManager();
  }

  virtual void TearDown()
  {
    g_playlistPlayer.Clear();
    PlexServerManagerTestUtility::TearDown();
  }

  CPlexPlayQueueManager manager;
};

#define newItem(list, a)                                                                           \
  {                                                                                                \
    CFileItemPtr item = CFileItemPtr(new CFileItem());                                            \
    item->GetMusicInfoTag()->SetDatabaseId(a, "song");                                                     \
    list.Add(item);                                                                                \
  }

TEST_F(PlayQueueManagerTest, ReconcilePlayQueueChanges_basic)
{
  CFileItemList list;
  newItem(list, 1);
  newItem(list, 2);
  newItem(list, 3);

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  // simulate a sliding window
  // our list is now 2, 3, 4
  list.Remove(0);
  newItem(list, 4);

  // reconcile changes and hope that the list
  // will be 2, 3, 4 as expected
  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);

  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 3);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[0]), 2);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[1]), 3);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[2]), 4);
}

TEST_F(PlayQueueManagerTest, ReconcilePlayQueueChanges_noMatching)
{
  CFileItemList list;
  newItem(list, 1);

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  list.Clear();
  newItem(list, 2);
  newItem(list, 3);

  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);
  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 2);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[0]), 2);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[1]), 3);
}

TEST_F(PlayQueueManagerTest, ReconcilePlayQueueChanges_gapInMiddle)
{
  CFileItemList list;
  newItem(list, 1);
  newItem(list, 2);
  newItem(list, 3);
  newItem(list, 4);

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);

  list.Remove(2);

  manager.reconcilePlayQueueChanges(PLAYLIST_VIDEO, list);
  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  EXPECT_EQ(playlist.size(), 3);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[0]), 1);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[1]), 2);
  EXPECT_EQ(PlexUtils::GetItemListID(playlist[2]), 4);
}

TEST_F(PlayQueueManagerTest, ReconcilePlayQueueChanges_largedataset)
{
  CFileItemList list;
  for (int i = 0; i < 100; i++)
    newItem(list, i);

  g_playlistPlayer.Add(PLAYLIST_MUSIC, list);

  list.Clear();

  for (int i = 60; i < 160; i++)
    newItem(list, i);

  manager.reconcilePlayQueueChanges(PLAYLIST_MUSIC, list);
  PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  EXPECT_EQ(playlist.size(), 100);
  for (int i = 60; i < 160; i++)
    EXPECT_EQ(PlexUtils::GetItemListID(playlist[i - 60]), i);
}

TEST_F(PlayQueueManagerTest, GetPlaylistFromType_basic)
{
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_MUSIC), PLAYLIST_MUSIC);
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_VIDEO), PLAYLIST_VIDEO);
  EXPECT_EQ(manager.getPlaylistFromType(PLEX_MEDIA_TYPE_PHOTO), PLAYLIST_NONE);
}

TEST_F(PlayQueueManagerTest, PlaylistImplementation)
{
  CFileItemList list;

  // get the playlist
  EXPECT_TRUE(PlexTestUtils::listFromXML(testItem_playlistMusic, list));
  list.SetPath("plexserver://abc123/playlists/123/items");
  CPlexPlayQueuePtr impl = manager.getImpl(list);
  EXPECT_STREQ("server", impl->implementationName().c_str());
}

TEST_F(PlayQueueManagerTest, channelImplementation)
{
  CFileItemList list;

  EXPECT_TRUE(PlexTestUtils::listFromXML(testItem_channelTwit, list));
  CPlexPlayQueuePtr impl = manager.getImpl(list);
  EXPECT_STREQ("local", impl->implementationName().c_str());
}

TEST_F(PlayQueueManagerTest, libraryImplementation)
{
  CFileItemList list;

  EXPECT_TRUE(PlexTestUtils::listFromXML(testItem_episodeListManyItems, list));
  CPlexPlayQueuePtr impl = manager.getImpl(list);
  EXPECT_STREQ("server", impl->implementationName().c_str());
}
