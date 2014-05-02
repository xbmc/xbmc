#include "gtest/gtest.h"
#include "GUI/GUIPlexMediaWindow.h"
#include "FileItem.h"
#include "music/tags/MusicInfoTag.h"

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

class PlexMediaWindowUniformPropertyTest : public PlexMediaWindowTests
{
  void SetUp()
  {
    PlexMediaWindowTests::SetUp();

    for (int i = 0; i < 10; i ++)
    {
      CFileItemPtr item = CFileItemPtr(new CFileItem);
      item->GetMusicInfoTag()->SetAlbum("album");
      item->GetMusicInfoTag()->SetArtist("artist");
      mw->GetVecItems()->Add(item);
      mw->GetVecItems()->SetPlexDirectoryType(PLEX_DIR_TYPE_ALBUM);
    }
  }
};

TEST_F(PlexMediaWindowUniformPropertyTest, sameAlbum)
{
  EXPECT_TRUE(mw->MatchUniformProperty("album"));
}

TEST_F(PlexMediaWindowUniformPropertyTest, notSameAlbum)
{
  CFileItemPtr item2 = CFileItemPtr(new CFileItem);
  item2->GetMusicInfoTag()->SetAlbum("bar");
  mw->GetVecItems()->Add(item2);

  EXPECT_FALSE(mw->MatchUniformProperty("album"));
}

TEST_F(PlexMediaWindowUniformPropertyTest, sameArtist)
{
  EXPECT_TRUE(mw->MatchUniformProperty("artist"));
}

TEST_F(PlexMediaWindowUniformPropertyTest, notSameArtist)
{
  CFileItemPtr item2 = CFileItemPtr(new CFileItem);
  item2->GetMusicInfoTag()->SetArtist("bar");
  mw->GetVecItems()->Add(item2);

  EXPECT_FALSE(mw->MatchUniformProperty("artist"));
}

TEST_F(PlexMediaWindowUniformPropertyTest, cachedAlbum)
{
  EXPECT_TRUE(mw->MatchUniformProperty("album"));
  EXPECT_TRUE(mw->MatchUniformProperty("album"));
}
