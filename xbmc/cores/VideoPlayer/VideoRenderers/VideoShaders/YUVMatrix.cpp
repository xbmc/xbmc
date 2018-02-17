/*
 *      Copyright (c) 2007 d4rk
 *      Copyright (C) 2007-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../RenderFlags.h"
#include "YUVMatrix.h"

// http://www.martinreddy.net/gfx/faqs/colorconv.faq

//
// Transformation matrixes for different colorspaces.
//
static float yuv_coef_bt601[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.344f,   1.773f,   0.0f },
    { 1.403f,   -0.714f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_bt709[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.1870f,  1.8556f,  0.0f },
    { 1.5701f,  -0.4664f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_bt2020[4][4] =
{
    { 1.0f,     1.0f,     1.0f,    0.0f },
    { 0.0f,    -0.1645f,  1.8814f, 0.0f },
    { 1.4745f, -0.5713f,  0.0f,    0.0f },
    { 0.0f,     0.0f,     0.0f,    0.0f }
};

static float yuv_coef_ebu[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.3960f,  2.029f,   0.0f },
    { 1.140f,   -0.581f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_smtp240m[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.2253f,  1.8270f,  0.0f },
    { 1.5756f,  -0.5000f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float** PickYUVConversionMatrix(unsigned flags)
{
  // Pick the matrix.

   switch(CONF_FLAGS_YUVCOEF_MASK(flags))
   {
     case CONF_FLAGS_YUVCOEF_240M:
       return reinterpret_cast<float**>(yuv_coef_smtp240m);
     case CONF_FLAGS_YUVCOEF_BT2020:
       return reinterpret_cast<float**>(yuv_coef_bt2020);
     case CONF_FLAGS_YUVCOEF_BT709:
       return reinterpret_cast<float**>(yuv_coef_bt709);
     case CONF_FLAGS_YUVCOEF_BT601:
       return reinterpret_cast<float**>(yuv_coef_bt601);
     case CONF_FLAGS_YUVCOEF_EBU:
       return reinterpret_cast<float**>(yuv_coef_ebu);
   }

   return reinterpret_cast<float**>(yuv_coef_bt601);
}

void CalculateYUVMatrix(TransformMatrix &matrix
                        , unsigned int  flags
                        , EShaderFormat format
                        , float         black
                        , float         contrast
                        , bool          limited)
{
  TransformMatrix coef;

  matrix *= TransformMatrix::CreateScaler(contrast, contrast, contrast);
  matrix *= TransformMatrix::CreateTranslation(black, black, black);

  float (*conv)[4] = (float (*)[4])PickYUVConversionMatrix(flags);
  for(int row = 0; row < 3; row++)
    for(int col = 0; col < 4; col++)
      coef.m[row][col] = conv[col][row];
  coef.identity = false;

  if (limited)
  {
    matrix *= TransformMatrix::CreateTranslation(+ 16.0f / 255
                                               , + 16.0f / 255
                                               , + 16.0f / 255);
    matrix *= TransformMatrix::CreateScaler((235 - 16) / 255.0f
                                          , (235 - 16) / 255.0f
                                          , (235 - 16) / 255.0f);
  }

  matrix *= coef;
  matrix *= TransformMatrix::CreateTranslation(0.0, -0.5, -0.5);

  if (!(flags & CONF_FLAGS_YUV_FULLRANGE))
  {
    matrix *= TransformMatrix::CreateScaler(255.0f / (235 - 16)
                                          , 255.0f / (240 - 16)
                                          , 255.0f / (240 - 16));
    matrix *= TransformMatrix::CreateTranslation(- 16.0f / 255
                                               , - 16.0f / 255
                                               , - 16.0f / 255);
  }

  int effectiveBpp;
  switch (format)
  {
    case SHADER_YV12_9:
      effectiveBpp = 9;
      break;
    case SHADER_YV12_10:
      effectiveBpp = 10;
      break;
    case SHADER_YV12_12:
      effectiveBpp = 12;
      break;
    case SHADER_YV12_14:
      effectiveBpp = 14;
      break;
    default:
      effectiveBpp = 0;
  }

  if (effectiveBpp > 8 && effectiveBpp < 16)
  {
    // Convert range to 2 bytes
    float scale = 65535.0f / ((1 << effectiveBpp) - 1);
    matrix *= TransformMatrix::CreateScaler(scale, scale, scale);
  }
}
