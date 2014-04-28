#include "gtest/gtest.h"
#include "GUI/GUIPlexMediaWindow.h"
#include "FileItem.h"

TEST(GUIPlexMediaWindow_MatchPlexContent, basic)
{
  CGUIPlexMediaWindow mw;
  mw.GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);

  EXPECT_TRUE(mw.MatchPlexContent("albums"));
}

TEST(GUIPlexMediaWindow_MatchPlexContent, twoArgs)
{
  CGUIPlexMediaWindow mw;
  mw.GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
  EXPECT_TRUE(mw.MatchPlexContent("albums, songs"));
  EXPECT_TRUE(mw.MatchPlexContent("songs, albums"));
}

TEST(GUIPlexMediaWindow_MatchPlexContent, spaces)
{
  CGUIPlexMediaWindow mw;
  mw.GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_TRUE(mw.MatchPlexContent("    movies  , foo "));
}

TEST(GUIPlexMediaWindow_MatchPlexContent, nomatch)
{
  CGUIPlexMediaWindow mw;
  mw.GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_FALSE(mw.MatchPlexContent("albums"));
}
