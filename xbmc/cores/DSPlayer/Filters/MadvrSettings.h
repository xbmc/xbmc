/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
// MadvrSettings.h: interface for the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

//#if !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
//#define AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#pragma once

enum MADVR_SCALING
{
  MADVR_SCALING_NEAREST_NEIGHBOR,
  MADVR_SCALING_BILINEAR,
  MADVR_SCALING_DXVA2,
  MADVR_SCALING_MITCHEL_NETRAVALI,
  MADVR_SCALING_CATMULL_ROM,
  MADVR_SCALING_BICUBIC_50,
  MADVR_SCALING_BICUBIC_60,
  MADVR_SCALING_BICUBIC_75,
  MADVR_SCALING_BICUBIC_100,
  MADVR_SCALING_SOFTCUBIC_50,
  MADVR_SCALING_SOFTCUBIC_60,
  MADVR_SCALING_SOFTCUBIC_70,
  MADVR_SCALING_SOFTCUBIC_80,
  MADVR_SCALING_SOFTCUBIC_100,
  MADVR_SCALING_LANCZOS_3,
  MADVR_SCALING_LANCZOS_4,
  MADVR_SCALING_LANCZOS_8,
  MADVR_SCALING_SPLINE_36,
  MADVR_SCALING_SPLINE_64,
  MADVR_SCALING_JINC_3,
  MADVR_SCALING_JINC_4,
  MADVR_SCALING_JINC_8,
  MADVR_SCALING_BILATERAL,
  MADVR_SCALING_SUPERXBR25,
  MADVR_SCALING_SUPERXBR50,
  MADVR_SCALING_SUPERXBR75,
  MADVR_SCALING_SUPERXBR100,
  MADVR_SCALING_SUPERXBR125,
  MADVR_SCALING_SUPERXBR150,
  MADVR_SCALING_NEDI,
  MADVR_SCALING_NNEDI3_16,
  MADVR_SCALING_NNEDI3_32,
  MADVR_SCALING_NNEDI3_64,
  MADVR_SCALING_NNEDI3_128,
  MADVR_SCALING_NNEDI3_256,
};

enum MADVR_NNEDI3_QUALITY
{
  MADVR_NNEDI3_16NEURONS,
  MADVR_NNEDI3_32NEURONS,
  MADVR_NNEDI3_64NEURONS,
  MADVR_NNEDI3_128NEURONS,
  MADVR_NNEDI3_256NEURONS
};

enum MADVR_DOUBLE_FACTOR
{
  MADVR_DOUBLE_FACTOR_2_0,
  MADVR_DOUBLE_FACTOR_1_5,
  MADVR_DOUBLE_FACTOR_1_2,
  MADVR_DOUBLE_FACTOR_ALWAYS
};

enum MADVR_QUADRUPLE_FACTOR
{
  MADVR_QUADRUPLE_FACTOR_4_0,
  MADVR_QUADRUPLE_FACTOR_3_0,
  MADVR_QUADRUPLE_FACTOR_2_4,
  MADVR_QUADRUPLE_FACTOR_ALWAYS
};

enum MADVR_DEINT_ACTIVE
{
  MADVR_DEINT_IFDOUBT_ACTIVE,
  MADVR_DEINT_IFDOUBT_DEACTIVE
};

enum MADVR_DEINT_FORCE
{
  MADVR_DEINT_FORCE_AUTO,
  MADVR_DEINT_FORCE_FILM,
  MADVR_DEINT_FORCE_VIDEO
};

enum MADVR_SMOOTHMOTION
{
  MADVR_SMOOTHMOTION_AVOIDJUDDER,
  MADVR_SMOOTHMOTION_ALMOSTALWAYS,
  MADVR_SMOOTHMOTION_ALWAYS
};

enum MADVR_DITHERING
{
  MADVR_DITHERING_RANDOM,
  MADVR_DITHERING_ORDERED,
  MADVR_DITHERING_ERRORD1,
  MADVR_DITHERING_ERRORD2
};

enum MADVR_DEBAND
{
  MADVR_DEBAND_LOW,
  MADVR_DEBAND_MEDIUM,
  MADVR_DEBAND_HIGH
};

enum MADVR_RES_SETTINGS
{
  MADVR_RES_SD,
  MADVR_RES_720,
  MADVR_RES_1080,
  MADVR_RES_2160,
  MADVR_RES_ALL
};

class CMadvrSettings
{
public:
  CMadvrSettings();
  ~CMadvrSettings() {};

  bool operator!=(const CMadvrSettings &right) const;

  int m_Resolution;

  int m_ChromaUpscaling;
  bool m_ChromaAntiRing;
  bool m_ChromaSuperRes;

  int m_ImageUpscaling;
  bool m_ImageUpAntiRing;
  bool m_ImageUpLinear;

  int m_ImageDownscaling;
  bool m_ImageDownAntiRing;
  bool m_ImageDownLinear;

  int m_ImageDoubleLuma;
  int m_ImageDoubleChroma;
  int m_ImageQuadrupleLuma;
  int m_ImageQuadrupleChroma;

  int m_ImageDoubleLumaFactor;
  int m_ImageDoubleChromaFactor;
  int m_ImageQuadrupleLumaFactor;
  int m_ImageQuadrupleChromaFactor;

  int m_deintactive;
  int m_deintforce;
  bool m_deintlookpixels;

  int m_smoothMotion;

  int m_dithering;
  bool m_ditheringColoredNoise;
  bool m_ditheringEveryFrame;

  bool m_deband;
  int m_debandLevel;
  int m_debandFadeLevel;

private:
};

//#endif // !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
