/*
 *      Copyright (C) 2005-2018 Team XBMC
 *      http://kodi.tv
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <memory>

extern "C" {
#include "libavutil/pixfmt.h"
}

template <unsigned Order>
class CMatrix
{
public:
  CMatrix(float (&src)[Order][Order]);
  CMatrix(float (&src)[Order-1][Order-1]);
  CMatrix& operator=(const CMatrix& src);
  CMatrix& operator=(const float (&src)[Order-1][Order-1]);
  virtual ~CMatrix() = default;

  float (&Get())[Order][Order];
  CMatrix Invert();
  CMatrix operator*(const CMatrix& other);
  CMatrix operator*=(const CMatrix& other);
  virtual CMatrix operator*(const float (&other)[Order][Order]);

protected:
  CMatrix() = default;

  void Copy(float (&dst)[Order][Order], const float (&src)[Order][Order]);
  void Invert(float (&dst)[Order][Order], float (&src)[Order][Order]);

  float m_mat[Order][Order] = {};
};

class CGlMatrix : public CMatrix<4>
{
public:
  CGlMatrix() = default;
  CGlMatrix(float (&src)[3][3]);
  virtual ~CGlMatrix() = default;
  CMatrix operator*(const float (&other)[4][4]) override;
};

class CScale : public CGlMatrix
{
public:
  CScale(float x, float y, float z);
  virtual ~CScale() = default;
};

class CTranslate : public CGlMatrix
{
public:
  CTranslate(float x, float y, float z);
  virtual ~CTranslate() = default;
};

class ConversionToRGB : public CMatrix<3>
{
public:
  ConversionToRGB(float Kr, float Kb);
  virtual ~ConversionToRGB() = default;
  ConversionToRGB& operator=(const float (&src)[3][3]);

protected:
  ConversionToRGB() = default;

  float a11, a12, a13;
  float CbDen, CrDen;
};

class PrimaryToXYZ : public CMatrix<3>
{
public:
  PrimaryToXYZ(const float (&primaries)[3][2], const float (&whitepoint)[2]);
  virtual ~PrimaryToXYZ() = default;

protected:
  PrimaryToXYZ() = default;
  float CalcBy(const float p[3][2], const float w[2]);
  float CalcGy(const float p[3][2], const float w[2], const float By);
  float CalcRy(const float By, const float Gy);
};

class PrimaryToRGB : public PrimaryToXYZ
{
public:
  PrimaryToRGB(float (&primaries)[3][2], float (&whitepoint)[2]);
  virtual ~PrimaryToRGB() = default;
};

//------------------------------------------------------------------------------

class CConvertMatrix
{
public:
  CConvertMatrix() = default;
  virtual ~CConvertMatrix() = default;

  void SetColParams(AVColorSpace colSpace, int bits, bool limited, int textuteBits);
  void SetColPrimaries(AVColorPrimaries dst, AVColorPrimaries src);
  void SetParams(float contrast, float black, bool limited);
  void GetYuvMat(float (&mat)[4][4]);
  bool GetPrimMat(float (&mat)[3][3]);
  float GetGammaSrc();
  float GetGammaDst();

  static bool GetRGBYuvCoefs(AVColorSpace colspace, float (&coefs)[3]);

protected:
  void GenMat();

  std::unique_ptr<CGlMatrix> m_pMat;
  std::unique_ptr<CMatrix<3>> m_pMatPrim;
  AVColorSpace m_colSpace = AVCOL_SPC_BT709;
  AVColorPrimaries m_colPrimariesSrc = AVCOL_PRI_BT709;
  float m_gammaSrc = 2.2f;
  bool m_limitedSrc = true;
  AVColorPrimaries m_colPrimariesDst = AVCOL_PRI_BT709;
  float m_gammaDst = 2.2f;
  bool m_limitedDst = false;
  int m_srcBits = 8;
  int m_srcTextureBits = 8;
  int m_dstBits = 8;
  float m_contrast = 1.0;
  float m_black = 0.0;
};
