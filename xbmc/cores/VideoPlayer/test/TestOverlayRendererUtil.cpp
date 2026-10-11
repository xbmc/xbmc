/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/VideoRenderers/OverlayRendererUtil.h"

#include <array>
#include <vector>

#include <ass/ass.h>
#include <gtest/gtest.h>

TEST(TestOverlayRendererUtil, BoundedPagesPreserveImages)
{
  constexpr int limit = 8;
  // Strided bitmap with no padding after the last row, larger than a page.
  std::vector<unsigned char> bitmap(23 * 16 + 19);
  for (size_t i = 0; i < bitmap.size(); ++i)
    bitmap[i] = i % 251;

  std::array<ASS_Image, 3> images{};
  for (size_t i = 0; i < images.size(); ++i)
  {
    images[i].w = i == 0 ? 19 : 7;
    images[i].h = i == 0 ? 17 : 7;
    images[i].stride = 23;
    images[i].bitmap = bitmap.data();
    images[i].color = i == 0 ? 0x12345600 : 0xabcdef30;
    images[i].dst_x = 30 * i;
    images[i].dst_y = 40 * i;
    images[i].next = i + 1 < images.size() ? &images[i + 1] : nullptr;
  }
  images.back().color = 0xffffffff;

  std::vector<OVERLAY::SQuads> pages;
  ASSERT_TRUE(OVERLAY::convert_quad_pages(images.data(), pages, limit));
  ASSERT_GT(pages.size(), 1u);
  std::array<std::vector<unsigned char>, 2> restored;
  std::array<std::vector<int>, 2> coverage;
  for (size_t i = 0; i < restored.size(); ++i)
  {
    restored[i].resize(images[i].w * images[i].h);
    coverage[i].resize(restored[i].size());
  }

  size_t previousImage = 0;
  for (const auto& page : pages)
  {
    ASSERT_GT(page.size_x, 0);
    ASSERT_GT(page.size_y, 0);
    ASSERT_LE(page.size_x, limit);
    ASSERT_LE(page.size_y, limit);
    ASSERT_EQ(page.texture.size(), static_cast<size_t>(page.size_x * page.size_y));
    for (const auto& quad : page.quad)
    {
      const size_t index = quad.r == 0x12 ? 0 : 1;
      EXPECT_GE(index, previousImage);
      previousImage = index;
      const auto& image = images[index];
      EXPECT_EQ(quad.r, (image.color >> 24) & 0xff);
      EXPECT_EQ(quad.g, (image.color >> 16) & 0xff);
      EXPECT_EQ(quad.b, (image.color >> 8) & 0xff);
      EXPECT_EQ(quad.a, 255 - (image.color & 0xff));
      ASSERT_GE(quad.u, 0);
      ASSERT_GE(quad.v, 0);
      ASSERT_LE(quad.u + quad.w, page.size_x);
      ASSERT_LE(quad.v + quad.h, page.size_y);
      const int x = quad.x - image.dst_x;
      const int y = quad.y - image.dst_y;
      ASSERT_GE(x, 0);
      ASSERT_GE(y, 0);
      ASSERT_LE(x + quad.w, image.w);
      ASSERT_LE(y + quad.h, image.h);
      for (int row = 0; row < quad.h; ++row)
        for (int col = 0; col < quad.w; ++col)
        {
          const size_t dest = (y + row) * image.w + x + col;
          restored[index][dest] = page.texture[(quad.v + row) * page.size_x + quad.u + col];
          ++coverage[index][dest];
        }
    }
  }
  for (size_t i = 0; i < restored.size(); ++i)
    for (int y = 0; y < images[i].h; ++y)
      for (int x = 0; x < images[i].w; ++x)
      {
        EXPECT_EQ(coverage[i][y * images[i].w + x], 1);
        EXPECT_EQ(restored[i][y * images[i].w + x], bitmap[y * images[i].stride + x]);
      }

  EXPECT_FALSE(OVERLAY::convert_quad_pages(nullptr, pages, limit));
  EXPECT_TRUE(pages.empty());
  EXPECT_FALSE(OVERLAY::convert_quad_pages(&images.back(), pages, limit));
  EXPECT_FALSE(OVERLAY::convert_quad_pages(images.data(), pages, 1));
}
