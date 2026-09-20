/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "test/TestUtils.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/pixdesc.h>
}

/*
 * The clips are generated, so the expected answers are known by construction:
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x240:r=10:d=2" -vf format=yuv420p10le \
 *          -c:v libaom-av1 -cpu-used 8 -crf 40 -pix_fmt yuv420p10le -g 10 \
 *          av1_10bit_320x240.mkv
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x240:r=10:d=2" \
 *          -c:v libx264 -preset veryslow -crf 30 -pix_fmt yuv420p -g 10 \
 *          h264_8bit_320x240.mkv
 */

namespace
{

struct DecodedPicture
{
  bool decoded{false};
  AVPixelFormat pixelFormat{AV_PIX_FMT_NONE};
  unsigned int colorBits{0};
  int depthFromDescriptor{0};
};

DecodedPicture DecodeFirstPicture(const std::string& path)
{
  DecodedPicture result;

  CFileItem item(path, false);
  auto inputStream = CDVDFactoryInputStream::CreateInputStream(nullptr, item);
  if (!inputStream || !inputStream->Open())
    return result;

  std::unique_ptr<CDVDDemux> demuxer{CDVDFactoryDemuxer::CreateDemuxer(inputStream, true)};
  if (!demuxer)
    return result;

  int videoStream = -1;
  int64_t demuxerId = -1;
  for (CDemuxStream* stream : demuxer->GetStreams())
  {
    if (stream && stream->type == StreamType::VIDEO && videoStream == -1)
    {
      videoStream = stream->uniqueId;
      demuxerId = stream->demuxerId;
    }
  }

  if (videoStream == -1)
    return result;

  auto processInfo = CProcessInfo::CreateInstance();
  std::vector<AVPixelFormat> pixFmts{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P10,
                                     AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P10,
                                     AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P10};
  processInfo->SetPixFormats(pixFmts);

  CDVDStreamInfo hint(*demuxer->GetStream(demuxerId, videoStream), true);
  hint.codecOptions = CODEC_FORCE_SOFTWARE;

  std::unique_ptr<CDVDVideoCodec> codec{CDVDFactoryCodec::CreateVideoCodec(hint, *processInfo)};
  if (!codec)
    return result;

  VideoPicture picture = {};
  int packetsTried = 0;
  if (!CDVDFileInfo::SeekAndDecodeFirstPicture(*demuxer, *codec, videoStream, 0, picture,
                                               packetsTried))
    return result;

  result.decoded = true;
  result.pixelFormat = picture.pixelFormat;
  result.colorBits = picture.colorBits;

  const AVPixFmtDescriptor* description = av_pix_fmt_desc_get(picture.pixelFormat);
  result.depthFromDescriptor = description ? description->comp[0].depth : 0;

  return result;
}

} // unnamed namespace

//! AV1 decodes through libdav1d, which allocates its own frames and so never has sw_pix_fmt set.
TEST(TestVideoCodecPictureFormat, AV1ReportsItsPixelFormatAndBitDepth)
{
  const DecodedPicture picture{DecodeFirstPicture(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/av1_10bit_320x240.mkv"))};

  ASSERT_TRUE(picture.decoded) << "the AV1 clip did not decode at all";
  EXPECT_NE(AV_PIX_FMT_NONE, picture.pixelFormat);
  EXPECT_EQ(10, picture.depthFromDescriptor);
  EXPECT_EQ(10u, picture.colorBits);
}

//! The get_format() path, which the fallback above must leave alone.
TEST(TestVideoCodecPictureFormat, H264ReportsItsPixelFormatAndBitDepth)
{
  const DecodedPicture picture{DecodeFirstPicture(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/h264_8bit_320x240.mkv"))};

  ASSERT_TRUE(picture.decoded) << "the H.264 clip did not decode at all";
  EXPECT_NE(AV_PIX_FMT_NONE, picture.pixelFormat);
  EXPECT_EQ(8, picture.depthFromDescriptor);
  EXPECT_EQ(8u, picture.colorBits);
}
