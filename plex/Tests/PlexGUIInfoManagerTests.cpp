#include "PlexTest.h"
#include "GUIInfoManager.h"
#include "FileItem.h"
#include "IPlayer.h"
#include "DVDPlayer.h"
#include "Application.h"

class FakeVideoPlayer  : public CDVDPlayer
{
public:
  FakeVideoPlayer() : CDVDPlayer(g_application) {}
  bool IsPlaying() const { return true; }
  bool HasVideo() const { return true; }
};

class PlexGUIInfoManagerTest : public ::testing::Test
{
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
  }
};

TEST_F(PlexGUIInfoManagerTest, videoPlayerPlexContentBasic)
{
  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  item->SetProperty("key", "plexserver://abc123/library/parts/2575/file.avi");

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
