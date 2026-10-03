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
