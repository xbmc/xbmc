/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/RenderBezel.h"

#include <stdint.h>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
constexpr unsigned int WIDTH = 64;
constexpr unsigned int HEIGHT = 36;

struct Picture
{
  Picture(unsigned int padding = 0) : pitch((WIDTH + padding) * 4), pixels(pitch * HEIGHT, 0xff) {}

  uint8_t& Alpha(unsigned int x, unsigned int y) { return pixels[y * pitch + x * 4 + 3]; }

  void Fill(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, uint8_t alpha)
  {
    for (unsigned int y = y1; y < y2; y++)
      for (unsigned int x = x1; x < x2; x++)
        Alpha(x, y) = alpha;
  }

  unsigned int pitch;
  std::vector<uint8_t> pixels;
};
} // namespace

TEST(TestRenderBezel, FindsClearWindow)
{
  Picture picture;
  picture.Fill(12, 4, 52, 32, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 4, 52, 32), window);
}

TEST(TestRenderBezel, FindsGlassWindow)
{
  Picture picture;
  picture.Fill(12, 4, 52, 32, 128);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 4, 52, 32), window);
}

TEST(TestRenderBezel, FindsWindowInPaddedRows)
{
  Picture picture(16);
  picture.Fill(12, 4, 52, 32, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 4, 52, 32), window);
}

TEST(TestRenderBezel, FindsWindowReachingEdges)
{
  Picture picture;
  picture.Fill(20, 0, 44, HEIGHT, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(20, 0, 44, HEIGHT), window);
}

TEST(TestRenderBezel, OpaquePictureHasNoWindow)
{
  Picture picture;

  CRect window;
  EXPECT_FALSE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
}

TEST(TestRenderBezel, CountsSoftEdgeAsWindow)
{
  Picture picture;
  picture.Fill(12, 4, 52, 32, 0);
  picture.Fill(9, 4, 12, 32, 254); // the frame's soft edge, short of opaque

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(9, 4, 52, 32), window);
}

TEST(TestRenderBezel, IgnoresStrayOpaquePixels)
{
  Picture picture;
  picture.Fill(12, 4, 52, 32, 0);
  picture.Alpha(20, HEIGHT / 2) = 255;
  picture.Alpha(40, HEIGHT / 2) = 255;
  picture.Alpha(41, HEIGHT / 2) = 255;

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 4, 52, 32), window);
}

TEST(TestRenderBezel, FindsThinFrameAtPictureEdge)
{
  Picture picture;
  picture.Fill(12, 2, 52, 35, 0); // 2 opaque rows above the window, 1 below

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 2, 52, 35), window);
}

TEST(TestRenderBezel, FindsWindowWithNoFrameAtEdge)
{
  Picture picture;
  picture.Fill(12, 0, 52, HEIGHT, 0);
  picture.Alpha(WIDTH / 2, 1) = 255; // a stray opaque pixel near the edge, not at it

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindGameWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 0, 52, HEIGHT), window);
}

TEST(TestRenderBezel, FramesGameOfWindowShape)
{
  // A console bezel's 4:3 window on a 1080p screen
  const CRect window(259, 18, 1661, 1061);
  EXPECT_TRUE(CRenderBezel::FramesGame(window, {265, 18, 1656, 1061}, {240, 0, 1680, 1080}));
}

TEST(TestRenderBezel, FramesWiderGameWithBars)
{
  // A 3:2 handheld game in a 4:3 window
  const CRect window(260, 2, 1659, 1077);
  EXPECT_TRUE(CRenderBezel::FramesGame(window, {260, 73, 1659, 1006}, {150, 0, 1770, 1080}));
}

TEST(TestRenderBezel, SkipsSmallWindow)
{
  // A photo of a cabinet, its screen a little over half the screen's height
  const CRect window(707, 182, 1238, 875);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {713, 182, 1233, 875}, {555, 0, 1365, 1080}));
}

TEST(TestRenderBezel, SkipsWindowOfWrongShape)
{
  // A game turned on its side, in a window made for it upright
  const CRect window(259, 18, 1661, 1061);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {569, 18, 1351, 1061}, {555, 0, 1365, 1080}));
}

TEST(TestRenderBezel, SkipsGameStretchedToScreen)
{
  // Stretched to fill a 1080p screen, the game would shrink to fit a 4:3 window
  const CRect window(259, 18, 1661, 1061);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {259, 18, 1661, 1061}, {0, 0, 1920, 1080}));
}

TEST(TestRenderBezel, SkipsGameLargerThanWindow)
{
  const CRect window(259, 18, 1661, 1061);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {240, 0, 1680, 1080}, {240, 0, 1680, 1080}));
}
