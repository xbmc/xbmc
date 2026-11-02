/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxCC/CaptionBlock.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxCC/H264AVCCBitstreamParser.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxCC/H264AnnexBBitstreamParser.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"

#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
using SlicePayload = std::array<uint8_t, 4>;

template<typename Parser>
CCPictureType Parse(std::vector<uint8_t>& data)
{
  DemuxPacket packet;
  packet.pData = data.data();
  packet.iSize = static_cast<int>(data.size());

  std::vector<CCaptionBlock> tempBuffer;
  std::vector<CCaptionBlock> reorderBuffer;
  Parser parser;
  return parser.ParsePacket(&packet, tempBuffer, reorderBuffer);
}

CCPictureType ParseAnnexB(uint8_t nalHeader, const SlicePayload& payload)
{
  std::vector<uint8_t> data{0x00, 0x00, 0x01, nalHeader};
  data.insert(data.end(), payload.begin(), payload.end());
  return Parse<CH264AnnexBBitstreamParser>(data);
}

CCPictureType ParseAvcc(uint8_t nalHeader, const SlicePayload& payload)
{
  std::vector<uint8_t> data{0x00, 0x00, 0x00, 0x05, nalHeader};
  data.insert(data.end(), payload.begin(), payload.end());
  return Parse<CH264AVCCBitstreamParser>(data);
}
} // namespace

// Each payload starts with first_mb_in_slice 0 (ue "1") followed by slice_type

TEST(TestCCBitstreamParser, ISlice)
{
  const SlicePayload payload{0x88, 0x80, 0x80, 0x80}; // slice_type 7
  EXPECT_EQ(ParseAnnexB(0x65, payload), CCPictureType::I_FRAME);
  EXPECT_EQ(ParseAvcc(0x65, payload), CCPictureType::I_FRAME);
}

TEST(TestCCBitstreamParser, PSlice)
{
  const SlicePayload payload{0x9A, 0x80, 0x80, 0x80}; // slice_type 5
  EXPECT_EQ(ParseAnnexB(0x41, payload), CCPictureType::P_FRAME);
  EXPECT_EQ(ParseAvcc(0x41, payload), CCPictureType::P_FRAME);
}

TEST(TestCCBitstreamParser, BSlice)
{
  const SlicePayload payload{0x9E, 0x80, 0x80, 0x80}; // slice_type 6
  EXPECT_EQ(ParseAnnexB(0x01, payload), CCPictureType::OTHER);
  EXPECT_EQ(ParseAvcc(0x01, payload), CCPictureType::OTHER);
}

TEST(TestCCBitstreamParser, MalformedSlice)
{
  const SlicePayload payload{0x00, 0x00, 0x00, 0x00}; // no Exp-Golomb stop bit
  EXPECT_EQ(ParseAnnexB(0x41, payload), CCPictureType::INVALID);
  EXPECT_EQ(ParseAvcc(0x41, payload), CCPictureType::INVALID);
}
