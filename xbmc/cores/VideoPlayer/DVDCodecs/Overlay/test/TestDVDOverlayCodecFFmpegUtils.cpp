/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayCodecFFmpegUtils.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::SUBTITLES;

namespace
{
class TestBitmap
{
public:
  TestBitmap(int sourceWidth, int sourceHeight, int x, int y, int width, int height)
    : m_pixels(width * height),
      m_bitmap{m_pixels.data(), width, x, y, width, height, sourceWidth, sourceHeight}
  {
    m_bitmap.palette[1] = 0xffffffff;
    m_bitmap.palette[2] = 0xff00ff00;
  }

  void SetPixel(int x, int y, uint8_t color)
  {
    m_pixels[(y - m_bitmap.y) * m_bitmap.stride + x - m_bitmap.x] = color;
  }

  void SetPattern(int x, int y)
  {
    SetPixel(x, y, 1);
    SetPixel(x + 1, y, 2);
    SetPixel(x, y + 1, 2);
    SetPixel(x + 1, y + 1, 1);
  }

  const BitmapSubtitle& Get() const { return m_bitmap; }

private:
  std::vector<uint8_t> m_pixels;
  BitmapSubtitle m_bitmap;
};

void FillPairWithMismatches(TestBitmap& bitmap, int mismatches)
{
  for (int y = 1; y < 11; ++y)
  {
    for (int x = 0; x < 20; ++x)
    {
      bitmap.SetPixel(2 + x, y, 1);
      bitmap.SetPixel(37 + x, y, 1);
    }
  }

  for (int i = 0; i < mismatches; ++i)
    bitmap.SetPixel(37 + (i % 20), 1 + (i / 20), 2);
}
} // unnamed namespace

TEST(TestDVDOverlayCodecFFmpegUtils, DetectsShiftedSideBySideImages)
{
  TestBitmap bitmap(16, 6, 1, 1, 12, 2);
  bitmap.SetPattern(1, 1);
  bitmap.SetPattern(11, 1);

  EXPECT_TRUE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::TOP_BOTTOM));
}

TEST(TestDVDOverlayCodecFFmpegUtils, DetectsShiftedTopBottomImages)
{
  TestBitmap bitmap(8, 12, 2, 1, 4, 8);
  bitmap.SetPattern(2, 1);
  bitmap.SetPattern(4, 7);

  EXPECT_TRUE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::TOP_BOTTOM));
  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, RejectsWideMonoscopicCaption)
{
  TestBitmap bitmap(16, 6, 2, 1, 12, 2);
  bitmap.SetPattern(5, 1);
  bitmap.SetPattern(9, 1);
  bitmap.SetPixel(9, 1, 2);

  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, RejectsCenteredRepeatedMonoscopicCaption)
{
  TestBitmap bitmap(16, 6, 2, 1, 12, 2);
  bitmap.SetPattern(5, 1);
  bitmap.SetPattern(9, 1);

  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, RejectsCaptionInsideOneView)
{
  TestBitmap bitmap(16, 6, 1, 1, 4, 2);
  bitmap.SetPattern(1, 1);
  bitmap.SetPattern(3, 1);

  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, AcceptsPairAtTolerance)
{
  TestBitmap bitmap(64, 12, 2, 1, 60, 10);
  FillPairWithMismatches(bitmap, 2);

  EXPECT_TRUE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, RejectsPairOutsideTolerance)
{
  TestBitmap bitmap(64, 12, 2, 1, 60, 10);
  FillPairWithMismatches(bitmap, 3);

  EXPECT_FALSE(IsStereoBitmap(bitmap.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, UsesRealisticSideBySidePositionTolerance)
{
  TestBitmap accepted(1920, 1080, 400, 100, 1062, 2);
  accepted.SetPattern(400, 100);
  accepted.SetPattern(1460, 100);

  TestBitmap rejected(1920, 1080, 400, 100, 1162, 2);
  rejected.SetPattern(400, 100);
  rejected.SetPattern(1560, 100);

  EXPECT_TRUE(IsStereoBitmap(accepted.Get(), BitmapStereoLayout::LEFT_RIGHT));
  EXPECT_FALSE(IsStereoBitmap(rejected.Get(), BitmapStereoLayout::LEFT_RIGHT));
}

TEST(TestDVDOverlayCodecFFmpegUtils, UsesRealisticTopBottomPositionTolerance)
{
  TestBitmap accepted(1920, 1080, 400, 200, 202, 546);
  accepted.SetPattern(400, 200);
  accepted.SetPattern(600, 744);

  TestBitmap excessiveHorizontalShift(1920, 1080, 400, 200, 302, 546);
  excessiveHorizontalShift.SetPattern(400, 200);
  excessiveHorizontalShift.SetPattern(700, 744);

  TestBitmap excessiveVerticalShift(1920, 1080, 400, 200, 202, 552);
  excessiveVerticalShift.SetPattern(400, 200);
  excessiveVerticalShift.SetPattern(600, 750);

  EXPECT_TRUE(IsStereoBitmap(accepted.Get(), BitmapStereoLayout::TOP_BOTTOM));
  EXPECT_FALSE(IsStereoBitmap(excessiveHorizontalShift.Get(), BitmapStereoLayout::TOP_BOTTOM));
  EXPECT_FALSE(IsStereoBitmap(excessiveVerticalShift.Get(), BitmapStereoLayout::TOP_BOTTOM));
}

TEST(TestDVDOverlayCodecFFmpegUtils, DerivesLayoutFromVideoStereoMode)
{
  EXPECT_EQ(GetBitmapStereoLayout("left_right"), BitmapStereoLayout::LEFT_RIGHT);
  EXPECT_EQ(GetBitmapStereoLayout("right_left"), BitmapStereoLayout::LEFT_RIGHT);
  EXPECT_EQ(GetBitmapStereoLayout("top_bottom"), BitmapStereoLayout::TOP_BOTTOM);
  EXPECT_EQ(GetBitmapStereoLayout("bottom_top"), BitmapStereoLayout::TOP_BOTTOM);
  EXPECT_EQ(GetBitmapStereoLayout("mono"), BitmapStereoLayout::MONO);
  EXPECT_EQ(GetBitmapStereoLayout("anaglyph_cyan_red"), BitmapStereoLayout::MONO);
}

TEST(TestDVDOverlayCodecFFmpegUtils, CropsAtPackedViewBoundary)
{
  BitmapSubtitle bitmap;
  bitmap.x = 300;
  bitmap.y = 100;
  bitmap.width = 1300;
  bitmap.height = 80;
  bitmap.sourceWidth = 1920;
  bitmap.sourceHeight = 1080;

  const BitmapStereoCrop left = GetStereoCrop(bitmap, BitmapStereoLayout::LEFT_RIGHT, false);
  EXPECT_EQ(left.packedX, 300);
  EXPECT_EQ(left.x, 300);
  EXPECT_EQ(left.width, 660);
  EXPECT_EQ(left.sourceWidth, 960);

  const BitmapStereoCrop right = GetStereoCrop(bitmap, BitmapStereoLayout::LEFT_RIGHT, true);
  EXPECT_EQ(right.packedX, 960);
  EXPECT_EQ(right.x, 0);
  EXPECT_EQ(right.width, 640);
  EXPECT_EQ(right.sourceWidth, 960);
}

TEST(TestDVDOverlayCodecFFmpegUtils, CropsAtPackedViewBoundaryTopBottom)
{
  BitmapSubtitle bitmap;
  bitmap.x = 100;
  bitmap.y = 400;
  bitmap.width = 800;
  bitmap.height = 300;
  bitmap.sourceWidth = 1920;
  bitmap.sourceHeight = 1080;

  const BitmapStereoCrop top = GetStereoCrop(bitmap, BitmapStereoLayout::TOP_BOTTOM, false);
  EXPECT_EQ(top.packedY, 400);
  EXPECT_EQ(top.y, 400);
  EXPECT_EQ(top.height, 140);
  EXPECT_EQ(top.sourceHeight, 540);

  const BitmapStereoCrop bottom = GetStereoCrop(bitmap, BitmapStereoLayout::TOP_BOTTOM, true);
  EXPECT_EQ(bottom.packedY, 540);
  EXPECT_EQ(bottom.y, 0);
  EXPECT_EQ(bottom.height, 160);
  EXPECT_EQ(bottom.sourceHeight, 540);
}

TEST(TestDVDOverlayCodecFFmpegUtils, RejectsOddCanvas)
{
  BitmapSubtitle bitmap;
  bitmap.width = 100;
  bitmap.height = 50;
  bitmap.sourceWidth = 1921;
  bitmap.sourceHeight = 1081;

  EXPECT_TRUE(GetStereoCrop(bitmap, BitmapStereoLayout::LEFT_RIGHT, false).IsEmpty());
  EXPECT_TRUE(GetStereoCrop(bitmap, BitmapStereoLayout::TOP_BOTTOM, false).IsEmpty());
}
