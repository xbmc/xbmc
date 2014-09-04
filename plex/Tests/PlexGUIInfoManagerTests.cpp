#include "PlexTest.h"
#include "GUIInfoManager.h"
#include "FileItem.h"
#include "IPlayer.h"
#include "DVDPlayer.h"
#include "Application.h"
#include "PlexApplication.h"
#include "Playlists/PlexPlayQueueManager.h"
#include "Playlists/PlexPlayQueueServer.h"

class FakeVideoPlayer : public CDVDPlayer
{
public:
  FakeVideoPlayer() : CDVDPlayer(g_application)
  {
  }
  bool IsPlaying() const
  {
    return true;
  }
  bool HasVideo() const
  {
    return true;
  }
  int64_t GetTime()
  {
    return currentTime;
  }
  bool IsPaused() const
  {
    return false;
  }

  int64_t currentTime;
};

class PlexGUIInfoManagerTest : public ::testing::Test
{
public:
  void SetUp()
  {
    g_application.m_pPlayer = new FakeVideoPlayer;
  }

  void TearDown()
  {
    g_infoManager.Clear();
    g_infoManager.ResetCurrentItem();

    delete g_application.m_pPlayer;
    g_application.m_pPlayer = NULL;

    g_playlistPlayer.Clear();
    g_playlistPlayer.Reset();
  }
};

TEST_F(PlexGUIInfoManagerTest, videoPlayerPlexContentBasic)
{
  CFileItemList list;
  EXPECT_TRUE(PlexTestUtils::listFromXML(testItem_movie, list));
  CFileItemPtr item = list.Get(0);

  g_infoManager.SetCurrentItem(*item);
  EXPECT_TRUE(g_infoManager.EvaluateBool("VideoPlayer.PlexContent(movie)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("VideoPlayer.PlexContent(episode)"));
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerPlexContentNoItem)
{
  EXPECT_FALSE(g_infoManager.EvaluateBool("VideoPlayer.PlexContent(movie)"));
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerPropertyPlexContent)
{
  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  item->SetProperty("key", "plexserver://abc123/library/parts/2575/file.avi");

  // if it doesn't have a videoinfo tag it will not work
  item->GetVideoInfoTag();

  g_infoManager.SetCurrentItem(*item);
  int value = g_infoManager.TranslateSingleString("VideoPlayer.PlexContentStr");
  EXPECT_GT(value, 0);
  EXPECT_STREQ("movie", g_infoManager.GetLabel(value));
}

TEST_F(PlexGUIInfoManagerTest, playerOnNewTrue)
{
  ((FakeVideoPlayer*)g_application.m_pPlayer)->currentTime = 1000;
  EXPECT_TRUE(g_infoManager.EvaluateBool("Player.OnNew"));
}

TEST_F(PlexGUIInfoManagerTest, playerOnNewFalse)
{
  ((FakeVideoPlayer*)g_application.m_pPlayer)->currentTime = 6000;
  EXPECT_FALSE(g_infoManager.EvaluateBool("Player.OnNew"));
}

TEST_F(PlexGUIInfoManagerTest, playerOnNewStartOffset)
{
  ((FakeVideoPlayer*)g_application.m_pPlayer)->currentTime = (60 * 1000) + 1000;

  // resume one minute in
  g_application.CurrentFileItem().m_lStartOffset = STARTOFFSET_RESUME;
  g_application.CurrentFileItem().SetProperty("viewOffset", 60 * 1000);
  g_application.CurrentFileItem().SetProperty("type", "movie");
  EXPECT_TRUE(g_infoManager.EvaluateBool("Player.OnNew"));
}

TEST_F(PlexGUIInfoManagerTest, playerOnNewStartOffsetFalse)
{
  ((FakeVideoPlayer*)g_application.m_pPlayer)->currentTime = (60 * 1000) + 6000;

  // resume one minute in
  g_application.CurrentFileItem().m_lStartOffset = STARTOFFSET_RESUME;
  g_application.CurrentFileItem().SetProperty("viewOffset", 60 * 1000);
  g_application.CurrentFileItem().SetProperty("type", "movie");
  EXPECT_FALSE(g_infoManager.EvaluateBool("Player.OnNew"));
}

#define ADD_PL_ITEM(a)                                                                             \
  {                                                                                                \
    CFileItemPtr item = CFileItemPtr(new CFileItem);                                               \
    item->SetLabel(a);                                                                             \
    item->SetArt("thumb", std::string("thumb_") + a);                                              \
    g_playlistPlayer.Add(PLAYLIST_VIDEO, item);                                                    \
  }

TEST_F(PlexGUIInfoManagerTest, VideoPlayerOffset)
{
  ADD_PL_ITEM("1");
  ADD_PL_ITEM("2");
  ADD_PL_ITEM("3");

  int value = g_infoManager.TranslateString("VideoPlayer.Offset(2).Title");
  EXPECT_GT(value, 0);
  EXPECT_STREQ("2", g_infoManager.GetLabel(value));

  value = g_infoManager.TranslateString("VideoPlayer.Offset(3).Title");
  EXPECT_GT(value, 0);
  EXPECT_STREQ("3", g_infoManager.GetLabel(value));
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerOffsetArt)
{
  ADD_PL_ITEM("1");
  ADD_PL_ITEM("2");

  int value = g_infoManager.TranslateString("VideoPlayer.Offset(2).Cover");
  EXPECT_GT(value, 0);
  EXPECT_STREQ("thumb_2", g_infoManager.GetLabel(value));
}

TEST_F(PlexGUIInfoManagerTest, VideoPlayerHasNextFalse)
{
  EXPECT_FALSE(g_infoManager.EvaluateBool("VideoPlayer.HasNext"));
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerHasNextTrue)
{
  ADD_PL_ITEM("1");
  ADD_PL_ITEM("2");

  g_playlistPlayer.SetCurrentSong(0);
  EXPECT_TRUE(g_infoManager.EvaluateBool("VideoPlayer.HasNext"));
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerHasNextEndOfPL)
{
  ADD_PL_ITEM("1");
  ADD_PL_ITEM("2");

  g_playlistPlayer.SetCurrentSong(1);
  EXPECT_FALSE(g_infoManager.EvaluateBool("VideoPlayer.HasNext"));
}

class PlexPlayQueueManagerFake : public CPlexPlayQueueManager
{
public:
  PlexPlayQueueManagerFake(ePlexMediaType type, EPlexDirectoryType dirType)
    : CPlexPlayQueueManager(), m_type(type), m_dirType(dirType)
  { m_playQueues[m_type] = CPlexPlayQueuePtr(new CPlexPlayQueueServer(CPlexServerPtr(), m_type, 0)); }

  virtual EPlexDirectoryType getPlayQueueDirType(ePlexMediaType type) const { return m_dirType; }
  virtual ePlexMediaType getPlayQueueType(ePlexMediaType type) const { return m_type; }
 
  ePlexMediaType m_type;
  EPlexDirectoryType m_dirType;
};

class PlexGUIInfoManagerPlayQueueTest : public PlexGUIInfoManagerTest
{
public:
  void TearDown()
  {
    PlexGUIInfoManagerTest::TearDown();
    g_plexApplication.playQueueManager.reset();
  }
};

TEST_F(PlexGUIInfoManagerPlayQueueTest, music)
{
  g_plexApplication.playQueueManager =
      CPlexPlayQueueManagerPtr(new PlexPlayQueueManagerFake(PLEX_MEDIA_TYPE_MUSIC, PLEX_DIR_TYPE_ALBUM));
  EXPECT_TRUE(g_infoManager.EvaluateBool("System.PlexPlayQueue(music)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(video)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(clip)"));
}

TEST_F(PlexGUIInfoManagerPlayQueueTest, video)
{
  g_plexApplication.playQueueManager =
      CPlexPlayQueueManagerPtr(new PlexPlayQueueManagerFake(PLEX_MEDIA_TYPE_VIDEO, PLEX_DIR_TYPE_MOVIE));
  EXPECT_TRUE(g_infoManager.EvaluateBool("System.PlexPlayQueue(video)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(clip)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(music)"));
}

TEST_F(PlexGUIInfoManagerPlayQueueTest, clip)
{
  g_plexApplication.playQueueManager =
      CPlexPlayQueueManagerPtr(new PlexPlayQueueManagerFake(PLEX_MEDIA_TYPE_VIDEO, PLEX_DIR_TYPE_CLIP));
  EXPECT_TRUE(g_infoManager.EvaluateBool("System.PlexPlayQueue(clip)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(video)"));
  EXPECT_FALSE(g_infoManager.EvaluateBool("System.PlexPlayQueue(music)"));
}

TEST_F(PlexGUIInfoManagerPlayQueueTest, caseSensitive)
{
  g_plexApplication.playQueueManager =
      CPlexPlayQueueManagerPtr(new PlexPlayQueueManagerFake(PLEX_MEDIA_TYPE_VIDEO, PLEX_DIR_TYPE_CLIP));
  EXPECT_TRUE(g_infoManager.EvaluateBool("System.PlexPlayQueue(clip)"));
}
