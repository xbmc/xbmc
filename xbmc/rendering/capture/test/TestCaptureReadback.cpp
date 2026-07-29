/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "rendering/capture/CaptureConvert.h"
#include "rendering/capture/CapturePixels.h"
#include "rendering/capture/CaptureTypes.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <gtest/gtest.h>

using namespace KODI::RENDERING::CAPTURE;

namespace
{
uint32_t Pack1010102(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  return (r & 0x3FF) | ((g & 0x3FF) << 10) | ((b & 0x3FF) << 20) | ((a & 0x3) << 30);
}

CaptureResult MakePacked(const std::vector<uint32_t>& pixels,
                         unsigned int width,
                         unsigned int height)
{
  auto buffer = std::make_unique<uint8_t[]>(pixels.size() * 4);
  std::memcpy(buffer.get(), pixels.data(), pixels.size() * 4);

  CaptureResult result;
  result.width = width;
  result.height = height;
  result.stride = static_cast<int>(width * 4);
  result.format = AV_PIX_FMT_X2BGR10LE;
  result.pixels = std::make_shared<CHeapCapturePixels>(std::move(buffer));
  result.color.primaries = AVCOL_PRI_BT709;
  result.color.transfer = AVCOL_TRC_BT709;
  result.color.range = AVCOL_RANGE_JPEG;
  return result;
}
} // namespace

TEST(TestCaptureReadback, PackedX2BGR10ChannelOrder)
{
  // GL_RGBA/UNSIGNED_INT_2_10_10_10_REV is tagged AV_PIX_FMT_X2BGR10LE, R in the
  // low field. A red then a blue pixel prove swscale reads the channels in that
  // order (a wrong tag would swap R and B on the BGRA8 output).
  const std::vector<uint32_t> src = {Pack1010102(1023, 0, 0, 3), Pack1010102(0, 0, 1023, 3)};
  CaptureResult result = MakePacked(src, 2, 1);

  std::vector<uint8_t> buffer(2 * 4, 0x55);
  ASSERT_TRUE(CaptureCopyBGRA8(result, 2, 1, buffer.data()));

  // pixel 0 is red: BGRA byte order puts near-zero in B/G and near-full in R
  EXPECT_LT(buffer[0], 4); // B
  EXPECT_LT(buffer[1], 4); // G
  EXPECT_GT(buffer[2], 250); // R

  // pixel 1 is blue: full in B, near-zero in G/R
  EXPECT_GT(buffer[4], 250); // B
  EXPECT_LT(buffer[5], 4); // G
  EXPECT_LT(buffer[6], 4); // R
}
