/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/VideoRenderers/VideoShaders/ConversionMatrix.h"
#include "utils/MathUtils.h"

#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

// clang-format off

namespace
{

constexpr bool DEBUG_PRINT{false};

void DebugPrint(const Matrix3& matrix)
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

void DebugPrint(const Matrix4& matrix)
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

template<uint8_t Order>
bool CompareMatrices(CMatrix<Order>& matrixA, CMatrix<Order>& matrixB, float threshold)
{
  bool result = true;
  for (unsigned int i = 0; i < Order; i++)
  {
    for (unsigned int j = 0; j < Order; j++)
    {
      result &= MathUtils::FloatEquals(matrixA[i][j], matrixB[i][j], threshold);
    }
  }
  return result;
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
  {1.1678080559, 1.1678080559, 1.1678080559, 0.0000000000},
  {0.0000000000, -0.2138758004, 2.1186144352, 0.0000000000},
  {1.7980136871, -0.5344766378, 0.0000000000, 0.0000000000},
  {-0.9729449749, 0.3014826477, -1.1334021091, 1.0000000000},
}};

Matrix4 bt709Mat_10bit(bt709_10bit);

std::array<std::array<float, 4>, 4> bt709_12bit =
{{
  {1.1686643836, 1.1686643836, 1.1686643836, 0.0000000000},
  {0.0000000000, -0.2140326165, 2.1201679688, 0.0000000000},
  {1.7993320313, -0.5348685540, 0.0000000000, 0.0000000000},
  {-0.9729450750, 0.3014826655, -1.1334022179, 1.0000000000},
}};

Matrix4 bt709Mat_12bit(bt709_12bit);

std::array<std::array<float, 4>, 4> bt709_16bit =
{{
  {1.1689319349, 1.1689319349, 1.1689319349, 0.0000000000},
  {0.0000000000, -0.2140816167, 2.1206533552, 0.0000000000},
  {1.7997439662, -0.5349910056, 0.0000000000, 0.0000000000},
  {-0.9729450750, 0.3014826655, -1.1334022179, 1.0000000000},
}};

Matrix4 bt709Mat_16bit(bt709_16bit);

std::array<std::array<float, 4>, 4> bt709_10bit_texture =
{{
  {74.8116378784, 74.8116378784, 74.8116378784, 0.0000000000},
  {0.0000000000, -13.7012224197, 135.7218017578, 0.0000000000},
  {115.1836090088, -34.2394218445, 0.0000000000, 0.0000000000},
  {-0.9729449749, 0.3014826477, -1.1334021091, 1.0000000000},
}};

Matrix4 bt709Mat_10bit_texture(bt709_10bit_texture);

std::array<std::array<float, 4>, 4> gbr_full =
{{
  {0.0000000000, 1.0000000000, 0.0000000000, 0.0000000000},
  {0.0000000000, 0.0000000000, 1.0000000000, 0.0000000000},
  {1.0000000000, 0.0000000000, 0.0000000000, 0.0000000000},
  {0.0000000000, 0.0000000000, 0.0000000000, 1.0000000000},
}};

Matrix4 gbrMat_full(gbr_full);

std::array<std::array<float, 4>, 4> gbr_8bit =
{{
  {0.0000000000, 1.1643835616, 0.0000000000, 0.0000000000},
  {0.0000000000, 0.0000000000, 1.1643835616, 0.0000000000},
  {1.1643835616, 0.0000000000, 0.0000000000, 0.0000000000},
  {-0.0730593607, -0.0730593607, -0.0730593607, 1.0000000000},
}};

Matrix4 gbrMat_8bit(gbr_8bit);

std::array<std::array<float, 4>, 4> gbr_10bit_texture =
{{
  {0.0000000000, 74.8116438356, 0.0000000000, 0.0000000000},
  {0.0000000000, 0.0000000000, 74.8116438356, 0.0000000000},
  {74.8116438356, 0.0000000000, 0.0000000000, 0.0000000000},
  {-0.0730593607, -0.0730593607, -0.0730593607, 1.0000000000},
}};

Matrix4 gbrMat_10bit_texture(gbr_10bit_texture);

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

  EXPECT_TRUE(CompareMatrices(bt709Mat_8bit, yuvMat, 0.0001f));

  convMat.SetSourceBitDepth(10)
         .SetSourceTextureBitDepth(8);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_TRUE(CompareMatrices(bt709Mat_10bit, yuvMat, 0.0001f));

  convMat.SetSourceBitDepth(12)
         .SetSourceTextureBitDepth(8);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_TRUE(CompareMatrices(bt709Mat_12bit, yuvMat, 0.0001f));

  convMat.SetSourceBitDepth(16)
         .SetSourceTextureBitDepth(8);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_TRUE(CompareMatrices(bt709Mat_16bit, yuvMat, 0.0001f));

  convMat.SetSourceBitDepth(10)
         .SetSourceTextureBitDepth(10);
  yuvMat = convMat.GetYuvMat();
  DebugPrint(yuvMat);

  EXPECT_TRUE(CompareMatrices(bt709Mat_10bit_texture, yuvMat, 0.0001f));
}

TEST(TestConvertMatrix, GBR2RGB)
{
  CConvertMatrix convMat;
  convMat.SetSourceRGB(true)
         .SetSourceLimitedRange(false);

  convMat.SetDestinationBlack(0.0f)
         .SetDestinationContrast(1.0f)
         .SetDestinationLimitedRange(false);

  Matrix4 rgbMat;

  convMat.SetSourceBitDepth(8)
         .SetSourceTextureBitDepth(8);
  rgbMat = convMat.GetYuvMat();
  DebugPrint(rgbMat);

  EXPECT_TRUE(CompareMatrices(gbrMat_full, rgbMat, 0.0001f));

  convMat.SetSourceLimitedRange(true);
  rgbMat = convMat.GetYuvMat();
  DebugPrint(rgbMat);

  EXPECT_TRUE(CompareMatrices(gbrMat_8bit, rgbMat, 0.0001f));

  convMat.SetSourceBitDepth(10)
         .SetSourceTextureBitDepth(10);
  rgbMat = convMat.GetYuvMat();
  DebugPrint(rgbMat);

  EXPECT_TRUE(CompareMatrices(gbrMat_10bit_texture, rgbMat, 0.0001f));
}

TEST(TestConvertMatrix, ColorSpaceConversion)
{
  CConvertMatrix convMat;
  Matrix3 primMat;

  convMat.SetSourceColorPrimaries(AVCOL_PRI_BT470BG)
         .SetDestinationColorPrimaries(AVCOL_PRI_BT709);
  primMat = convMat.GetPrimMat();
  DebugPrint(primMat);

  EXPECT_TRUE(CompareMatrices(bt601_to_bt709_Mat, primMat, 0.0001f));

  convMat.SetSourceColorPrimaries(AVCOL_PRI_BT2020)
         .SetDestinationColorPrimaries(AVCOL_PRI_BT709);
  primMat = convMat.GetPrimMat();
  DebugPrint(primMat);

  EXPECT_TRUE(CompareMatrices(bt2020_to_bt709_Mat, primMat, 0.0001f));
}

// clang-format on
