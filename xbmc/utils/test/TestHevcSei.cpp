/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/HevcSei.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// Prefix SEI NAL unit header (nal_unit_type 39)
constexpr uint8_t SEI_HEADER_0 = 0x4E;
constexpr uint8_t SEI_HEADER_1 = 0x01;
} // namespace

TEST(TestHevcSei, TwoMessages)
{
  const std::vector<uint8_t> nal{SEI_HEADER_0, SEI_HEADER_1, 0x05, 0x03, 0xAA, 0xBB,
                                 0xCC,         0x04,         0x02, 0x11, 0x22, 0x80};

  const auto messages{CHevcSei::ParseSeiRbsp(nal.data(), nal.size())};

  ASSERT_EQ(messages.size(), 2u);
  EXPECT_EQ(messages[0].m_payloadType, 5);
  EXPECT_EQ(messages[0].m_payloadSize, 3u);
  EXPECT_EQ(messages[0].m_msgOffset, 2u);
  EXPECT_EQ(messages[0].m_payloadOffset, 4u);
  EXPECT_EQ(messages[1].m_payloadType, 4);
  EXPECT_EQ(messages[1].m_payloadSize, 2u);
  EXPECT_EQ(messages[1].m_msgOffset, 7u);
  EXPECT_EQ(messages[1].m_payloadOffset, 9u);
}

TEST(TestHevcSei, PayloadPastEnd)
{
  // The second message declares 32 bytes but only 10 follow
  const std::vector<uint8_t> nal{SEI_HEADER_0, SEI_HEADER_1, 0x05, 0x01, 0xAA, 0x04,
                                 0x20,         0x01,         0x02, 0x03, 0x04, 0x05,
                                 0x06,         0x07,         0x08, 0x09, 0x0A};

  const auto messages{CHevcSei::ParseSeiRbsp(nal.data(), nal.size())};

  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0].m_payloadType, 5);
  EXPECT_EQ(messages[0].m_payloadSize, 1u);
}

TEST(TestHevcSei, SizeRunPastEnd)
{
  // The second message's payload type runs off the end of the NAL
  const std::vector<uint8_t> nal{SEI_HEADER_0, SEI_HEADER_1, 0x05, 0x01, 0xAA, 0xFF, 0xFF};

  const auto messages{CHevcSei::ParseSeiRbsp(nal.data(), nal.size())};

  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0].m_payloadType, 5);
}

TEST(TestHevcSei, RemoveHdr10Plus)
{
  // An HDR10+ message (United States, Samsung, ST 2094-40, application 4), then a message whose
  // RBSP holds 00 00 02 and so carries an emulation prevention byte
  const std::vector<uint8_t> nal{SEI_HEADER_0, SEI_HEADER_1, 0x04, 0x07, 0xB5, 0x00, 0x3C,
                                 0x00,         0x01,         0x04, 0x01, 0x05, 0x04, 0x00,
                                 0x00,         0x03,         0x02, 0x11, 0x80};

  const auto [containsHdr10Plus, result] =
      CHevcSei::RemoveHdr10PlusFromSeiNalu(nal.data(), nal.size());

  EXPECT_TRUE(containsHdr10Plus);
  const std::vector<uint8_t> expected{SEI_HEADER_0, SEI_HEADER_1, 0x05, 0x04, 0x00,
                                      0x00,         0x03,         0x02, 0x11, 0x80};
  EXPECT_EQ(result, expected);
}

TEST(TestHevcSei, PayloadSizeMultiplicationOverflow)
{
  // An HDR10+ message declaring 0x20000007 bytes, which is 56 bits when converted in 32 bits,
  // with only 11 bytes left, the last 4 of them a second message
  std::vector<uint8_t> nal{SEI_HEADER_0, SEI_HEADER_1, 0x04};
  nal.insert(nal.end(), 2105376, 0xFF);
  nal.push_back(0x27);
  nal.insert(nal.end(), {0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x05, 0x01, 0xAA, 0x80});

  // Fatal, so that the erase in RemoveHdr10PlusFromSeiNalu is not reached if this fails
  ASSERT_TRUE(CHevcSei::ParseSeiRbsp(nal.data(), nal.size()).empty());

  const auto [containsHdr10Plus, result] =
      CHevcSei::RemoveHdr10PlusFromSeiNalu(nal.data(), nal.size());
  EXPECT_FALSE(containsHdr10Plus);
  EXPECT_TRUE(result.empty());
}
