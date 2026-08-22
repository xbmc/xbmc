/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/tags/MatroskaTagReader.h"

#include <initializer_list>
#include <utility>

#include <gtest/gtest.h>

/*!
 * What both readers hand their caller, and what the caller makes of it: a chapter's play range,
 * whether the file left it open, and whether what is left once it is closed is a song. Neither
 * reader is involved - these are the rules the two share, so they are checked without one.
 */
using namespace MUSIC_INFO;

namespace
{
MatroskaAlbum AlbumOf(std::initializer_list<std::pair<double, double>> ranges)
{
  MatroskaAlbum album;
  for (const auto& [start, end] : ranges)
  {
    ChapterTags& chapter = album.chapters.emplace_back();
    chapter.start = start;
    chapter.end = end;
  }
  return album;
}
} // unnamed namespace

//! The case the file leaves open: a last chapter with no ChapterTimeEnd and no Segment Duration.
TEST(TestMatroskaTagChapterRanges, ClosesAnOpenChapterWithTheMeasuredLength)
{
  MatroskaAlbum album = AlbumOf({{0.0, 3.0}, {3.0, 6.0}, {6.0, 0.0}});
  CloseOpenEndedChapters(album, 9.0);

  EXPECT_DOUBLE_EQ(9.0, album.chapters[2].end);
  // What the file did say is what it keeps saying.
  EXPECT_DOUBLE_EQ(3.0, album.chapters[0].end);
  EXPECT_DOUBLE_EQ(6.0, album.chapters[1].end);
}

/*!
 * A measurement that does not reach the chapter measures something else - a container that
 * declares no length and holds too little to time, or a file whose chapters run past its media.
 * Writing it in would make the chapter end before it starts, which is what an artefact looks like.
 */
TEST(TestMatroskaTagChapterRanges, LeavesAChapterOpenWhenNothingCouldMeasureIt)
{
  MatroskaAlbum album = AlbumOf({{6.0, 0.0}});

  CloseOpenEndedChapters(album, 0.0);
  EXPECT_DOUBLE_EQ(0.0, album.chapters[0].end);

  CloseOpenEndedChapters(album, 6.0);
  EXPECT_DOUBLE_EQ(0.0, album.chapters[0].end);
}

/*!
 * The point of closing before judging: until the file is measured a trailing artefact and a last
 * song look the same, and only one of them is a track.
 */
TEST(TestMatroskaTagChapterRanges, TellsATrailingArtefactFromALastSongOnceClosed)
{
  EXPECT_TRUE(IsTrack(6.0, 0.0)) << "unmeasured, so taken on trust";

  MatroskaAlbum artefact = AlbumOf({{6.0, 0.0}});
  CloseOpenEndedChapters(artefact, 6.3);
  EXPECT_FALSE(IsTrack(artefact.chapters[0].start, artefact.chapters[0].end));

  MatroskaAlbum song = AlbumOf({{6.0, 0.0}});
  CloseOpenEndedChapters(song, 9.0);
  EXPECT_TRUE(IsTrack(song.chapters[0].start, song.chapters[0].end));
}

//! A chapter of exactly the minimum is a track: the bound is where a song starts, not where it
//! stops being an artefact.
TEST(TestMatroskaTagChapterRanges, KeepsAChapterOfExactlyOneSecond)
{
  EXPECT_TRUE(IsTrack(3.0, 4.0));
  EXPECT_FALSE(IsTrack(3.0, 3.999));
}
