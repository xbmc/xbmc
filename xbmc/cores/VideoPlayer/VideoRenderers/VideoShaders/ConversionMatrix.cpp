/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ConversionMatrix.h"

#include <stdexcept>
#include <string>

//------------------------------------------------------------------------------
// constants for primaries and transfers functions and color models
//------------------------------------------------------------------------------

// source: https://www.khronos.org/registry/DataFormat/specs/1.2/dataformat.1.2.html#PRIMARY_CONVERSION

namespace
{
struct ConvYCbCr
{
  float Kr, Kb;
};

struct Primaries
{
  float primaries[3][2];
  float whitepoint[2];
};

constexpr ConvYCbCr BT709YCbCr = {0.2126, 0.0722};
constexpr ConvYCbCr BT601YCbCr = {0.299, 0.114};
constexpr ConvYCbCr BT2020YCbCr = {0.2627, 0.0593};
constexpr ConvYCbCr ST240YCbCr = {0.212, 0.087};

constexpr Primaries PrimariesBT709 = {{{0.640, 0.330}, {0.300, 0.600}, {0.150, 0.060}},
                                      {0.3127, 0.3290}};
constexpr Primaries PrimariesBT610_525 = {{{0.640, 0.340}, {0.310, 0.595}, {0.155, 0.070}},
                                          {0.3127, 0.3290}};
constexpr Primaries PrimariesBT610_625 = {{{0.640, 0.330}, {0.290, 0.600}, {0.150, 0.060}},
                                          {0.3127, 0.3290}};
constexpr Primaries PrimariesBT2020 = {{{0.708, 0.292}, {0.170, 0.797}, {0.131, 0.046}},
                                       {0.3127, 0.3290}};
} // namespace

//------------------------------------------------------------------------------
// Matrix helpers
//------------------------------------------------------------------------------
// source: http://timjones.io/blog/archive/2014/10/20/the-matrix-inverted

template<uint8_t Order>
float CalculateDeterminant(const std::array<std::array<float, Order>, Order>& src);

template<uint8_t Order>
std::array<std::array<float, Order - 1>, Order - 1> GetSubmatrix(
    const std::array<std::array<float, Order>, Order>& src, uint8_t row, uint8_t col)
{
  uint8_t colCount = 0;
  uint8_t rowCount = 0;

  std::array<std::array<float, Order - 1>, Order - 1> dest;

  for (int i = 0; i < Order; i++)
  {
    if (i != row)
    {
      colCount = 0;
      for (int j = 0; j < Order; j++)
      {
        if (j != col)
        {
          dest[rowCount][colCount] = src[i][j];
          colCount++;
        }
      }
      rowCount++;
    }
  }

  return dest;
}

template<uint8_t Order>
float CalculateMinor(const std::array<std::array<float, Order>, Order>& src,
                     uint8_t row,
                     uint8_t col)
{
  const std::array<std::array<float, Order - 1>, Order - 1> sub =
      GetSubmatrix<Order>(src, row, col);
  return CalculateDeterminant<Order - 1>(sub);
}

template<uint8_t Order>
float CalculateDeterminant(const std::array<std::array<float, Order>, Order>& src)
{
  float det = 0.0f;

  for (int i = 0; i < Order; i++)
  {
    // Get minor of element (0, i)
    float minor = CalculateMinor<Order>(src, 0, i);

    // If this is an odd-numbered row, negate the value.
    float factor = (i % 2 == 1) ? -1.0f : 1.0f;

    det += factor * src[0][i] * minor;
  }
  return det;
}

template<>
float CalculateDeterminant<2>(const std::array<std::array<float, 2>, 2>& src)
{
  return src[0][0] * src[1][1] - src[0][1] * src[1][0];
}

//------------------------------------------------------------------------------
// Matrix classes
//------------------------------------------------------------------------------

template<uint8_t Order>
CMatrix<Order>::CMatrix(const std::array<std::array<float, Order - 1>, Order - 1>& other)
{
  *this = other;
}

template<uint8_t Order>
CMatrix<Order>::CMatrix(const CMatrix<Order - 1>& other)
{
  *this = other.Get();
}

template<uint8_t Order>
CMatrix<Order>& CMatrix<Order>::operator=(
    const std::array<std::array<float, Order - 1>, Order - 1>& other)
{
  for (int i = 0; i < Order - 1; ++i)
    for (int j = 0; j < Order - 1; ++j)
      m_mat[i][j] = other[i][j];

  for (int i = 0; i < Order; ++i)
    m_mat[i][Order-1] = 0;

  for (int i = 0; i < Order; ++i)
    m_mat[Order-1][i] = 0;

  return *this;
}

template<uint8_t Order>
std::array<std::array<float, Order>, Order>& CMatrix<Order>::Get()
{
  return m_mat;
}

template<uint8_t Order>
const std::array<std::array<float, Order>, Order>& CMatrix<Order>::Get() const
{
  return m_mat;
}

template<uint8_t Order>
CMatrix<Order> CMatrix<Order>::operator*(const CMatrix& other)
{
  return *this * other.m_mat;
}

template<uint8_t Order>
CMatrix<Order>& CMatrix<Order>::operator*=(const CMatrix& other)
{
  CMatrix<Order> tmp = *this * other.m_mat;
  *this = tmp;
  return *this;
}

template<uint8_t Order>
CMatrix<Order> CMatrix<Order>::operator*(const std::array<std::array<float, Order>, Order>& other)
{
  CMatrix<Order> ret;
  for (int i = 0; i < Order; ++i)
    for (int j = 0; j < Order; ++j)
      for (int k = 0; k < Order; ++k)
        ret.m_mat[i][j] += m_mat[i][k] * other[k][j];

  return ret;
}

template<uint8_t Order>
CMatrix<Order>& CMatrix<Order>::Invert()
{
  CMatrix<Order> tmp;
  tmp.m_mat = Invert(m_mat);
  *this = tmp;
  return *this;
}

template<uint8_t Order>
std::array<std::array<float, Order>, Order> CMatrix<Order>::Invert(
    std::array<std::array<float, Order>, Order>& other) const
{
  // Calculate the inverse of the determinant of src.
  float det = CalculateDeterminant<Order>(other);
  float inverseDet = 1.0f / det;

  std::array<std::array<float, Order>, Order> dst;

  for (int j = 0; j < Order; j++)
  {
    for (int i = 0; i < Order; i++)
    {
      // Get minor of element (j, i) - not (i, j) because
      // this is where the transpose happens.
      float minor = CalculateMinor<Order>(other, j, i);

      // Multiply by (−1)^{i+j}
      float factor = ((i + j) % 2 == 1) ? -1.0f : 1.0f;
      float cofactor = minor * factor;

      dst[i][j] = inverseDet * cofactor;
    }
  }

  return dst;
}

CGlMatrix::CGlMatrix(const CMatrix<3>& other) : CMatrix<4>(other)
{
}

CGlMatrix::CGlMatrix(const std::array<std::array<float, 3>, 3>& other) : CMatrix<4>(other)
{
}

CGlMatrix::CMatrix CGlMatrix::operator*(const std::array<std::array<float, 4>, 4>& other)
{
  CGlMatrix ret;

  std::array<std::array<float, 4>, 4>& left = m_mat;
  const std::array<std::array<float, 4>, 4>& right = other;

  ret.m_mat[0][0] = left[0][0] * right[0][0] + left[0][1] * right[1][0] + left[0][2] * right[2][0];
  ret.m_mat[0][1] = left[0][0] * right[0][1] + left[0][1] * right[1][1] + left[0][2] * right[2][1];
  ret.m_mat[0][2] = left[0][0] * right[0][2] + left[0][1] * right[1][2] + left[0][2] * right[2][2];
  ret.m_mat[0][3] = left[0][0] * right[0][3] + left[0][1] * right[1][3] + left[0][2] * right[2][3] + left[0][3];
  ret.m_mat[1][0] = left[1][0] * right[0][0] + left[1][1] * right[1][0] + left[1][2] * right[2][0];
  ret.m_mat[1][1] = left[1][0] * right[0][1] + left[1][1] * right[1][1] + left[1][2] * right[2][1];
  ret.m_mat[1][2] = left[1][0] * right[0][2] + left[1][1] * right[1][2] + left[1][2] * right[2][2];
  ret.m_mat[1][3] = left[1][0] * right[0][3] + left[1][1] * right[1][3] + left[1][2] * right[2][3] + left[1][3];
  ret.m_mat[2][0] = left[2][0] * right[0][0] + left[2][1] * right[1][0] + left[2][2] * right[2][0];
  ret.m_mat[2][1] = left[2][0] * right[0][1] + left[2][1] * right[1][1] + left[2][2] * right[2][1];
  ret.m_mat[2][2] = left[2][0] * right[0][2] + left[2][1] * right[1][2] + left[2][2] * right[2][2];
  ret.m_mat[2][3] = left[2][0] * right[0][3] + left[2][1] * right[1][3] + left[2][2] * right[2][3] + left[2][3];

  return ret;
}

CScale::CScale(float x, float y, float z)
{
  m_mat[0][0] = x;
  m_mat[1][1] = y;
  m_mat[2][2] = z;
  m_mat[3][3] = 1;
}

CTranslate::CTranslate(float x, float y, float z)
{
  m_mat[0][0] = 1;
  m_mat[1][1] = 1;
  m_mat[2][2] = 1;
  m_mat[3][3] = 1;
  m_mat[0][3] = x;
  m_mat[1][3] = y;
  m_mat[2][3] = z;
}

//------------------------------------------------------------------------------
// Conversion classes
//------------------------------------------------------------------------------

ConversionToRGB::ConversionToRGB(float Kr, float Kb)
{
  float Kg = 1-Kr-Kb;
  a11 = Kr;
  a12 = Kg;
  a13 = Kb;
  CbDen = 2*(1-Kb);
  CrDen = 2*(1-Kr);

  m_mat[0][0] = a11;       m_mat[0][1] = a12;       m_mat[0][2] = a13;
  m_mat[1][0] = -Kr/CbDen; m_mat[1][1] = -Kg/CbDen; m_mat[1][2] = 0.5;
  m_mat[2][0] = 0.5;       m_mat[2][1] = -Kg/CrDen; m_mat[2][2] = -Kb/CrDen;

  m_mat = Invert(m_mat);
};

PrimaryToXYZ::PrimaryToXYZ(const float (&primaries)[3][2], const float (&whitepoint)[2])
{
  float By = CalcBy(primaries, whitepoint);
  float Gy = CalcGy(primaries, whitepoint, By);
  float Ry = CalcRy(By, Gy);

  m_mat[0][0] = Ry*primaries[0][0]/primaries[0][1];
  m_mat[0][1] = Gy*primaries[1][0]/primaries[1][1];
  m_mat[0][2] = By*primaries[2][0]/primaries[2][1];
  m_mat[1][0] = Ry;
  m_mat[1][1] = Gy;
  m_mat[1][2] = By;
  m_mat[2][0] = Ry/primaries[0][1] * (1- primaries[0][0] - primaries[0][1]);
  m_mat[2][1] = Gy/primaries[1][1] * (1- primaries[1][0] - primaries[1][1]);
  m_mat[2][2] = By/primaries[2][1] * (1- primaries[2][0] - primaries[2][1]);
}

float PrimaryToXYZ::CalcBy(const float p[3][2], const float w[2])
{
  float val = ((1-w[0])/w[1] - (1-p[0][0])/p[0][1]) * (p[1][0]/p[1][1] - p[0][0]/p[0][1]) -
  (w[0]/w[1] - p[0][0]/p[0][1]) * ((1-p[1][0])/p[1][1] - (1-p[0][0])/p[0][1]);

  val /= ((1-p[2][0])/p[2][1] - (1-p[0][0])/p[0][1]) * (p[1][0]/p[1][1] - p[0][0]/p[0][1]) -
  (p[2][0]/p[2][1] - p[0][0]/p[0][1]) * ((1-p[1][0])/p[1][1] - (1-p[0][0])/p[0][1]);

  return val;
}

float PrimaryToXYZ::CalcGy(const float p[3][2], const float w[2], const float By)
{
  float val = w[0]/w[1] - p[0][0]/p[0][1] - By * (p[2][0]/p[2][1] - p[0][0]/p[0][1]);
  val /= p[1][0]/p[1][1] - p[0][0]/p[0][1];

  return val;
}

float PrimaryToXYZ::CalcRy(const float By, const float Gy)
{
  return 1.0f - Gy - By;
}

PrimaryToRGB::PrimaryToRGB(float (&primaries)[3][2], float (&whitepoint)[2]) : PrimaryToXYZ(primaries, whitepoint)
{
  m_mat = Invert(m_mat);
}

//------------------------------------------------------------------------------

CConvertMatrix& CConvertMatrix::SetSourceColorSpace(AVColorSpace colorSpace)
{
  if (m_colSpace != colorSpace)
    m_mat.reset();

  m_colSpace = colorSpace;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetSourceBitDepth(int bits)
{
  if (m_srcBits != bits)
    m_mat.reset();

  m_srcBits = bits;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetSourceLimitedRange(bool limited)
{
  if (m_limitedSrc != limited)
    m_mat.reset();

  m_limitedSrc = limited;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetSourceTextureBitDepth(int textureBits)
{
  if (m_srcTextureBits != textureBits)
    m_mat.reset();

  m_srcTextureBits = textureBits;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetSourceColorPrimaries(AVColorPrimaries src)
{
  if (m_colPrimariesSrc != src)
    m_matPrim.reset();

  m_colPrimariesSrc = src;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetDestinationColorPrimaries(AVColorPrimaries dst)
{
  if (m_colPrimariesDst != dst)
    m_matPrim.reset();

  m_colPrimariesDst = dst;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetDestinationContrast(float contrast)
{
  m_contrast = contrast;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetDestinationBlack(float black)
{
  m_black = black;
  return *this;
}

CConvertMatrix& CConvertMatrix::SetDestinationLimitedRange(bool limited)
{
  m_limitedDst = limited;
  return *this;
}

const CMatrix<3>& CConvertMatrix::GenPrimMat()
{
  if (m_matPrim)
    return *m_matPrim;

  Primaries primToRGB;
  Primaries primToXYZ;
  switch (m_colPrimariesSrc)
  {
    case AVCOL_PRI_BT709:
      primToXYZ = PrimariesBT709;
      m_gammaSrc = 2.2;
      break;
    case AVCOL_PRI_BT470BG:
      primToXYZ = PrimariesBT610_625;
      m_gammaSrc = 2.2;
      break;
    case AVCOL_PRI_SMPTE170M:
    case AVCOL_PRI_SMPTE240M:
      primToXYZ = PrimariesBT610_525;
      m_gammaSrc = 2.2;
      break;
    case AVCOL_PRI_BT2020:
      primToXYZ = PrimariesBT2020;
      m_gammaSrc = 2.4;
      break;
    default:
      primToXYZ = PrimariesBT709;
      m_gammaSrc = 2.2;
      break;
  }
  switch (m_colPrimariesDst)
  {
    case AVCOL_PRI_BT709:
      primToRGB = PrimariesBT709;
      m_gammaDst = 2.2;
      break;
    case AVCOL_PRI_BT470BG:
      primToRGB = PrimariesBT610_625;
      m_gammaDst = 2.2;
      break;
    case AVCOL_PRI_SMPTE170M:
    case AVCOL_PRI_SMPTE240M:
      primToRGB = PrimariesBT610_525;
      m_gammaDst = 2.2;
      break;
    case AVCOL_PRI_BT2020:
      primToRGB = PrimariesBT2020;
      m_gammaDst = 2.4;
      break;
    default:
      primToRGB = PrimariesBT709;
      m_gammaDst = 2.2;
      break;
  }
  PrimaryToXYZ toXYZ(primToXYZ.primaries, primToXYZ.whitepoint);
  PrimaryToRGB toRGB(primToRGB.primaries, primToRGB.whitepoint);

  m_matPrim = std::make_unique<CMatrix<3>>(toRGB * toXYZ);

  return *m_matPrim;
}

const CGlMatrix& CConvertMatrix::GenMat()
{
  if (m_mat)
    return *m_mat;

  ConvYCbCr convYCbCr;
  switch (m_colSpace)
  {
    case AVCOL_SPC_BT709:
      convYCbCr = BT709YCbCr;
      break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      convYCbCr = BT601YCbCr;
      break;
    case AVCOL_SPC_SMPTE240M:
      convYCbCr = ST240YCbCr;
      break;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
      convYCbCr = BT2020YCbCr;
      break;
    default:
      convYCbCr = BT709YCbCr;
      break;
  }

  ConversionToRGB mConvRGB(convYCbCr.Kr, convYCbCr.Kb);
  CGlMatrix mat(mConvRGB);

  CTranslate trans(0, -0.5, -0.5);
  mat *= trans;

  if (m_limitedSrc)
  {
    if (m_srcBits >= 12)
    {
      CScale scale(4095.0f / (3760 - 256), 4095.0f / (3840 - 256), 4095.0f / (3840 - 256));
      CTranslate trans(-256.0f / 4095.0f, -256.0f / 4095.0f, -256.0f / 4095.0f);
      mat *= scale;
      mat *= trans;
    }
    else if (m_srcBits == 10)
    {
      CScale scale(1023.0f / (940 - 64), 1023.0f / (960 - 64), 1023.0f / (960 - 64));
      CTranslate trans(-64.0f / 1023.0f, -64.0f / 1023.0f, -64.0f / 1023.0f);
      mat *= scale;
      mat *= trans;
    }
    else
    {
      CScale scale(255.0f / (235 - 16), 255.0f / (240 - 16), 255.0f / (240 - 16));
      CTranslate trans(- 16.0f / 255, - 16.0f / 255, - 16.0f / 255);
      mat *= scale;
      mat *= trans;
    }
  }

  if (m_srcTextureBits > 8)
  {
    float val = 65535.0f / ((1 << m_srcTextureBits) - 1);
    CScale scale(val, val, val);
    mat *= scale;
  }

  m_mat = std::make_unique<CGlMatrix>(mat);

  return *m_mat;
}

Matrix4 CConvertMatrix::GetYuvMat()
{
  const CGlMatrix& mat = GenMat();

  CScale contrast(m_contrast, m_contrast, m_contrast);
  CTranslate black(m_black, m_black, m_black);

  CGlMatrix ret = contrast;
  ret *= black;
  if (m_limitedDst)
  {
    float valScale = (940.0f - 64.0f) / 1023.0f;
    float valTrans = 64.0f / 1023.0f;
    CScale scale(valScale, valScale, valScale);
    CTranslate trans(valTrans, valTrans, valTrans);
    ret *= trans;
    ret *= scale;
  }

  ret *= mat;

  Matrix4 dst;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      dst[i][j] = ret[j][i];

  dst[0][3] = 0.0f;
  dst[1][3] = 0.0f;
  dst[2][3] = 0.0f;
  dst[3][3] = 1.0f;

  return dst;
}

Matrix3 CConvertMatrix::GetPrimMat()
{
  if (m_colPrimariesDst == m_colPrimariesSrc)
    return Matrix3();

  const Matrix3& matPrim = GenPrimMat();

  Matrix3 dst;

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      dst[i][j] = matPrim[j][i];

  return dst;
}

float CConvertMatrix::GetGammaSrc()
{
  return m_gammaSrc;
}

float CConvertMatrix::GetGammaDst()
{
  return m_gammaDst;
}

Matrix3x1 CConvertMatrix::GetRGBYuvCoefs(AVColorSpace colspace)
{
  Matrix3x1 coefs;

  switch (colspace)
  {
    case AVCOL_SPC_BT709:
      coefs[0] = BT709YCbCr.Kr;
      coefs[1] = 1 - BT709YCbCr.Kr - BT709YCbCr.Kb;
      coefs[2] = BT709YCbCr.Kb;
      break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      coefs[0] = BT601YCbCr.Kr;
      coefs[1] = 1 - BT601YCbCr.Kr - BT601YCbCr.Kb;
      coefs[2] = BT601YCbCr.Kb;
      break;
    case AVCOL_SPC_SMPTE240M:
      coefs[0] = ST240YCbCr.Kr;
      coefs[1] = 1 - ST240YCbCr.Kr - ST240YCbCr.Kb;
      coefs[2] = ST240YCbCr.Kb;
      break;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
      coefs[0] = BT2020YCbCr.Kr;
      coefs[1] = 1 - BT2020YCbCr.Kr - BT2020YCbCr.Kb;
      coefs[2] = BT2020YCbCr.Kb;
      break;
    default:
      throw std::invalid_argument("unknown colorspace: " + std::to_string(colspace));
  }

  return coefs;
}
