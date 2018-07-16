/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ConversionMatrix.h"

//------------------------------------------------------------------------------
// constants for primaries and transfers functions and color models
//------------------------------------------------------------------------------

// source: https://www.khronos.org/registry/DataFormat/specs/1.2/dataformat.1.2.html#PRIMARY_CONVERSION

struct ConvYCbCr
{
  float Kr, Kb;
};

struct Primaries
{
  float primaries[3][2];
  float whitepoint[2];
};

const ConvYCbCr BT709YCbCr = {0.2126, 0.0722};
const ConvYCbCr BT601YCbCr = {0.299, 0.114};
const ConvYCbCr BT2020YCbCr = {0.2627, 0.0593};
const ConvYCbCr ST240YCbCr = {0.212, 0.087};

const Primaries PrimariesBT709 = {{{0.640, 0.330}, {0.300, 0.600}, {0.150, 0.060}},
  {0.3127, 0.3290} };
const Primaries PrimariesBT610_525 = {{{0.640, 0.340}, {0.310, 0.595}, {0.155, 0.070}},
  {0.3127, 0.3290} };
const Primaries PrimariesBT610_625 = {{{0.640, 0.330}, {0.290, 0.600}, {0.150, 0.060}},
  {0.3127, 0.3290} };
const Primaries PrimariesBT2020 = {{{0.708, 0.292}, {0.170, 0.797}, {0.131, 0.046}},
  {0.3127, 0.3290} };

//------------------------------------------------------------------------------
// Matrix helpers
//------------------------------------------------------------------------------
// source: http://timjones.io/blog/archive/2014/10/20/the-matrix-inverted

template <unsigned Order>
float CalculateDeterminant(float (&src)[Order][Order]);

template <unsigned Order>
void GetSubmatrix(float (&dest)[Order-1][Order-1], float (&src)[Order][Order], unsigned row, unsigned col)
{
  unsigned colCount = 0;
  unsigned rowCount = 0;

  for (unsigned i = 0; i < Order; i++)
  {
    if (i != row)
    {
      colCount = 0;
      for (unsigned j = 0; j < Order; j++)
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
}

template <unsigned Order>
float CalculateMinor(float (&src)[Order][Order], unsigned row, unsigned col)
{
  float sub[Order-1][Order-1];
  GetSubmatrix<Order>(sub, src, row, col);
  return CalculateDeterminant<Order - 1>(sub);
}

template <unsigned Order>
float CalculateDeterminant(float (&src)[Order][Order])
{
  float det = 0.0f;

  for (unsigned i = 0; i < Order; i++)
  {
    // Get minor of element (0, i)
    float minor = CalculateMinor<Order>(src, 0, i);

    // If this is an odd-numbered row, negate the value.
    float factor = (i % 2 == 1) ? -1.0f : 1.0f;

    det += factor * src[0][i] * minor;
  }
  return det;
}

template <>
float CalculateDeterminant<2>(float (&src)[2][2])
{
  return src[0][0] * src[1][1] - src[0][1] * src[1][0];
}

//------------------------------------------------------------------------------
// Matrix classes
//------------------------------------------------------------------------------

template <unsigned Order>
CMatrix<Order>::CMatrix(float (&src)[Order][Order])
{
  Copy(m_mat, src);
}

template <unsigned Order>
CMatrix<Order>::CMatrix(float (&src)[Order-1][Order-1])
{
  *this = src;
}

template <unsigned Order>
CMatrix<Order>& CMatrix<Order>::operator=(const CMatrix& src)
{
  Copy(m_mat, src.m_mat);
  return *this;
}

template <unsigned Order>
CMatrix<Order>& CMatrix<Order>::operator=(const float (&src)[Order-1][Order-1])
{
  for (unsigned i=0; i<Order-1; ++i)
    for (unsigned j=0; j<Order-1; ++j)
      m_mat[i][j] = src[i][j];

  for (unsigned i=0; i<Order; ++i)
    m_mat[i][Order-1] = 0;

  for (unsigned i=0; i<Order; ++i)
    m_mat[Order-1][i] = 0;

  return *this;
}

template <unsigned Order>
float (&CMatrix<Order>::Get())[Order][Order]
{
  return m_mat;
}

template <unsigned Order>
CMatrix<Order> CMatrix<Order>::Invert()
{
  CMatrix<Order> ret;
  Invert(ret.m_mat, m_mat);
  return ret;
}

template <unsigned Order>
CMatrix<Order> CMatrix<Order>::operator*(const CMatrix& other)
{
  return *this * other.m_mat;
}

template <unsigned Order>
CMatrix<Order> CMatrix<Order>::operator*=(const CMatrix& other)
{
  CMatrix<Order> tmp = *this * other.m_mat;
  *this = tmp;
  return *this;
}

template <unsigned Order>
CMatrix<Order> CMatrix<Order>::operator*(const float (&other)[Order][Order])
{
  CMatrix<Order> ret;
  for (unsigned i=0; i<Order; ++i)
    for (unsigned j=0; j<Order; ++j)
      for (unsigned k=0; k<Order; ++k)
        ret.m_mat[i][j] += m_mat[i][k] * other[k][j];

  return ret;
}

template <unsigned Order>
void CMatrix<Order>::Copy(float (&dst)[Order][Order], const float (&src)[Order][Order])
{
  for (unsigned i=0; i<Order; ++i)
    for (unsigned j=0; j<Order; ++j)
      dst[i][j] = src[i][j];
}

template <unsigned Order>
void CMatrix<Order>::Invert(float (&dst)[Order][Order], float (&src)[Order][Order])
{
  // Calculate the inverse of the determinant of src.
  float det = CalculateDeterminant<Order>(src);
  float inverseDet = 1.0f / det;

  for (unsigned j = 0; j < Order; j++)
  {
    for (unsigned i = 0; i < Order; i++)
    {
      // Get minor of element (j, i) - not (i, j) because
      // this is where the transpose happens.
      float minor = CalculateMinor<Order>(src, j, i);

      // Multiply by (−1)^{i+j}
      float factor = ((i + j) % 2 == 1) ? -1.0f : 1.0f;
      float cofactor = minor * factor;

      dst[i][j] = inverseDet * cofactor;
    }
  }
}

CGlMatrix::CGlMatrix(float (&src)[3][3]) : CMatrix<4>(src)
{

}

CGlMatrix::CMatrix CGlMatrix::operator*(const float (&other)[4][4])
{
  CGlMatrix ret;

  float (&left)[4][4] = m_mat;
  const float (&right)[4][4] = other;

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

  CMatrix<3> inv(m_mat);
  Copy(m_mat, inv.Invert().Get());
};

ConversionToRGB& ConversionToRGB::operator=(const float (&src)[3][3])
{
  Copy(m_mat, src);
  return *this;
}

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
  return 1.0 - Gy - By;
}

PrimaryToRGB::PrimaryToRGB(float (&primaries)[3][2], float (&whitepoint)[2]) : PrimaryToXYZ(primaries, whitepoint)
{
  CMatrix<3> inv(m_mat);
  Copy(m_mat, inv.Invert().Get());
}

//------------------------------------------------------------------------------

void CConvertMatrix::SetColParams(AVColorSpace colSpace, int bits, bool limited, int textuteBits)
{
  if (m_colSpace == colSpace &&
      m_srcBits == bits &&
      m_limitedSrc == limited &&
      m_srcTextureBits == textuteBits &&
      m_pMat)
    return;

  m_colSpace = colSpace;
  m_srcBits = bits;
  m_limitedSrc = limited;
  m_srcTextureBits = textuteBits;

  GenMat();
}

void CConvertMatrix::SetColPrimaries(AVColorPrimaries dst, AVColorPrimaries src)
{
  m_colPrimariesDst = dst;
  m_colPrimariesSrc = src;

  if (m_colPrimariesDst != m_colPrimariesSrc)
  {
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

    CMatrix<3> tmp = toRGB*toXYZ;
    m_pMatPrim.reset(new CMatrix<3>(tmp));
  }
  else
  {
    m_pMatPrim.reset();
  }
}

void CConvertMatrix::SetParams(float contrast, float black, bool limited)
{
  m_contrast = contrast;
  m_black = black;
  m_limitedDst = limited;
}

void CConvertMatrix::GenMat()
{
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

  m_pMat.reset(new CGlMatrix(mConvRGB.Get()));

  CTranslate trans(0, -0.5, -0.5);
  *m_pMat *= trans;

  if (m_limitedSrc)
  {
    if (m_srcBits >= 12)
    {
      CScale scale(4080.0f / (3760 - 256), 4080.0f / (3840 - 256), 4080.0f / (3840 - 256));
      CTranslate trans(- 256.0f / 4080, - 256.0f / 4080, - 256.0f / 4080);
      *m_pMat *= scale;
      *m_pMat *= trans;
    }
    else if (m_srcBits == 10)
    {
      CScale scale(1020.0f / (940 - 64), 1020.0f / (960 - 64), 1020.0f / (960 - 64));
      CTranslate trans(- 64.0f / 1020, - 64.0f / 1020, - 64.0f / 1020);
      *m_pMat *= scale;
      *m_pMat *= trans;
    }
    else
    {
      CScale scale(255.0f / (235 - 16), 255.0f / (240 - 16), 255.0f / (240 - 16));
      CTranslate trans(- 16.0f / 255, - 16.0f / 255, - 16.0f / 255);
      *m_pMat *= scale;
      *m_pMat *= trans;
    }
  }

  if (m_srcTextureBits > 8)
  {
    float val = 65535.0f / ((1 << m_srcTextureBits) - 1);
    CScale scale(val, val, val);
    *m_pMat *= scale;
  }
}

void CConvertMatrix::GetYuvMat(float (&mat)[4][4])
{
  if (!m_pMat)
    return;

  CScale contrast(m_contrast, m_contrast, m_contrast);
  CTranslate black(m_black, m_black, m_black);

  CGlMatrix ret = contrast;
  ret *= black;
  if (m_limitedDst)
  {
    float valScale = (235 - 16) / 255.0f;
    float valTrans = 16.0f / 255;
    CScale scale(valScale, valScale, valScale);
    CTranslate trans(valTrans, valTrans, valTrans);
    ret *= trans;
    ret *= scale;
  }

  ret *= m_pMat->Get();
  float (&src)[4][4] = ret.Get();

  for (int i=0; i<4; ++i)
    for (int j=0; j<4; ++j)
      mat[i][j] = src[j][i];

  mat[0][3] = 0.0f;
  mat[1][3] = 0.0f;
  mat[2][3] = 0.0f;
  mat[3][3] = 1.0f;
}

bool CConvertMatrix::GetPrimMat(float (&mat)[3][3])
{
  if (!m_pMatPrim)
    return false;

  float (&src)[3][3] = m_pMatPrim->Get();

  for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
      mat[i][j] = src[j][i];

  return true;
}

float CConvertMatrix::GetGammaSrc()
{
  return m_gammaSrc;
}

float CConvertMatrix::GetGammaDst()
{
  return m_gammaDst;
}

bool CConvertMatrix::GetRGBYuvCoefs(AVColorSpace colspace, float (&coefs)[3])
{
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
      return false;
  }
  return true;
}
