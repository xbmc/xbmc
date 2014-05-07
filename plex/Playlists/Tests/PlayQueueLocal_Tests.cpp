#include "PlexTest.h"
#include "PlexPlayQueueLocal.h"
#include <boost/lexical_cast.hpp>
#include "PlexPlaylistPlayer.h"

class PlexPlayQueueLocalTests : public ::testing::Test
{
public:
  void SetUp()
  {
    pq = new CPlexPlayQueueLocal(CPlexServerPtr());
  }

  void TearDown()
  {
    delete pq;
    g_playlistPlayer.Clear();
    g_playlistPlayer.Reset();
  }

  CPlexPlayQueueLocal* pq;
};

TEST_F(PlexPlayQueueLocalTests, addItem_basic)
{
  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetLabel("1");

  EXPECT_TRUE(pq->addItem(item, false));

  CFileItemList list;
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(list.Size(), 1);
  EXPECT_STREQ(list.Get(0)->GetLabel(), "1");
}

TEST_F(PlexPlayQueueLocalTests, addItem_emptyNext)
{
  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetLabel("1");

  EXPECT_TRUE(pq->addItem(item, true));

  CFileItemList list;
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(list.Size(), 1);
  EXPECT_STREQ(list.Get(0)->GetLabel(), "1");
}

TEST_F(PlexPlayQueueLocalTests, addItem_tenNext)
{
  for (int i = 0; i < 10; i ++)
  {
    CFileItemPtr item = CFileItemPtr(new CFileItem);
    item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
    item->SetLabel(boost::lexical_cast<std::string>(i));
    EXPECT_TRUE(pq->addItem(item, false));
  }

  CFileItemList list;
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(list.Size(), 10);

  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetLabel("foo");
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_TRUE(pq->addItem(item, true));

  list.Clear();
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(11, list.Size());
  EXPECT_STREQ("foo", list.Get(0)->GetLabel());
}

TEST_F(PlexPlayQueueLocalTests, addItem_tenNextHavePosition)
{
  for (int i = 0; i < 10; i ++)
  {
    CFileItemPtr item = CFileItemPtr(new CFileItem);
    item->SetLabel(boost::lexical_cast<std::string>(i));
    item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
    EXPECT_TRUE(pq->addItem(item, false));
  }

  CFileItemList list;
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(list.Size(), 10);

  g_playlistPlayer.Add(PLAYLIST_VIDEO, list);
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  g_playlistPlayer.SetCurrentSong(2);

  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  item->SetLabel("foo");
  EXPECT_TRUE(pq->addItem(item, true));

  list.Clear();
  EXPECT_TRUE(pq->getCurrent(list));
  EXPECT_EQ(11, list.Size());
  EXPECT_STREQ("foo", list.Get(3)->GetLabel());
}
