/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/AudioBookFileDirectory.h"
#include "music/Artist.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/TagLibVersion.h"
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

/*!
 * The fixtures come from tools/testdata/mkmka.py, which is also where what each one holds is
 * written down. These hold for either reader, bar the one marked below.
 */
using namespace XFILE;

namespace
{
constexpr const char* DATA_PATH = "xbmc/filesystem/test/data/audiobook/";

//! Expand one fixture the way a music scan would.
void Expand(const std::string& fixture, CFileItemList& items)
{
  const CURL url(XBMC_REF_FILE_PATH(DATA_PATH + fixture));
  CAudioBookFileDirectory dir;
  ASSERT_TRUE(dir.ContainsFiles(url)) << fixture;
  ASSERT_TRUE(dir.GetDirectory(url, items)) << fixture;
}
} // unnamed namespace

TEST(TestAudioBookFileDirectory, MakesOneTrackPerTaggedChapter)
{
  CFileItemList items;
  Expand("chaptered.mka", items);

  ASSERT_EQ(3, items.Size());
  const auto& first = *items[0]->GetMusicInfoTag();
  EXPECT_EQ("Live At The Test Venue", first.GetAlbum());
  EXPECT_EQ("Opening Number", first.GetTitle());
  EXPECT_EQ("The Test Band", first.GetArtistString());
  EXPECT_EQ(1, first.GetTrackNumber());
  EXPECT_EQ(0, items[0]->GetStartOffset());
  EXPECT_EQ(3000, items[0]->GetEndOffset());

  EXPECT_EQ("Someone's Song", items[1]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ("Encore", items[2]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ(3, items[2]->GetMusicInfoTag()->GetTrackNumber());
  EXPECT_EQ(9000, items[2]->GetEndOffset());
}

//! A chapter named only by its ChapterDisplay still names its track, rather than taking the album's.
TEST(TestAudioBookFileDirectory, NamesTracksAfterTheirChapterDisplayName)
{
  CFileItemList items;
  Expand("chapternames-only.mka", items);

  ASSERT_EQ(3, items.Size());
  EXPECT_EQ("Opening Number", items[0]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ("Someone's Song", items[1]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ("Encore", items[2]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ("Live At The Test Venue", items[0]->GetMusicInfoTag()->GetAlbum());
}

//! Where a chapter has both, its own TITLE tag is the better source and wins.
TEST(TestAudioBookFileDirectory, PrefersAChapterTitleTagToItsDisplayName)
{
  CFileItemList items;
  Expand("precedence.mka", items);

  ASSERT_EQ(3, items.Size());
  EXPECT_EQ("TAG Opening Number", items[0]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ("TAG Encore", items[2]->GetMusicInfoTag()->GetTitle());
}

/*!
 * The fixture holds a 0.4s chapter between the first and second track. Dropping it must not shift
 * the tags of everything after it onto the wrong chapter, which is what indexing one reader's
 * chapter list by another reader's position did.
 */
TEST(TestAudioBookFileDirectory, DropsAMicroChapterWithoutShiftingTheRest)
{
  CFileItemList items;
  Expand("microchapter.mka", items);

  ASSERT_EQ(3, items.Size());
  EXPECT_EQ("Opening Number", items[0]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ(0, items[0]->GetStartOffset());
  EXPECT_EQ(3000, items[0]->GetEndOffset());

  EXPECT_EQ("Someone's Song", items[1]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ(3400, items[1]->GetStartOffset());
  EXPECT_EQ(6400, items[1]->GetEndOffset());

  EXPECT_EQ("Encore", items[2]->GetMusicInfoTag()->GetTitle());
  EXPECT_EQ(6400, items[2]->GetStartOffset());
  EXPECT_EQ(9400, items[2]->GetEndOffset());
}

//! PART_NUMBER is what the tagger meant the track to be, so it outranks the position in the file.
TEST(TestAudioBookFileDirectory, KeepsTheTrackNumberAChapterGivesItself)
{
  CFileItemList items;
  Expand("partnumber.mka", items);

  ASSERT_EQ(3, items.Size());
  EXPECT_EQ(5, items[0]->GetMusicInfoTag()->GetTrackNumber());
  EXPECT_EQ(6, items[1]->GetMusicInfoTag()->GetTrackNumber());
  EXPECT_EQ(7, items[2]->GetMusicInfoTag()->GetTrackNumber());
}

/*!
 * A file can hold several editions and only one of them is followed. Tags targeting a chapter of
 * an edition that was not selected describe tracks this file does not produce, so they must reach
 * neither the tracks nor the album.
 */
TEST(TestAudioBookFileDirectory, IgnoresTagsFromAnEditionItDidNotSelect)
{
  CFileItemList items;
  Expand("twoeditions.mka", items);

  ASSERT_EQ(3, items.Size());
  for (int i = 0; i < items.Size(); ++i)
  {
    EXPECT_NE("FOREIGN TITLE", items[i]->GetMusicInfoTag()->GetTitle()) << i;
    for (const auto& c : items[i]->GetMusicInfoTag()->GetContributors())
      EXPECT_NE("Foreign Composer", c.GetArtist()) << i;
  }
}

#ifdef HAS_TAGLIB_MATROSKA
/*!
 * The Matroska spec writes one SimpleTag per value, so three composers are three tags. Keeping only
 * the last is the fidelity FFmpeg's demuxer cannot offer and TagLib can.
 */
TEST(TestAudioBookFileDirectory, KeepsEveryValueOfARepeatedTag)
{
  CFileItemList items;
  Expand("repeated.mka", items);

  ASSERT_EQ(3, items.Size());
  const auto& contributors = items[0]->GetMusicInfoTag()->GetContributors();
  std::vector<std::string> composers;
  for (const auto& c : contributors)
    if (c.GetRoleDesc() == "Composer")
      composers.emplace_back(c.GetArtist());

  EXPECT_EQ((std::vector<std::string>{"Bill Evans", "Miles Davis", "Gil Evans"}), composers);
}

#endif // TagLib >= 2.3.1
