/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecWebCodecsBitstream.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// OBUs with obu_has_size_field set and a one byte size
const std::vector<uint8_t> TEMPORAL_DELIMITER{0x12, 0x00};
const std::vector<uint8_t> SEQUENCE_HEADER{0x0A, 0x01, 0x00};
// seq_profile 0, still_picture 0, reduced_still_picture_header 1
const std::vector<uint8_t> REDUCED_SEQUENCE_HEADER{0x0A, 0x01, 0x08};
// show_existing_frame 0, frame_type KEY_FRAME
const std::vector<uint8_t> KEY_FRAME{0x32, 0x01, 0x00};
// show_existing_frame 0, frame_type INTER_FRAME
const std::vector<uint8_t> INTER_FRAME{0x32, 0x01, 0x20};
// show_existing_frame 1
const std::vector<uint8_t> EXISTING_FRAME{0x32, 0x01, 0x80};

std::vector<uint8_t> Concat(std::initializer_list<std::vector<uint8_t>> obus)
{
  std::vector<uint8_t> data;
  for (const auto& obu : obus)
    data.insert(data.end(), obu.begin(), obu.end());
  return data;
}

bool IsKeyFrame(std::vector<uint8_t> data, const std::vector<uint8_t>& extradata = {})
{
  CDVDStreamInfo hints;
  hints.codec = AV_CODEC_ID_AV1;
  if (!extradata.empty())
    hints.extradata = FFmpegExtraData(extradata.data(), extradata.size());

  DemuxPacket packet;
  packet.pData = data.data();
  packet.iSize = static_cast<int>(data.size());
  return WEBCODECS::PacketIsKeyFrame(hints, packet, 0);
}
} // namespace

TEST(TestWebCodecsBitstream, AV1KeyFrame)
{
  EXPECT_TRUE(IsKeyFrame(Concat({TEMPORAL_DELIMITER, SEQUENCE_HEADER, KEY_FRAME})));
}

TEST(TestWebCodecsBitstream, AV1InterFrame)
{
  EXPECT_FALSE(IsKeyFrame(Concat({TEMPORAL_DELIMITER, SEQUENCE_HEADER, INTER_FRAME})));
}

TEST(TestWebCodecsBitstream, AV1ShowExistingFrame)
{
  EXPECT_FALSE(IsKeyFrame(Concat({TEMPORAL_DELIMITER, EXISTING_FRAME})));
}

TEST(TestWebCodecsBitstream, AV1ReducedStillPictureHeader)
{
  EXPECT_TRUE(IsKeyFrame(Concat({REDUCED_SEQUENCE_HEADER, INTER_FRAME})));
}

TEST(TestWebCodecsBitstream, AV1SequenceHeaderInConfigObus)
{
  // av1C marker and version, then three bytes of the record, then the configOBUs
  const std::vector<uint8_t> av1C{Concat({{0x81, 0x00, 0x00, 0x00}, REDUCED_SEQUENCE_HEADER})};
  EXPECT_TRUE(IsKeyFrame(INTER_FRAME, av1C));
  EXPECT_FALSE(IsKeyFrame(INTER_FRAME));
}

TEST(TestWebCodecsBitstream, AV1ObuSizePastEnd)
{
  EXPECT_FALSE(IsKeyFrame({0x32, 0x05, 0x00}));
}

TEST(TestWebCodecsBitstream, AV1Leb128TooLong)
{
  EXPECT_FALSE(IsKeyFrame({0x32, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
}

TEST(TestWebCodecsBitstream, AV1EmptyPacket)
{
  EXPECT_FALSE(IsKeyFrame({}));
}

TEST(TestWebCodecsBitstream, AV1ForbiddenBit)
{
  EXPECT_FALSE(IsKeyFrame({0xB2, 0x01, 0x00}));
}
