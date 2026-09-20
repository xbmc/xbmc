/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/FFmpegVfsContext.h"
#include "music/tags/MatroskaTagReader.h"
#include "music/tags/TagLibVersion.h"
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

extern "C"
{
#include <libavformat/avformat.h>
}

/*!
 * The FFmpeg reader, named directly so that it is exercised from a build that has TagLib - which
 * is every build the CI makes, and where ReadMatroskaTags() would pick the other reader.
 *
 * Expectations are written for this reader rather than derived from the TagLib one. Where the two
 * agree the values are simply the same on both sides, which is the point; where they do not, the
 * difference is stated here rather than hidden behind a tolerance.
 */
using namespace MUSIC_INFO;

namespace
{
//! Opens a fixture and reads it with the FFmpeg reader.
class Fixture
{
public:
  explicit Fixture(const std::string& name)
  {
    m_path = XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/" + name);
    if (m_demux.Open(m_path))
      m_album = ReadMatroskaTagsWithFFmpeg(CURL(m_path), m_demux.FormatContext());
  }

  const MatroskaAlbum& album() const { return m_album; }
  const AVFormatContext* context() const { return m_demux.FormatContext(); }
  const std::string& path() const { return m_path; }

private:
  std::string m_path;
  XFILE::CFFmpegVfsContext m_demux;
  MatroskaAlbum m_album;
};
} // unnamed namespace

//! FFmpeg carries the level as a prefix on the name; a reader owes its caller the level itself.
TEST(TestMatroskaTagReaderFFmpeg, SortsAlbumLevelTagsFromTheFilesOwn)
{
  const Fixture f("singlefile.mka");
  const auto& album = f.album().albumTags;

  ASSERT_TRUE(f.album().hasAlbumTags());
  EXPECT_EQ("Live At The Test Venue", album.at("TITLE"));
  EXPECT_EQ("The Test Band", album.at("ARTIST"));
  EXPECT_EQ("2026", album.at("DATE_RELEASED"));
  EXPECT_EQ("Bill Evans", album.at("COMPOSER"));

  // The Segment title names the file, not the album, and carries no level.
  EXPECT_EQ("So What", f.album().fileTags.at("TITLE"));
  EXPECT_EQ(0u, f.album().albumTags.count("ALBUM/TITLE"));
}

TEST(TestMatroskaTagReaderFFmpeg, ReadsChapterTagsAndRanges)
{
  const Fixture f("chaptered.mka");
  const auto& chapters = f.album().chapters;

  ASSERT_EQ(3u, chapters.size());
  EXPECT_EQ("Opening Number", chapters[0].tags.at("TITLE"));
  EXPECT_EQ("The Test Band", chapters[0].tags.at("ARTIST"));
  EXPECT_EQ("1", chapters[0].tags.at("TRACK"));
  EXPECT_DOUBLE_EQ(0.0, chapters[0].start);
  EXPECT_DOUBLE_EQ(3.0, chapters[0].end);

  EXPECT_EQ("Someone's Song", chapters[1].tags.at("TITLE"));
  EXPECT_EQ("Encore", chapters[2].tags.at("TITLE"));
  EXPECT_DOUBLE_EQ(9.0, chapters[2].end);
}

//! A chapter named only by its ChapterDisplay arrives as a title, as it does through TagLib.
TEST(TestMatroskaTagReaderFFmpeg, NamesChaptersFromTheirDisplayName)
{
  const Fixture f("chapternames-only.mka");
  const auto& chapters = f.album().chapters;

  ASSERT_EQ(3u, chapters.size());
  EXPECT_EQ("Opening Number", chapters[0].tags.at("TITLE"));
  EXPECT_EQ("Encore", chapters[2].tags.at("TITLE"));
}

//! A chapter's own TITLE outranks its display name here too.
TEST(TestMatroskaTagReaderFFmpeg, PrefersAChapterTitleTagToItsDisplayName)
{
  const Fixture f("precedence.mka");

  ASSERT_EQ(3u, f.album().chapters.size());
  EXPECT_EQ("TAG Opening Number", f.album().chapters[0].tags.at("TITLE"));
}

/*!
 * FFmpeg has no notion of an edition: it nests every EditionEntry's chapters into one list and
 * keeps whichever starts after the last it kept. A second edition that reruns the same timeline
 * therefore disappears on its own, and its tags with it - no filtering needed.
 */
TEST(TestMatroskaTagReaderFFmpeg, SwallowsAnEditionThatRerunsTheTimeline)
{
  const Fixture f("twoeditions.mka");

  ASSERT_EQ(3u, f.album().chapters.size());
  for (const auto& chapter : f.album().chapters)
    EXPECT_EQ(0u, chapter.tags.count("COMPOSER"));
  EXPECT_EQ(0u, f.album().albumTags.count("COMPOSER"));
}

/*!
 * ChapterUID is optional, and a file leaving it out has no chapters at all as far as FFmpeg is
 * concerned - it requires a nonzero UID. TagLib gives such a chapter one and reads it, so the same
 * file is an album on one build and a single track on the other. Nothing here can recover a
 * chapter the demuxer did not report.
 */
TEST(TestMatroskaTagReaderFFmpeg, ReportsNoChapterWhereTheFileGaveThemNoUid)
{
  const Fixture f("nochapteruid.mka");

  EXPECT_TRUE(f.album().chapters.empty());
}

/*!
 * Where that rule fails, and the readers part company. An edition whose chapters start after the
 * default one's is kept, so FFmpeg reports a track the file does not play and TagLib does not
 * report. Nothing in the demuxer's output says which edition a chapter came from, so this cannot
 * be filtered here - it is the cost of reading Matroska without TagLib, pinned rather than hidden.
 */
TEST(TestMatroskaTagReaderFFmpeg, KeepsAnEditionThatStartsLater)
{
  const Fixture f("lateredition.mka");

  EXPECT_EQ(4u, f.album().chapters.size());
  EXPECT_DOUBLE_EQ(10.0, f.album().chapters.back().start);
}

/*!
 * The one place the two readers genuinely part company. The Matroska spec writes one SimpleTag per
 * value, and FFmpeg keeps only the last of a repeated set (https://trac.ffmpeg.org/ticket/9641),
 * where TagLib returns all three. This is the fidelity the fallback cannot offer.
 */
TEST(TestMatroskaTagReaderFFmpeg, KeepsOnlyTheLastOfARepeatedTag)
{
  const Fixture f("repeated.mka");

  EXPECT_EQ("Gil Evans", f.album().albumTags.at("COMPOSER"));
}

/*!
 * The Segment's Duration is as optional as ChapterTimeEnd, and a file that wrote neither leaves the
 * demuxer to estimate one from the bitrate. compute_chapters_end() then closes every chapter with
 * that estimate rather than with the next chapter's start, and with the chapter's own start where
 * the estimate precedes it - so a file whose chapters run to nine seconds, read through a duration
 * estimated at one millisecond, reports three chapters of no length.
 *
 * None of them survives IsTrack(), so CAudioBookFileDirectory does not expand the file at all,
 * where TagLib expands it into its three tracks - see
 * TestAudioBookFileDirectory.KeepsTheLastTrackOfAFileThatDeclaresNoLength.
 *
 * This is pinned rather than worked around. The starts alone would say where the chapters are, but
 * a manufactured end is not marked as one: it is told apart from a declared end only by matching
 * the estimate, and a reader that second-guessed the demuxer on that would be deciding when to
 * believe it. The same file with its Duration written is read correctly (noendtimes.mka), and this
 * one is malformed enough that no muxer writes it.
 */
TEST(TestMatroskaTagReaderFFmpeg, ClampsEveryEndOfAFileThatDeclaresNoLength)
{
  const Fixture f("noduration.mka");

  ASSERT_EQ(3u, f.album().chapters.size());

  // The starts are read from the file, so they are the ones it declared.
  EXPECT_DOUBLE_EQ(0.0, f.album().chapters[0].start);
  EXPECT_DOUBLE_EQ(3.0, f.album().chapters[1].start);
  EXPECT_DOUBLE_EQ(6.0, f.album().chapters[2].start);

  // The ends are not: the first carries the estimate, the rest their own starts.
  EXPECT_GT(f.album().chapters[0].start + 1.0, f.album().chapters[0].end);
  EXPECT_DOUBLE_EQ(f.album().chapters[1].start, f.album().chapters[1].end);
  EXPECT_DOUBLE_EQ(f.album().chapters[2].start, f.album().chapters[2].end);

  // Which is what costs the file its expansion.
  for (const auto& c : f.album().chapters)
    EXPECT_FALSE(IsTrack(c.start, c.end)) << c.start;
}

//! A chapter shorter than a track is reported; dropping it is the caller's business.
TEST(TestMatroskaTagReaderFFmpeg, ReportsEveryChapterIncludingTheTiniest)
{
  const Fixture f("microchapter.mka");

  ASSERT_EQ(4u, f.album().chapters.size());
  EXPECT_DOUBLE_EQ(3.0, f.album().chapters[1].start);
  EXPECT_DOUBLE_EQ(3.4, f.album().chapters[1].end);
}

#ifdef HAS_TAGLIB_MATROSKA
/*!
 * Whatever else the two readers differ on, the shape of what they return has to match: the same
 * chapters, in the same order, with the same ranges. Which of them count as tracks is decided once
 * by the caller, which is only sound if both readers describe a file the same way.
 */
TEST(TestMatroskaTagReaderFFmpeg, AgreesWithTagLibOnTheChapterList)
{
  for (const char* name : {"chaptered.mka", "chapternames-only.mka", "precedence.mka",
                           "microchapter.mka", "twoeditions.mka", "singlefile.mka",
                           "noendtimes.mka", "singlefile-notitle.mka", "lowchapteruid.mka"})
  {
    const Fixture f(name);
    const MatroskaAlbum taglib = ReadMatroskaTags(CURL(f.path()), f.context());

    ASSERT_EQ(f.album().chapters.size(), taglib.chapters.size()) << name;
    for (size_t i = 0; i < taglib.chapters.size(); ++i)
    {
      EXPECT_DOUBLE_EQ(f.album().chapters[i].start, taglib.chapters[i].start) << name << ' ' << i;
      EXPECT_DOUBLE_EQ(f.album().chapters[i].end, taglib.chapters[i].end) << name << ' ' << i;
    }
  }
}
#endif // TagLib >= 2.3.1
