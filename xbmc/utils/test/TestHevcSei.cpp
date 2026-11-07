/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/HevcSei.h"

#include <vector>

#include <gtest/gtest.h>

TEST(TestHevcSei, TrailingBytesAfterStopBit)
{
  // prefix SEI NAL: one 3-byte T.35 message, rbsp stop bit, 4 trailing bytes
  const std::vector<uint8_t> nal{0x4e, 0x01, 4, 3, 0x26, 0x00, 0x04, 0x80, 0x13, 0x77, 0x6b, 0x8c};

  const auto messages = CHevcSei::ParseSeiRbsp(nal.data(), nal.size());
  ASSERT_EQ(1u, messages.size());
  EXPECT_EQ(4, messages[0].m_payloadType);
  EXPECT_EQ(3u, messages[0].m_payloadSize);
}
