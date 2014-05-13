#include "PlexTest.h"
#include "GUIInfoManager.h"
#include "FileItem.h"

class PlexGUIInfoManagerTest : public ::testing::Test
{
  void TearDown()
  {
    g_infoManager.Clear();
    g_infoManager.ResetCurrentItem();
  }
};

TEST_F(PlexGUIInfoManagerTest, videoPlayerPlexContentBasic)
{
  CFileItemPtr item = CFileItemPtr(new CFileItem);
  item->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  item->SetPath("plexserver://abc123/library/metadata/123");

  g_infoManager.SetCurrentItem(*item);
  bool movie = g_infoManager.EvaluateBool("VideoPlayer.PlexContent(movie)");
  EXPECT_TRUE(movie);
}

TEST_F(PlexGUIInfoManagerTest, videoPlayerPlexContentNoItem)
{
  EXPECT_FALSE(g_infoManager.EvaluateBool("VideoPlayer.PlexContent(movie)"));
}
