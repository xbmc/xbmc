/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "rendering/capture/CaptureConvert.h"
#include "rendering/capture/CaptureTypes.h"

#include <cmath>
#include <cstring>
#include <vector>

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <gtest/gtest.h>

using namespace KODI::RENDERING::CAPTURE;

namespace
{

// SMPTE ST 2084 inverse EOTF: absolute luminance in nits to a PQ code value.
uint16_t PQCode(double nits)
{
  constexpr double m1 = 2610.0 / 16384.0;
  constexpr double m2 = 2523.0 / 4096.0 * 128.0;
  constexpr double c1 = 3424.0 / 4096.0;
  constexpr double c2 = 2413.0 / 4096.0 * 32.0;
  constexpr double c3 = 2392.0 / 4096.0 * 32.0;
  const double y = std::pow(nits / 10000.0, m1);
  const double code = std::pow((c1 + c2 * y) / (1.0 + c3 * y), m2);
  return static_cast<uint16_t>(code * 65535.0 + 0.5);
}

CaptureResult MakeBGRA8(unsigned int width, unsigned int height)
{
  CaptureResult result;
  result.width = width;
  result.height = height;
  result.stride = width * 4;
  result.bitDepth = 8;
  result.pixels = std::shared_ptr<uint8_t[]>(new uint8_t[result.stride * height]);
  result.color.primaries = AVCOL_PRI_BT709;
  result.color.transfer = AVCOL_TRC_BT709;
  result.color.range = AVCOL_RANGE_JPEG;
  return result;
}

} // namespace

TEST(TestCaptureConvert, SDRSameSizeByteExact)
{
  constexpr unsigned int width = 4;
  constexpr unsigned int height = 2;
  CaptureResult result = MakeBGRA8(width, height);
  for (unsigned int i = 0; i < width * height * 4; i++)
    result.pixels[i] = static_cast<uint8_t>(i * 7 + 3);

  std::vector<uint8_t> buffer(width * height * 4, 0);
  ASSERT_TRUE(CaptureToBGRA(result, width, height, buffer.data()));
  EXPECT_EQ(std::memcmp(buffer.data(), result.pixels.get(), buffer.size()), 0);
}

TEST(TestCaptureConvert, SDRStrideAwareByteExact)
{
  constexpr unsigned int width = 3;
  constexpr unsigned int height = 2;
  constexpr unsigned int stride = 16; // 4 padding bytes per row
  CaptureResult result = MakeBGRA8(width, height);
  result.stride = stride;
  result.pixels = std::shared_ptr<uint8_t[]>(new uint8_t[stride * height]());
  for (unsigned int y = 0; y < height; y++)
    for (unsigned int i = 0; i < width * 4; i++)
      result.pixels[y * stride + i] = static_cast<uint8_t>(y * 100 + i);

  std::vector<uint8_t> buffer(width * height * 4, 0xAA);
  ASSERT_TRUE(CaptureToBGRA(result, width, height, buffer.data()));
  for (unsigned int y = 0; y < height; y++)
    for (unsigned int i = 0; i < width * 4; i++)
      EXPECT_EQ(buffer[y * width * 4 + i], result.pixels[y * stride + i]);
}

TEST(TestCaptureConvert, CopyBGRA8IgnoresHDRTagsNoTonemap)
{
  // the Python RenderCapture contract: an 8-bit BGRA capture is delivered
  // verbatim even when tagged HDR, NEVER tonemapped. Byte-exact proves
  // CaptureCopyBGRA8 does not route PQ/HLG-tagged input through the
  // color-managed path the way CaptureToBGRA does.
  constexpr unsigned int width = 4;
  constexpr unsigned int height = 2;
  CaptureResult result = MakeBGRA8(width, height);
  result.color.primaries = AVCOL_PRI_BT2020;
  result.color.transfer = AVCOL_TRC_SMPTE2084; // PQ tag on an 8-bit buffer
  for (unsigned int i = 0; i < width * height * 4; i++)
    result.pixels[i] = static_cast<uint8_t>(i * 11 + 5);

  std::vector<uint8_t> buffer(width * height * 4, 0);
  ASSERT_TRUE(CaptureCopyBGRA8(result, width, height, buffer.data()));
  EXPECT_EQ(std::memcmp(buffer.data(), result.pixels.get(), buffer.size()), 0);
}

TEST(TestCaptureConvert, PQRampHonorsMasteringPeak)
{
  // one row per luminance step, gray PQ-coded RGBA64 tagged BT.2020 + PQ
  const std::vector<double> nits = {0.0, 1.0, 10.0, 50.0, 100.0, 203.0, 400.0, 1000.0, 10000.0};
  const unsigned int width = 4;
  const unsigned int height = nits.size();

  CaptureResult result;
  result.width = width;
  result.height = height;
  result.stride = width * 8;
  result.bitDepth = 10;
  result.pixels = std::shared_ptr<uint8_t[]>(new uint8_t[result.stride * height]);
  result.color.primaries = AVCOL_PRI_BT2020;
  result.color.transfer = AVCOL_TRC_SMPTE2084;
  result.color.range = AVCOL_RANGE_JPEG;
  result.hasDisplayMetadata = true;

  for (unsigned int y = 0; y < height; y++)
  {
    const uint16_t code = PQCode(nits[y]);
    uint16_t* row = reinterpret_cast<uint16_t*>(result.pixels.get() + y * result.stride);
    for (unsigned int x = 0; x < width; x++)
    {
      row[x * 4 + 0] = code;
      row[x * 4 + 1] = code;
      row[x * 4 + 2] = code;
      row[x * 4 + 3] = 0xFFFF;
    }
  }

  // convert the same ramp under two mastering peaks; a lower peak must map the
  // shared 203-nit row brighter, proving swscale honors the metadata we feed
  auto convertAtPeak = [&](int peakNits)
  {
    result.displayMetadata = {};
    result.displayMetadata.has_luminance = 1;
    result.displayMetadata.max_luminance = AVRational{peakNits, 1};
    std::vector<uint8_t> buffer(width * height * 4, 0);
    EXPECT_TRUE(CaptureToBGRA(result, width, height, buffer.data()));
    return buffer;
  };

  const std::vector<uint8_t> lowPeak = convertAtPeak(203);
  const std::vector<uint8_t> highPeak = convertAtPeak(1000);

  auto gray = [&](const std::vector<uint8_t>& buf, unsigned int y)
  {
    const uint8_t* px = buf.data() + y * width * 4;
    return (px[0] + px[1] + px[2]) / 3;
  };

  // row 5 is 203 nits: peak white at a 203-nit master, well below at 1000. equal
  // outputs would mean the metadata was ignored, the regression this guards
  EXPECT_GT(gray(lowPeak, 5), gray(highPeak, 5));

  // sanity on the 1000-nit pass: gray stays gray, ramp monotone and not clipped
  std::vector<int> ramp(height);
  for (unsigned int y = 0; y < height; y++)
  {
    const uint8_t* px = highPeak.data() + y * width * 4;
    EXPECT_NEAR(px[0], px[1], 8) << "row " << y;
    EXPECT_NEAR(px[1], px[2], 8) << "row " << y;
    ramp[y] = (px[0] + px[1] + px[2]) / 3;
  }
  for (unsigned int y = 1; y < height; y++)
    EXPECT_GE(ramp[y] + 2, ramp[y - 1]) << "row " << y;
  EXPECT_LE(ramp[0], 10);
  EXPECT_GE(ramp[height - 1], 200);
  EXPECT_GT(ramp[7], ramp[4]);
}
