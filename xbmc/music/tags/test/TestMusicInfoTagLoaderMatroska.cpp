/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "music/Artist.h"
#include "music/tags/MatroskaTagReader.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderMatroska.h"
#include "music/tags/TagLibVersion.h"
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

/*!
 * The single file path: a Matroska holding one song rather than an album, which never reaches
 * CAudioBookFileDirectory. The fixture comes from tools/testdata/mkmka.py, and what is asserted
 * holds for either reader.
 */
using namespace MUSIC_INFO;

TEST(TestMusicInfoTagLoaderMatroska, ReadsAFileWithNoChapters)
{
  const std::string path = XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/singlefile.mka");

  CMusicInfoTag tag;
  CMusicInfoTagLoaderMatroska loader;
  ASSERT_TRUE(loader.Load(path, tag, nullptr));

  EXPECT_TRUE(tag.Loaded());
  EXPECT_EQ("Live At The Test Venue", tag.GetAlbum());
  // The Segment title names the file, so it is the song; the album level TITLE is the album.
  EXPECT_EQ("So What", tag.GetTitle());
  // The file tags it at TargetTypeValue 50, which is the album's artist rather than the track's.
  EXPECT_EQ("The Test Band", tag.GetAlbumArtistString());
  /*!
   * The album's artist is the song's until the song says otherwise. A file that names an artist
   * only at album level - which is most of them - would otherwise scan in with none at all.
   */
  EXPECT_EQ("The Test Band", tag.GetArtistString());
  EXPECT_EQ("2026", tag.GetReleaseDate());

  bool composed = false;
  for (const auto& c : tag.GetContributors())
    composed = composed || (c.GetRoleDesc() == "Composer" && c.GetArtist() == "Bill Evans");
  EXPECT_TRUE(composed);
}

/*!
 * With no Segment title nothing names the song, and a file that says only what album it belongs
 * to would reach the library untitled. The album title stands in.
 */
TEST(TestMusicInfoTagLoaderMatroska, TitlesASongAfterItsAlbumWhenTheFileNamesNoSong)
{
  const std::string path =
      XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/singlefile-notitle.mka");

  CMusicInfoTag tag;
  CMusicInfoTagLoaderMatroska loader;
  ASSERT_TRUE(loader.Load(path, tag, nullptr));

  EXPECT_TRUE(tag.Loaded());
  EXPECT_EQ("Live At The Test Venue", tag.GetAlbum());
  EXPECT_EQ("Live At The Test Venue", tag.GetTitle());
}

/*!
 * A file whose only real chapter follows one too short to be a track is not an album, so this
 * loader reads it whole. The song it describes is the track, not the artefact before it.
 */
TEST(TestMusicInfoTagLoaderMatroska, SkipsAChapterTooShortToBeTheSong)
{
  const std::string path =
      XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/onetrackplusartefact.mka");

  CMusicInfoTag tag;
  CMusicInfoTagLoaderMatroska loader;
  ASSERT_TRUE(loader.Load(path, tag, nullptr));

  EXPECT_TRUE(tag.Loaded());
  EXPECT_EQ("Someone's Song", tag.GetTitle());
}

#ifdef HAS_TAGLIB_MATROSKA
/*!
 * A track level tag naming a ChapterUID the file does not contain describes no track of it - a
 * stale UID a tagger left behind, or one carried over from the file this was cut from. It falls
 * back to the file's tags, as a tag naming an absent chapter does whatever the chapter count.
 *
 * Reading the only chapter as the one every such tag meant gave it a title that names a chapter
 * somewhere else, and left the file's own title to whatever the Segment happened to say.
 *
 * TagLib only: the ChapterUID a tag names is part of the hierarchy that FFmpeg's demuxer
 * flattens, so only this reader can tell a stray one from a chapter's own.
 */
TEST(TestMusicInfoTagLoaderMatroska, DoesNotGiveTheOnlyChapterATagNamingAnotherOne)
{
  const CURL url(XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/straychapteruid.mka"));

  const MatroskaAlbum album = ReadMatroskaTags(url, nullptr);

  ASSERT_EQ(1u, album.chapters.size());

  // The chapter keeps the only name it was given, its ChapterDisplay - not the stray tag's.
  const auto chapterTitle = album.chapters[0].tags.find("TITLE");
  ASSERT_NE(album.chapters[0].tags.end(), chapterTitle);
  EXPECT_EQ("DISPLAY Only Track", chapterTitle->second);

  /*!
   * And the stray tag reached the file, where it outranks the Segment title: a tag the file
   * states outright says more about it than the container's own name for itself.
   */
  const auto fileTitle = album.fileTags.find("TITLE");
  ASSERT_NE(album.fileTags.end(), fileTitle);
  EXPECT_EQ("TAG Stray Title", fileTitle->second);
}
#endif
