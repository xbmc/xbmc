/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodecPassthrough.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/crc.h>
}

namespace
{
constexpr unsigned int TRUEHD_RAW_BUFFER_CAPACITY = 61440;
constexpr unsigned int TRUEHD_RAW_FRAMES_PER_PACKET = 24;

class CRawPassthroughProcessInfo : public CProcessInfo
{
public:
  bool WantsRawPassthrough() override { return true; }
};

std::vector<uint8_t> MakeTrueHDMajorSyncFrame(unsigned int frameSize, uint8_t fillByte)
{
  std::vector<uint8_t> frame(frameSize, fillByte);
  const uint16_t words = static_cast<uint16_t>(frameSize >> 1);
  frame[0] = static_cast<uint8_t>(0xF0 | ((words >> 8) & 0x0F));
  frame[1] = static_cast<uint8_t>(words & 0xFF);
  frame[2] = 0x00;
  frame[3] = 0x00;
  frame[4] = 0xF8; // format_sync
  frame[5] = 0x72;
  frame[6] = 0x6F;
  frame[7] = 0xBA;
  frame[8] = 0x00; // 48 kHz
  frame[9] = 0x00;
  frame[10] = 0x00;
  frame[11] = 0x03; // 2 channels
  frame[20] = 0x10; // 1 substream
  frame[29] = 0x00; // no extensions -> major_sync_size = 28

  AVCRC crcTable[257];
  av_crc_init(crcTable, 0, 16, 0x2D, sizeof(crcTable));
  uint16_t crc = av_crc(crcTable, 0, frame.data() + 4, 24);
  crc ^= (static_cast<uint16_t>(frame[29]) << 8) | frame[28];
  frame[30] = static_cast<uint8_t>(crc & 0xFF);
  frame[31] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  return frame;
}
} // namespace

TEST(TestDVDAudioCodecPassthrough, PackTrueHDRawNormal24Frames)
{
  CRawPassthroughProcessInfo processInfo;
  CDVDAudioCodecPassthrough codec(processInfo, CAEStreamInfo::STREAM_TYPE_TRUEHD);
  CDVDStreamInfo hints;
  CDVDCodecOptions options;
  ASSERT_TRUE(codec.Open(hints, options));

  constexpr unsigned int frameSize = 1024;
  for (unsigned int i = 0; i < TRUEHD_RAW_FRAMES_PER_PACKET; ++i)
  {
    auto frameBytes = MakeTrueHDMajorSyncFrame(frameSize, static_cast<uint8_t>(0x10 + i));
    DemuxPacket pkt;
    pkt.pData = frameBytes.data();
    pkt.iSize = static_cast<int>(frameBytes.size());
    ASSERT_TRUE(codec.AddData(pkt));

    DVDAudioFrame out{};
    codec.GetData(out);
    if (i + 1 < TRUEHD_RAW_FRAMES_PER_PACKET)
    {
      EXPECT_EQ(out.nb_frames, 0u);
    }
    else
    {
      EXPECT_EQ(out.nb_frames, TRUEHD_RAW_FRAMES_PER_PACKET * frameSize);
      EXPECT_TRUE(out.passthrough);
      EXPECT_EQ(out.format.m_streamInfo.m_type, CAEStreamInfo::STREAM_TYPE_TRUEHD);
      ASSERT_NE(out.data[0], nullptr);
      EXPECT_EQ(out.data[0][32], 0x10);
      EXPECT_EQ(out.data[0][(TRUEHD_RAW_FRAMES_PER_PACKET - 1) * frameSize + 32], 0x10 + 23);
    }
  }
}

TEST(TestDVDAudioCodecPassthrough, PackTrueHDRawExactCapacityBoundary)
{
  CRawPassthroughProcessInfo processInfo;
  CDVDAudioCodecPassthrough codec(processInfo, CAEStreamInfo::STREAM_TYPE_TRUEHD);
  CDVDStreamInfo hints;
  CDVDCodecOptions options;
  ASSERT_TRUE(codec.Open(hints, options));

  constexpr unsigned int frameSize = TRUEHD_RAW_BUFFER_CAPACITY / TRUEHD_RAW_FRAMES_PER_PACKET;
  for (unsigned int i = 0; i < TRUEHD_RAW_FRAMES_PER_PACKET; ++i)
  {
    auto frameBytes = MakeTrueHDMajorSyncFrame(frameSize, static_cast<uint8_t>(0x40 + i));
    DemuxPacket pkt;
    pkt.pData = frameBytes.data();
    pkt.iSize = static_cast<int>(frameBytes.size());
    ASSERT_TRUE(codec.AddData(pkt));

    DVDAudioFrame out{};
    codec.GetData(out);
    if (i + 1 < TRUEHD_RAW_FRAMES_PER_PACKET)
    {
      EXPECT_EQ(out.nb_frames, 0u);
    }
    else
    {
      EXPECT_EQ(out.nb_frames, TRUEHD_RAW_BUFFER_CAPACITY);
      ASSERT_NE(out.data[0], nullptr);
      EXPECT_EQ(out.data[0][32], 0x40);
      EXPECT_EQ(out.data[0][TRUEHD_RAW_BUFFER_CAPACITY - 1], 0x40 + 23);
    }
  }
}

TEST(TestDVDAudioCodecPassthrough, PackTrueHDRawOversizedGroupDiscardedAndRecovers)
{
  CRawPassthroughProcessInfo processInfo;
  CDVDAudioCodecPassthrough codec(processInfo, CAEStreamInfo::STREAM_TYPE_TRUEHD);
  CDVDStreamInfo hints;
  CDVDCodecOptions options;
  ASSERT_TRUE(codec.Open(hints, options));

  // 20 * 3072 == 61440 fills the buffer; the 21st frame would overflow by 3072 bytes.
  constexpr double discardedBasePts = 1000000.0;
  constexpr double frameStepPts = 833.0;
  auto oversizedFrame = MakeTrueHDMajorSyncFrame(3072, 0x5A);
  for (unsigned int i = 0; i < 21; ++i)
  {
    DemuxPacket pkt;
    pkt.pData = oversizedFrame.data();
    pkt.iSize = static_cast<int>(oversizedFrame.size());
    pkt.pts = discardedBasePts + i * frameStepPts;
    ASSERT_TRUE(codec.AddData(pkt));

    DVDAudioFrame out{};
    codec.GetData(out);
    EXPECT_EQ(out.nb_frames, 0u);
  }

  // Subsequent normal 24-frame sequence recovers cleanly.
  constexpr double recoveredBasePts = 5000000.0;
  constexpr unsigned int normalFrameSize = 1024;
  auto normalFrame = MakeTrueHDMajorSyncFrame(normalFrameSize, 0x7E);
  for (unsigned int i = 0; i < TRUEHD_RAW_FRAMES_PER_PACKET; ++i)
  {
    DemuxPacket pkt;
    pkt.pData = normalFrame.data();
    pkt.iSize = static_cast<int>(normalFrame.size());
    pkt.pts = recoveredBasePts + i * frameStepPts;
    ASSERT_TRUE(codec.AddData(pkt));

    DVDAudioFrame out{};
    codec.GetData(out);
    if (i + 1 < TRUEHD_RAW_FRAMES_PER_PACKET)
    {
      EXPECT_EQ(out.nb_frames, 0u);
    }
    else
    {
      EXPECT_EQ(out.nb_frames, TRUEHD_RAW_FRAMES_PER_PACKET * normalFrameSize);
      EXPECT_DOUBLE_EQ(out.pts, recoveredBasePts);
      ASSERT_NE(out.data[0], nullptr);
      EXPECT_EQ(out.data[0][32], 0x7E);
    }
  }
}
