/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/BitstreamConverter.h"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

struct NalPacketTest
{
  unsigned int lengthSize;
  std::vector<uint8_t> packet;
  bool valid;
};

class TestBitstreamConverter : public testing::TestWithParam<NalPacketTest>
{
};

TEST_P(TestBitstreamConverter, NalLengthValidation)
{
  const auto& param = GetParam();
  std::array<uint8_t, 7> avcc{1,    100, 0, 31, static_cast<uint8_t>(0xfc | (param.lengthSize - 1)),
                              0xe0, 0};
  CBitstreamConverter converter;
  ASSERT_TRUE(converter.Open(AV_CODEC_ID_H264, avcc.data(), avcc.size(), true));
  ASSERT_TRUE(converter.NeedConvert());

  // No padding: sanitizers should catch any read beyond the supplied packet.
  auto packet = std::make_unique<uint8_t[]>(std::max<size_t>(1, param.packet.size()));
  std::copy(param.packet.begin(), param.packet.end(), packet.get());
  EXPECT_EQ(param.valid, converter.Convert(packet.get(), param.packet.size()));
  if (param.valid)
  {
    const std::vector<uint8_t> expected{0, 0, 0, 1, 0x41};
    ASSERT_EQ(expected.size(), converter.GetConvertSize());
    ASSERT_NE(nullptr, converter.GetConvertBuffer());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), converter.GetConvertBuffer()));
  }
  else
  {
    EXPECT_EQ(0, converter.GetConvertSize());
    EXPECT_EQ(nullptr, converter.GetConvertBuffer());
  }

  std::vector<uint8_t> nextPacket(param.lengthSize, 0);
  nextPacket.back() = 1;
  nextPacket.push_back(0x41);
  EXPECT_TRUE(converter.Convert(nextPacket.data(), nextPacket.size()));
  EXPECT_EQ(5, converter.GetConvertSize());
}

// clang-format off
const NalPacketTest nalPackets[] = {
    {1, {1, 0x41}, true},
    {2, {0, 1, 0x41}, true},
    {3, {0, 0, 1, 0x41}, true},
    {4, {0, 0, 0, 1, 0x41}, true},
    {4, {}, false},
    {4, {0}, false},
    {4, {0, 0}, false},
    {4, {0, 0, 0}, false},
    {4, {0, 0, 0, 0}, false},
    {4, {0, 0, 0, 1}, false},
    {4, {0, 0, 0, 2, 0x41}, false},
    {4, {0x7f, 0xff, 0xff, 0xff, 0x41}, false},
    {4, {0x80, 0, 0, 0, 0x41}, false},
    {4, {0xff, 0xff, 0xff, 0xff, 0x41}, false},
    {1, {0}, false},
    {1, {2, 0x41}, false},
    {2, {0}, false},
    {2, {0, 0}, false},
    {2, {0xff, 0xff, 0x41}, false},
    {3, {0, 0}, false},
    {3, {0xff, 0xff, 0xff, 0x41}, false},
    // Reject the entire packet even if an earlier NAL was valid.
    {4, {0, 0, 0, 1, 0x41, 0}, false},
    {4, {0, 0, 0, 1, 0x41, 0, 0, 0, 0}, false},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(NalPackets, TestBitstreamConverter, testing::ValuesIn(nalPackets));

namespace
{
// avcC and hvcC records with 4 byte NAL lengths and no parameter sets
const std::vector<uint8_t> avcC{1, 100, 0, 31, 0xff, 0xe0, 0};
const std::vector<uint8_t> hvcC{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0x03, 0};

// Non-IDR slices
const std::vector<uint8_t> h264Slice{0, 0, 0, 3, 0x41, 0x88, 0x80};
const std::vector<uint8_t> hevcTrailR{0, 0, 0, 4, 0x02, 0x01, 0xd0, 0x80};

bool StartsDecode(AVCodecID codec,
                  std::vector<uint8_t> extradata,
                  const std::vector<uint8_t>& sei,
                  const std::vector<uint8_t>& slice)
{
  CBitstreamConverter converter;
  EXPECT_TRUE(converter.Open(codec, extradata.data(), static_cast<int>(extradata.size()), true));
  converter.ResetStartDecode();

  std::vector<uint8_t> data{sei};
  data.insert(data.end(), slice.begin(), slice.end());

  // No padding: sanitizers should catch any read beyond the supplied packet.
  auto packet = std::make_unique<uint8_t[]>(data.size());
  std::copy(data.begin(), data.end(), packet.get());
  converter.Convert(packet.get(), static_cast<int>(data.size()));
  return converter.CanStartDecode();
}
} // namespace

TEST(TestBitstreamConverterSei, H264RecoveryPoint)
{
  const std::vector<uint8_t> sei{0, 0, 0, 5, 0x06, 0x06, 0x01, 0x80, 0x80};
  EXPECT_TRUE(StartsDecode(AV_CODEC_ID_H264, avcC, sei, h264Slice));
}

TEST(TestBitstreamConverterSei, H264OtherMessage)
{
  const std::vector<uint8_t> sei{0, 0, 0, 5, 0x06, 0x05, 0x01, 0xaa, 0x80};
  EXPECT_FALSE(StartsDecode(AV_CODEC_ID_H264, avcC, sei, h264Slice));
}

TEST(TestBitstreamConverterSei, H264SizeRunPastEnd)
{
  // The payload size of a recovery point runs to the end of the packet
  const std::vector<uint8_t> sei{0, 0, 0, 4, 0x06, 0x06, 0xff, 0xff};
  EXPECT_FALSE(StartsDecode(AV_CODEC_ID_H264, avcC, sei, {}));
}

TEST(TestBitstreamConverterSei, HevcRecoveryPoint)
{
  const std::vector<uint8_t> sei{0, 0, 0, 6, 0x4e, 0x01, 0x06, 0x01, 0x80, 0x80};
  EXPECT_TRUE(StartsDecode(AV_CODEC_ID_HEVC, hvcC, sei, hevcTrailR));
}

TEST(TestBitstreamConverterSei, HevcOtherMessage)
{
  const std::vector<uint8_t> sei{0, 0, 0, 6, 0x4e, 0x01, 0x89, 0x01, 0x80, 0x80};
  EXPECT_FALSE(StartsDecode(AV_CODEC_ID_HEVC, hvcC, sei, hevcTrailR));
}

TEST(TestBitstreamConverterMpeg2, SequenceHeader)
{
  // 1920x1080, aspect_ratio_information 3 (16:9), frame_rate_code 3 (25 fps)
  const std::vector<uint8_t> header{0x00, 0x00, 0x01, 0xB3, 0x78, 0x04, 0x38, 0x33};

  mpeg2_sequence sequence{};
  EXPECT_TRUE(CBitstreamConverter::mpeg2_sequence_header(header.data(), header.size(), &sequence));
  EXPECT_EQ(sequence.width, 1920u);
  EXPECT_EQ(sequence.height, 1080u);
  EXPECT_EQ(sequence.ratio_info, 3u);
  EXPECT_FLOAT_EQ(sequence.ratio, 16.0f / 9);
  EXPECT_EQ(sequence.fps_rate, 25000u);
  EXPECT_EQ(sequence.fps_scale, 1000u);

  EXPECT_FALSE(CBitstreamConverter::mpeg2_sequence_header(header.data(), header.size(), &sequence));

  // A group of pictures start code first
  std::vector<uint8_t> packet{0x00, 0x00, 0x01, 0xB8, 0x00, 0x00, 0x00};
  packet.insert(packet.end(), header.begin(), header.end());
  mpeg2_sequence afterGop{};
  EXPECT_TRUE(CBitstreamConverter::mpeg2_sequence_header(packet.data(), packet.size(), &afterGop));
  EXPECT_EQ(afterGop.width, 1920u);
  EXPECT_EQ(afterGop.height, 1080u);
}

TEST(TestBitstreamConverterMpeg2, TruncatedSequenceHeader)
{
  // 1920x1080, aspect_ratio_information 3 (16:9), frame_rate_code 3 (25 fps)
  const std::vector<uint8_t> header{0x00, 0x00, 0x01, 0xB3, 0x78, 0x04, 0x38, 0x33};

  mpeg2_sequence sequence{};
  ASSERT_TRUE(CBitstreamConverter::mpeg2_sequence_header(header.data(), header.size(), &sequence));

  // Cut right after the start code, and inside vertical_size_value
  for (const uint32_t length : {4u, 6u})
  {
    EXPECT_FALSE(CBitstreamConverter::mpeg2_sequence_header(header.data(), length, &sequence))
        << "length " << length;
    EXPECT_EQ(sequence.width, 1920u);
    EXPECT_EQ(sequence.height, 1080u);
    EXPECT_EQ(sequence.ratio_info, 3u);
    EXPECT_FLOAT_EQ(sequence.ratio, 16.0f / 9);
    EXPECT_EQ(sequence.fps_rate, 25000u);
    EXPECT_EQ(sequence.fps_scale, 1000u);
  }
}
