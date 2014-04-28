#include "gtest/gtest.h"
#include "GUI/GUIPlexMediaWindow.h"
#include "FileItem.h"

class PlexMediaWindowTests : public ::testing::Test
{
public:
  void SetUp()
  {
    mw = new CGUIPlexMediaWindow;
    CURL u("plexserver://abc123");
    mw->m_sectionFilter = CPlexSectionFilterPtr(new CPlexSectionFilter(u));
  }

  void TearDown()
  {
    delete mw;
    mw = NULL;
  }

  CGUIPlexMediaWindow* mw;
};

TEST_F(PlexMediaWindowTests, matchPlexContent_basic)
{
  mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
  EXPECT_TRUE(mw->MatchPlexContent("albums"));
}

TEST_F(PlexMediaWindowTests, matchPlexContent_twoArgs)
{
  mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
  EXPECT_TRUE(mw->MatchPlexContent("albums; songs"));
  EXPECT_TRUE(mw->MatchPlexContent("songs; albums"));
}

TEST_F(PlexMediaWindowTests, matchPlexContent_spaces)
{
  mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_TRUE(mw->MatchPlexContent("    movies  ; foo "));
}

TEST_F(PlexMediaWindowTests, matchPlexContent_nomatch)
{
  mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_FALSE(mw->MatchPlexContent("albums"));
}

TEST_F(PlexMediaWindowTests, matchPlexContent_cased)
{
  mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_MOVIE);
  EXPECT_TRUE(mw->MatchPlexContent("MOVIES"));
  EXPECT_TRUE(mw->MatchPlexContent("movies"));
  EXPECT_TRUE(mw->MatchPlexContent("MovIes"));
}

TEST_F(PlexMediaWindowTests, matchPlexFilter_basic)
{
  mw->m_sectionFilter->setPrimaryFilter("all");
  EXPECT_TRUE(mw->MatchPlexFilter("all"));
}

TEST_F(PlexMediaWindowTests, matchPlexFilter_cased)
{
  mw->m_sectionFilter->setPrimaryFilter("all");
  EXPECT_TRUE(mw->MatchPlexFilter("aLl"));
}

TEST_F(PlexMediaWindowTests, matchPlexFilter_nomatch)
{
  mw->m_sectionFilter->setPrimaryFilter("onDeck");
  EXPECT_FALSE(mw->MatchPlexFilter("aLl"));
}

TEST_F(PlexMediaWindowTests, matchPlexFilter_twoArgs)
{
  mw->m_sectionFilter->setPrimaryFilter("onDeck");
  EXPECT_TRUE(mw->MatchPlexFilter("all; onDeck"));
}
