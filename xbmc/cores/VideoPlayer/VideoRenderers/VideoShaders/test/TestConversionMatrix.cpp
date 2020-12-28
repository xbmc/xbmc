/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "cores/VideoPlayer/VideoRenderers/VideoShaders/ConversionMatrix.h"

#include <iostream>

#include <gtest/gtest.h>

// clang-format off

namespace
{

constexpr bool DEBUG_PRINT{false};

void DebugPrint(Matrix3 matrix)
{
  if (!DEBUG_PRINT)
    return;

  std::cout << std::fixed;

  std::cout << "===== Matrix Start =====" << std::endl;

  for (unsigned int i = 0; i < 3; i++)
  {
    std::cout << std::setprecision(10)
              << "{" << matrix[i][0] << ", "
              <<        matrix[i][1] << ", "
              <<        matrix[i][2] << "}," << std::endl;
  }

  std::cout << "===== Matrix End =====" << std::endl;
}

void DebugPrint(Matrix4 matrix)
{
  if (!DEBUG_PRINT)
    return;

  std::cout << std::fixed;

  std::cout << "===== Matrix Start =====" << std::endl;

  for (unsigned int i = 0; i < 4; i++)
  {
    std::cout << std::setprecision(10)
              << "{" << matrix[i][0] << ", "
              <<        matrix[i][1] << ", "
              <<        matrix[i][2] << ", "
              <<        matrix[i][3] << "}," << std::endl;
  }

  std::cout << "===== Matrix End =====" << std::endl;
}

std::array<std::array<float, 4>, 4> bt709_8bit =
{{
  {1.1643834114, 1.1643834114, 1.1643834114, 0.0000000000},
  {0.0000000000, -0.2132485956, 2.1124014854, 0.0000000000},
  {1.7927408218, -0.5329092145, 0.0000000000, 0.0000000000},
  {-0.9729449749, 0.3014826477, -1.1334021091, 1.0000000000}
}};

Matrix4 bt709Mat_8bit(bt709_8bit);

std::array<std::array<float, 4>, 4> bt709_10bit =
{{
  {1.1643834114, 1.1643834114, 1.1643834114, 0.0000000000},
  {0.0000000000, -0.2132485956, 2.1124014854, 0.0000000000},
  {1.7927408218, -0.5329092145, 0.0000000000, 0.0000000000},
  {-0.9729449749, 0.3014826477, -1.1334021091, 1.0000000000},
}};

Matrix4 bt709Mat_10bit(bt709_10bit);

std::array<std::array<float, 4>, 4> bt709_10bit_texture =
{{
  {74.5922470093, 74.5922470093, 74.5922470093, 0.0000000000},
  {0.0000000000, -13.6610431671, 135.3237915039, 0.0000000000},
  {114.8458175659, -34.1390075684, 0.0000000000, 0.0000000000},
  {-0.9729449749, 0.3014826477, -1.1334021091, 1.0000000000},
}};

Matrix4 bt709Mat_10bit_texture(bt709_10bit_texture);

std::array<std::array<float, 3>, 3> bt601_to_bt709 =
{{
  {1.0440435410, -0.0000000230, 0.0000000056},
  {-0.0440433398, 0.9999997616, 0.0117934495},
  {-0.0000000298, -0.0000000075, 0.9882065654},
}};

Matrix3 bt601_to_bt709_Mat(bt601_to_bt709);

std::array<std::array<float, 3>, 3> bt2020_to_bt709 =
{{
  {1.6604905128, -0.1245504916, -0.0181507617},
  {-0.5876411796, 1.1328998804, -0.1005788445},
  {-0.0728498399, -0.0083493963, 1.1187294722},
}};

Matrix3 bt2020_to_bt709_Mat(bt2020_to_bt709);

} // namespace

TEST(TestConvertMatrix, YUV2RGB)
{
  CConvertMatrix convMat;
  convMat.SetSourceColorSpace(AVCOL_SPC_BT709)
         .SetSourceLimitedRange(true);

  convMat.SetDestinationBlack(0.0f)
         .SetDestinationContrast(1.0f)
         .SetDestinationLimitedRange(false);

  Matrix4 yuvMat;

  convMat.SetSourceBitDepth(8)
         .SetSourceTextureBitDepth(8);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_EQ(bt709Mat_8bit, yuvMat);

  convMat.SetSourceBitDepth(10)
         .SetSourceTextureBitDepth(8);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_EQ(bt709Mat_10bit, yuvMat);

  convMat.SetSourceBitDepth(10)
         .SetSourceTextureBitDepth(10);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_EQ(bt709Mat_10bit_texture, yuvMat);
}

TEST(TestConvertMatrix, ColorSpaceConversion)
{
  CConvertMatrix convMat;
  Matrix3 primMat;

  convMat.SetSourceColorPrimaries(AVCOL_PRI_BT470BG)
         .SetDestinationColorPrimaries(AVCOL_PRI_BT709);
  primMat = convMat.GetPrimMat();
  DebugPrint(primMat);

  EXPECT_EQ(bt601_to_bt709_Mat, primMat);

  convMat.SetSourceColorPrimaries(AVCOL_PRI_BT2020)
         .SetDestinationColorPrimaries(AVCOL_PRI_BT709);
  primMat = convMat.GetPrimMat();
  DebugPrint(primMat);

  EXPECT_EQ(bt2020_to_bt709_Mat, primMat);
}

// clang-format on
