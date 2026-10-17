/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/DolbyVisionRpu.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::DOLBYVISION;

TEST(TestDolbyVisionRpu, HevcRpuToNalAddsHeaderAndRemovesEmulationPrevention)
{
  const std::vector<uint8_t> rpu = {0x19, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x03, 0x00, 0x80};
  const std::vector<uint8_t> expected = {0x7C, 0x01, 0x19, 0x00, 0x00,
                                         0x01, 0x00, 0x00, 0x00, 0x80};
  EXPECT_EQ(expected, HevcRpuToNal(rpu.data(), rpu.size()));
}

TEST(TestDolbyVisionRpu, ExtractAv1RpuIgnoresOtherMetadata)
{
  // temporal delimiter, then an ITU-T T.35 metadata OBU from another provider
  const std::vector<uint8_t> temporalUnit = {0x12, 0x00, 0x2A, 0x09, 0x04, 0xB5, 0x00,
                                             0x3C, 0x00, 0x01, 0x04, 0x00, 0x80};
  EXPECT_TRUE(ExtractAv1Rpu(temporalUnit.data(), temporalUnit.size()).empty());
}

TEST(TestDolbyVisionRpu, ExtractAv1RpuRejectsTruncatedObu)
{
  const std::vector<uint8_t> temporalUnit = {0x2A, 0x20, 0x04, 0xB5};
  EXPECT_TRUE(ExtractAv1Rpu(temporalUnit.data(), temporalUnit.size()).empty());
}
