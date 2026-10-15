/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderFFmpeg.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
//! An mp4 holding a subtitle track with no samples, tagged with a title and a track. The tags are
//! written as named metadata, which keeps the track as the text given, rather than as the number
//! the iTunes track tag holds.
class TestVideoTagLoaderFFmpeg : public ::testing::Test
{
protected:
  void TearDown() override { XFILE::CFile::Delete(m_path); }

  bool WriteMp4(const char* track)
  {
    AVFormatContext* context{nullptr};
    if (avformat_alloc_output_context2(&context, nullptr, "mp4", m_path.c_str()) < 0)
      return false;

    AVStream* stream{avformat_new_stream(context, nullptr)};
    stream->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
    stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
    stream->time_base = {1, 1000};
    av_dict_set(&context->metadata, "title", "Film", 0);
    av_dict_set(&context->metadata, "track", track, 0);

    AVDictionary* options{nullptr};
    av_dict_set(&options, "movflags", "use_metadata_tags", 0);
    const bool written{avio_open(&context->pb, m_path.c_str(), AVIO_FLAG_WRITE) >= 0 &&
                       avformat_write_header(context, &options) >= 0 &&
                       av_write_trailer(context) >= 0};
    av_dict_free(&options);
    avio_closep(&context->pb);
    avformat_free_context(context);
    return written;
  }

  const std::string m_path{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderFFmpeg.mp4")};
};
} // namespace

TEST_F(TestVideoTagLoaderFFmpeg, AnMp4TrackTagIsReadAsANumber)
{
  ASSERT_TRUE(WriteMp4("3/12"));
  const CFileItem item{m_path, false};
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::TITLE, loader.Load(tag, false, nullptr));

  EXPECT_EQ("Film", tag.m_strTitle);
  EXPECT_EQ(3, tag.m_iTrack);
}

TEST_F(TestVideoTagLoaderFFmpeg, AnMp4TrackTagThatIsNotANumberIsPassedOver)
{
  ASSERT_TRUE(WriteMp4("Side A"));
  const CFileItem item{m_path, false};
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::TITLE, loader.Load(tag, false, nullptr));

  EXPECT_EQ("Film", tag.m_strTitle);
  EXPECT_EQ(-1, tag.m_iTrack);
}
