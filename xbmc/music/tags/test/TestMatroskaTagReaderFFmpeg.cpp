/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/File.h"
#include "music/tags/MatroskaTagReader.h"
#include "music/tags/TagLibVersion.h"
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/mem.h>
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
constexpr size_t BUFFER_SIZE = 32768;

int VfsRead(void* h, uint8_t* buf, int size)
{
  return static_cast<XFILE::CFile*>(h)->Read(buf, size);
}

int64_t VfsSeek(void* h, int64_t pos, int whence)
{
  auto* file = static_cast<XFILE::CFile*>(h);
  return whence == AVSEEK_SIZE ? file->GetLength() : file->Seek(pos, whence & ~AVSEEK_FORCE);
}

//! Opens a fixture and reads it with the FFmpeg reader.
class Fixture
{
public:
  explicit Fixture(const std::string& name)
  {
    m_path = XBMC_REF_FILE_PATH("xbmc/filesystem/test/data/audiobook/" + name);
    const std::string& path = m_path;
    if (!m_file.Open(CURL(path)))
      return;

    auto* buffer = static_cast<uint8_t*>(av_malloc(BUFFER_SIZE));
    m_ioctx = avio_alloc_context(buffer, BUFFER_SIZE, 0, &m_file, VfsRead, nullptr, VfsSeek);
    m_fctx = avformat_alloc_context();
    m_fctx->pb = m_ioctx;
    m_fctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    const AVInputFormat* iformat = nullptr;
    av_probe_input_buffer(m_ioctx, &iformat, path.c_str(), nullptr, 0, 0);
    if (avformat_open_input(&m_fctx, path.c_str(), iformat, nullptr) < 0)
    {
      m_fctx = nullptr;
      return;
    }
    avformat_find_stream_info(m_fctx, nullptr);
    m_album = ReadMatroskaTagsWithFFmpeg(CURL(path), m_fctx);
  }

  ~Fixture()
  {
    if (m_fctx)
      avformat_close_input(&m_fctx);
    if (m_ioctx)
    {
      av_free(m_ioctx->buffer);
      av_free(m_ioctx);
    }
  }

  const MatroskaAlbum& album() const { return m_album; }
  const AVFormatContext* context() const { return m_fctx; }
  const std::string& path() const { return m_path; }

private:
  std::string m_path;
  XFILE::CFile m_file;
  AVIOContext* m_ioctx = nullptr;
  AVFormatContext* m_fctx = nullptr;
  MatroskaAlbum m_album;
};
} // unnamed namespace

TEST(TestMatroskaTagReaderFFmpeg, ReadsAlbumTagsUnderTheirAlbumPrefix)
{
  const Fixture f("singlefile.mka");
  const auto& tags = f.album().fileTags;

  ASSERT_TRUE(f.album().hasAlbumTags());
  EXPECT_EQ("Live At The Test Venue", tags.at("ALBUM/TITLE"));
  EXPECT_EQ("The Test Band", tags.at("ALBUM/ARTIST"));
  EXPECT_EQ("2026", tags.at("ALBUM/DATE_RELEASED"));
  EXPECT_EQ("Bill Evans", tags.at("ALBUM/COMPOSER"));
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
 * Only the default edition's chapters are reported, and the tags of the other edition are not
 * reported at all - so the reader has nothing to filter, unlike the TagLib one.
 */
TEST(TestMatroskaTagReaderFFmpeg, ReportsOnlyTheDefaultEdition)
{
  const Fixture f("twoeditions.mka");

  ASSERT_EQ(3u, f.album().chapters.size());
  for (const auto& chapter : f.album().chapters)
    EXPECT_EQ(0u, chapter.tags.count("COMPOSER"));
  EXPECT_EQ(0u, f.album().fileTags.count("ALBUM/COMPOSER"));
}

/*!
 * The one place the two readers genuinely part company. The Matroska spec writes one SimpleTag per
 * value, and FFmpeg keeps only the last of a repeated set (https://trac.ffmpeg.org/ticket/9641),
 * where TagLib returns all three. This is the fidelity the fallback cannot offer.
 */
TEST(TestMatroskaTagReaderFFmpeg, KeepsOnlyTheLastOfARepeatedTag)
{
  const Fixture f("repeated.mka");

  EXPECT_EQ("Gil Evans", f.album().fileTags.at("ALBUM/COMPOSER"));
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
  for (const char* name :
       {"chaptered.mka", "chapternames-only.mka", "precedence.mka", "microchapter.mka",
        "twoeditions.mka", "singlefile.mka", "noendtimes.mka"})
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
