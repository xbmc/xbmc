/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/VideoRenderers/RenderGeometryFBO.h"

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestRenderGeometryFBO, BlackBarBoundsAreIndependentOfRotation)
{
  const ViewportCoordinates rotations[] = {{{{100, 80}, {420, 80}, {420, 320}, {100, 320}}},
                                           {{{100, 320}, {100, 80}, {420, 80}, {420, 320}}},
                                           {{{420, 320}, {100, 320}, {100, 80}, {420, 80}}},
                                           {{{420, 80}, {420, 320}, {100, 320}, {100, 80}}}};
  for (const auto& coordinates : rotations)
    EXPECT_EQ(CRect(100, 80, 420, 320), CRenderGeometryFBO::GetDestinationRect(coordinates));
}

TEST(TestRenderGeometryFBO, FrameInOversizedTexture)
{
  EXPECT_EQ(CRect(0, 0, 0.3125f, 0.46875f),
            CRenderGeometryFBO::GetTextureCoordinates({0, 0, 320, 240}, 240, 1024, 512, false));
  EXPECT_EQ(CRect(0, 0.453125f, 0.3125f, 0),
            CRenderGeometryFBO::GetTextureCoordinates({0, 0, 1280, 928}, 928, 4096, 2048, true));
}

TEST(TestRenderGeometryFBO, BottomLeftCropIsRelativeToFrame)
{
  EXPECT_EQ(CRect(0.03125f, 0.40625f, 0.25f, 0.21875f),
            CRenderGeometryFBO::GetTextureCoordinates({32, 32, 256, 128}, 240, 1024, 512, true));
}

TEST(TestRenderGeometryFBO, ShaderTextureRetainsSourceCrop)
{
  EXPECT_EQ(CRect(0.1f, 0.125f, 0.9f, 0.75f),
            CRenderGeometryFBO::GetTextureCoordinates({32, 30, 288, 180}, 240, 320, 240, false));
}

TEST(TestRenderGeometryFBO, ShaderResolutionAccountsForZoomCrop)
{
  const ViewportCoordinates dest{{{100, 100}, {740, 100}, {740, 460}, {100, 460}}};
  EXPECT_EQ(CSize(640, 480),
            CRenderGeometryFBO::GetShaderOutputSize({0, 30, 320, 210}, 320, 240, dest));
}

TEST(TestRenderGeometryFBO, ShaderResolutionFollowsRotatedPreview)
{
  const ViewportCoordinates dest{{{100, 420}, {100, 100}, {340, 100}, {340, 420}}};
  EXPECT_EQ(CSize(320, 240),
            CRenderGeometryFBO::GetShaderOutputSize({0, 0, 320, 240}, 320, 240, dest));
}
