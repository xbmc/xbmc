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
#include <initializer_list>
#include <memory>
#include <tuple>
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
  ASSERT_TRUE(converter.Open(AV_CODEC_ID_H264, avcc.data(), avcc.size()));
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
    // FFmpeg skips empty NAL units.
    {4, {0, 0, 0, 1, 0x41, 0, 0, 0, 0}, true},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(NalPackets, TestBitstreamConverter, testing::ValuesIn(nalPackets));

namespace
{
std::vector<uint8_t> MakeAvcc(unsigned int lengthSize)
{
  return {1,    66,  0, 30,   static_cast<uint8_t>(0xfc | (lengthSize - 1)),
          0xe1, 0,   4, 0x67, 66,
          0,    30,  1, 0,    2,
          0x68, 0x80};
}

std::vector<uint8_t> MakeHvcc(unsigned int lengthSize,
                              std::initializer_list<std::vector<uint8_t>> nals)
{
  std::vector<uint8_t> extraData(23, 0);
  extraData[0] = 1;
  extraData[21] = static_cast<uint8_t>(0xfc | (lengthSize - 1));
  extraData[22] = static_cast<uint8_t>(nals.size());
  for (const auto& nal : nals)
  {
    extraData.insert(extraData.end(),
                     {static_cast<uint8_t>((nal[0] >> 1) & 0x3f), 0, 1,
                      static_cast<uint8_t>(nal.size() >> 8), static_cast<uint8_t>(nal.size())});
    extraData.insert(extraData.end(), nal.begin(), nal.end());
  }
  return extraData;
}

std::vector<uint8_t> MakePacket(unsigned int lengthSize,
                                std::initializer_list<std::vector<uint8_t>> nals)
{
  std::vector<uint8_t> packet;
  for (const auto& nal : nals)
  {
    for (unsigned int i = lengthSize; i > 0; --i)
      packet.push_back(static_cast<uint8_t>(nal.size() >> ((i - 1) * 8)));
    packet.insert(packet.end(), nal.begin(), nal.end());
  }
  return packet;
}

struct RecoveryPacketTest
{
  AVCodecID codec;
  std::vector<uint8_t> nal;
  bool recovery;
};

const RecoveryPacketTest recoveryPackets[] = {
    {AV_CODEC_ID_H264, {0x68, 0x80}, false},
    {AV_CODEC_ID_H264, {0x06, 0, 1, 0x80, 0x80}, false},
    {AV_CODEC_ID_H264, {0x06}, false},
    {AV_CODEC_ID_H264, {0x06, 0xff}, false},
    {AV_CODEC_ID_H264, {0x06, 6}, false},
    {AV_CODEC_ID_H264, {0x06, 6, 4, 0x80}, false},
    {AV_CODEC_ID_H264, {0x06, 6, 0}, false},
    {AV_CODEC_ID_H264, {0x67, 66, 0, 30}, true},
    {AV_CODEC_ID_H264, {0x65, 0x80}, true},
    {AV_CODEC_ID_H264, {0x06, 6, 1, 0x80, 0x80}, true},
    {AV_CODEC_ID_HEVC, {0x44, 1, 0x80}, false},
    {AV_CODEC_ID_HEVC, {0x42, 1, 0x80}, true},
    {AV_CODEC_ID_HEVC, {0x26, 1, 0x80}, true},
    {AV_CODEC_ID_HEVC, {0x2a, 1, 0x80}, true},
    {AV_CODEC_ID_HEVC, {0x4e, 1, 6, 1, 0x80, 0x80}, true},
};
} // namespace

class TestBitstreamConverterRecovery
  : public testing::TestWithParam<std::tuple<unsigned int, RecoveryPacketTest>>
{
};

TEST_P(TestBitstreamConverterRecovery, WaitForInputRecoveryPoint)
{
  const auto& [lengthSize, param] = GetParam();
  const bool h264 = param.codec == AV_CODEC_ID_H264;
  const auto extraData =
      h264 ? MakeAvcc(lengthSize)
           : MakeHvcc(lengthSize, {{0x40, 1, 0x80}, {0x42, 1, 0x80}, {0x44, 1, 0x80}});
  const std::vector<uint8_t> slice =
      h264 ? std::vector<uint8_t>{0x41, 0x80} : std::vector<uint8_t>{0x02, 1, 0x80};
  const std::vector<uint8_t> idr =
      h264 ? std::vector<uint8_t>{0x65, 0x80} : std::vector<uint8_t>{0x26, 1, 0x80};
  CBitstreamConverter converter;
  ASSERT_TRUE(converter.Open(param.codec, extraData.data(), static_cast<int>(extraData.size())));

  for (int seek = 0; seek < 2; ++seek)
  {
    converter.ResetStartDecode();
    auto packet = MakePacket(lengthSize, {slice});
    ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
    EXPECT_FALSE(converter.CanStartDecode());

    packet = MakePacket(lengthSize, {param.nal, slice});
    ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
    EXPECT_EQ(param.recovery, converter.CanStartDecode());

    packet = MakePacket(lengthSize, {idr});
    ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
    EXPECT_TRUE(converter.CanStartDecode());
  }
}

INSTANTIATE_TEST_SUITE_P(LengthSizes,
                         TestBitstreamConverterRecovery,
                         testing::Combine(testing::Values(1U, 2U, 3U, 4U),
                                          testing::ValuesIn(recoveryPackets)));

TEST(TestBitstreamConverter, ResetStartDecodeRestoresParameterSets)
{
  const auto nalTypes = [](const uint8_t* data, int size)
  {
    std::vector<int> types;
    for (int i = 0; i + 3 < size; ++i)
    {
      if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
        types.push_back(data[i + 3] & 0x1f);
    }
    return types;
  };

  const auto extraData = MakeAvcc(4);
  CBitstreamConverter converter;
  ASSERT_TRUE(
      converter.Open(AV_CODEC_ID_H264, extraData.data(), static_cast<int>(extraData.size())));

  // An IDR picture with in-band parameter sets, and no non-IDR slice after it
  auto packet = MakePacket(4, {{0x67, 66, 0, 30}, {0x68, 0x80}, {0x65, 0x88, 0x80}});
  ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
  EXPECT_EQ(nalTypes(converter.GetConvertBuffer(), converter.GetConvertSize()),
            (std::vector<int>{7, 8, 5}));

  // After a seek, an IDR picture without them gets the avcC's
  converter.ResetStartDecode();
  packet = MakePacket(4, {{0x65, 0x88, 0x80}});
  ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
  EXPECT_EQ(nalTypes(converter.GetConvertBuffer(), converter.GetConvertSize()),
            (std::vector<int>{7, 8, 5}));
}

TEST(TestBitstreamConverter, HevcExtradataContainsOnlyParameterSets)
{
  const std::vector<uint8_t> hdr10Plus{0x4e, 1, 4, 7, 0xb5, 0, 0x3c, 0, 1, 4, 1, 0x80};
  const auto extraData =
      MakeHvcc(4, {{0x40, 1, 0x80}, hdr10Plus, {0x42, 1, 0x80}, {0x50, 1, 0x80}, {0x44, 1, 0x80}});
  const std::vector<uint8_t> expected{0,    0, 0,    1, 0x40, 1, 0x80, 0,    0, 0,   1,
                                      0x42, 1, 0x80, 0, 0,    0, 1,    0x44, 1, 0x80};
  CBitstreamConverter converter;
  ASSERT_TRUE(
      converter.Open(AV_CODEC_ID_HEVC, extraData.data(), static_cast<int>(extraData.size())));
  converter.SetRemoveHdr10Plus(true);
  ASSERT_EQ(expected.size(), converter.GetExtraSize());
  ASSERT_NE(nullptr, converter.GetExtraData());
  EXPECT_TRUE(std::equal(expected.begin(), expected.end(), converter.GetExtraData()));

  auto packet = MakePacket(4, {{0x26, 1, 0x80}});
  ASSERT_TRUE(converter.Convert(packet.data(), static_cast<int>(packet.size())));
  const uint8_t* begin = converter.GetConvertBuffer();
  const uint8_t* end = begin + converter.GetConvertSize();
  EXPECT_EQ(end, std::search(begin, end, hdr10Plus.begin(), hdr10Plus.end()));
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

  // Cut by the end of the packet right after the start code and inside vertical_size_value, and
  // cut inside vertical_size_value by the next start code
  const std::vector<std::vector<uint8_t>> packets{
      {0x00, 0x00, 0x01, 0xB3},
      {0x00, 0x00, 0x01, 0xB3, 0x78, 0x04},
      {0x00, 0x00, 0x01, 0xB3, 0x78, 0x04, 0x00, 0x00, 0x01, 0xB8}};
  for (const auto& packet : packets)
  {
    EXPECT_FALSE(CBitstreamConverter::mpeg2_sequence_header(
        packet.data(), static_cast<uint32_t>(packet.size()), &sequence))
        << "size " << packet.size();
    EXPECT_EQ(sequence.width, 1920u);
    EXPECT_EQ(sequence.height, 1080u);
    EXPECT_EQ(sequence.ratio_info, 3u);
    EXPECT_FLOAT_EQ(sequence.ratio, 16.0f / 9);
    EXPECT_EQ(sequence.fps_rate, 25000u);
    EXPECT_EQ(sequence.fps_scale, 1000u);
  }
}

TEST(TestBitstreamConverterMpeg2, SequenceHeaderBeforeAnotherStartCode)
{
  // The 40 bits read from the header end right where a group of pictures starts
  const std::vector<uint8_t> packet{0x00, 0x00, 0x01, 0xB3, 0x78, 0x04, 0x38,
                                    0x33, 0x00, 0x00, 0x01, 0xB8, 0x00};

  mpeg2_sequence sequence{};
  EXPECT_TRUE(CBitstreamConverter::mpeg2_sequence_header(
      packet.data(), static_cast<uint32_t>(packet.size()), &sequence));
  EXPECT_EQ(sequence.width, 1920u);
  EXPECT_EQ(sequence.height, 1080u);
  EXPECT_EQ(sequence.ratio_info, 3u);
  EXPECT_EQ(sequence.fps_rate, 25000u);
  EXPECT_EQ(sequence.fps_scale, 1000u);
}
