/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "rendering/capture/CaptureReadback.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RENDERING::CAPTURE;

namespace
{
uint32_t Pack1010102(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  return (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20) | ((a & 0x3) << 30);
}
} // namespace

TEST(TestCaptureReadback, Unpack1010102Endpoints)
{
  // full scale maps to full scale, zero to zero, alpha always opaque
  const std::vector<uint32_t> src = {Pack1010102(0, 0, 0, 0), Pack1010102(1023, 1023, 1023, 3)};
  std::vector<uint16_t> dst(src.size() * 4, 0x1234);
  Unpack1010102ToRGBA16(src.data(), dst.data(), static_cast<unsigned int>(src.size()));

  EXPECT_EQ(dst[0], 0);
  EXPECT_EQ(dst[1], 0);
  EXPECT_EQ(dst[2], 0);
  EXPECT_EQ(dst[3], 0xFFFF); // alpha forced opaque, ignoring the 2-bit source

  EXPECT_EQ(dst[4], 0xFFFF);
  EXPECT_EQ(dst[5], 0xFFFF);
  EXPECT_EQ(dst[6], 0xFFFF);
  EXPECT_EQ(dst[7], 0xFFFF);
}

TEST(TestCaptureReadback, Unpack1010102ProjectionNotPlainShift)
{
  // the projection must differ from a plain <<6: 512 maps to
  // round(512 * 65535 / 1023) = 32800, whereas 512 << 6 would be 32768.
  // Per-channel fields are R, G, B in ascending 10-bit slots.
  const std::vector<uint32_t> src = {Pack1010102(512, 512, 512, 3)};
  std::vector<uint16_t> dst(4, 0);
  Unpack1010102ToRGBA16(src.data(), dst.data(), 1);

  EXPECT_GT(dst[0], 512u << 6); // strictly above the plain-shift value
  EXPECT_EQ(dst[0], 32800);
}

TEST(TestCaptureReadback, Unpack1010102ToBGRA8EndpointsAndOrder)
{
  // 10-bit decimated to 8-bit, written B, G, R, A (the capture contract), with
  // R the least significant source field. Distinct channels prove the order.
  const std::vector<uint32_t> src = {Pack1010102(1023, 0, 0, 0), // R full, G/B zero
                                     Pack1010102(0, 0, 1023, 3)}; // B full, R/G zero
  std::vector<uint8_t> dst(src.size() * 4, 0x55);
  Unpack1010102ToBGRA8(src.data(), dst.data(), static_cast<unsigned int>(src.size()));

  // pixel 0: R=1023 lands in byte 2, B and G zero, alpha opaque
  EXPECT_EQ(dst[0], 0); // B
  EXPECT_EQ(dst[1], 0); // G
  EXPECT_EQ(dst[2], 255); // R
  EXPECT_EQ(dst[3], 0xFF); // A

  // pixel 1: B=1023 lands in byte 0
  EXPECT_EQ(dst[4], 255); // B
  EXPECT_EQ(dst[5], 0); // G
  EXPECT_EQ(dst[6], 0); // R
  EXPECT_EQ(dst[7], 0xFF); // A
}
