/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"

#include <cstddef>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/pixdesc.h>
}

namespace
{
constexpr int WIDTH = 32;
constexpr int HEIGHT = 16;

struct Layout
{
  AVPixelFormat format;
  int strides[YuvImage::MAX_PLANES];
  // where the second and third planes start, counted from the first; -1 where there is none
  std::ptrdiff_t second;
  std::ptrdiff_t third;
};

constexpr Layout LAYOUTS[] = {
    {AV_PIX_FMT_YUV420P, {32, 16, 16}, 32 * 16, 32 * 16 + 16 * 8},
    {AV_PIX_FMT_YUV420P10, {64, 32, 32}, 64 * 16, 64 * 16 + 32 * 8},
    {AV_PIX_FMT_NV12, {32, 32, 0}, 32 * 16, -1},
    {AV_PIX_FMT_YUV422P, {32, 16, 16}, 32 * 16, 32 * 16 + 16 * 16},
    {AV_PIX_FMT_YUV444P, {32, 32, 32}, 32 * 16, 2 * 32 * 16},
    {AV_PIX_FMT_YUV444P12, {64, 64, 64}, 64 * 16, 2 * 64 * 16},
    {AV_PIX_FMT_GBRP, {32, 32, 32}, 32 * 16, 2 * 32 * 16},
    {AV_PIX_FMT_GBRP10, {64, 64, 64}, 64 * 16, 2 * 64 * 16},
};
} // namespace

TEST(TestVideoBufferSysMem, PlacesThePlanesOfEachLayoutOneAfterAnother)
{
  for (const Layout& layout : LAYOUTS)
  {
    SCOPED_TRACE(av_get_pix_fmt_name(layout.format));

    CVideoBufferManager manager;
    CVideoBuffer* buffer = manager.Get(layout.format, 3 * 64 * HEIGHT, nullptr);
    ASSERT_NE(buffer, nullptr);

    buffer->SetDimensions(WIDTH, HEIGHT, layout.strides);

    uint8_t* planes[YuvImage::MAX_PLANES];
    buffer->GetPlanes(planes);

    EXPECT_EQ(planes[1] - planes[0], layout.second);
    if (layout.third >= 0)
      EXPECT_EQ(planes[2] - planes[0], layout.third);

    buffer->Release();
  }
}
