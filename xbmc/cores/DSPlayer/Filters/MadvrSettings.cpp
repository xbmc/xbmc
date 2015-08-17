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
// MadvrSettings.cpp: implementation of the CMadvrSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "MadvrSettings.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMadvrSettings::CMadvrSettings()
{
  m_Resolution = -1;

  m_ChromaUpscaling = MADVR_SCALING_BICUBIC_75;  
  m_ChromaAntiRing = false;
  m_ChromaSuperRes = false;
  
  m_ImageUpscaling = MADVR_SCALING_LANCZOS_3;
  m_ImageUpAntiRing = false;
  m_ImageUpLinear = false;

  m_ImageDownscaling = MADVR_SCALING_CATMULL_ROM;
  m_ImageDownAntiRing = false;
  m_ImageDownLinear = false;

  m_ImageDoubleLuma = -1;
  m_ImageDoubleChroma = -1;
  m_ImageQuadrupleLuma = -1;
  m_ImageQuadrupleChroma = -1;

  m_ImageDoubleLumaFactor = MADVR_DOUBLE_FACTOR_1_5;
  m_ImageDoubleChromaFactor = MADVR_DOUBLE_FACTOR_1_5;
  m_ImageQuadrupleLumaFactor = MADVR_QUADRUPLE_FACTOR_3_0;
  m_ImageQuadrupleChromaFactor = MADVR_QUADRUPLE_FACTOR_3_0;

  m_deintactive = MADVR_DEINT_IFDOUBT_DEACTIVE;
  m_deintforce = MADVR_DEINT_FORCE_AUTO;
  m_deintlookpixels = true;

  m_smoothMotion = -1;

  m_dithering = MADVR_DITHERING_ORDERED;
  m_ditheringColoredNoise = true;
  m_ditheringEveryFrame = true;

  m_deband = false;
  m_debandLevel = MADVR_DEBAND_LOW;
  m_debandFadeLevel = MADVR_DEBAND_HIGH;
}

bool CMadvrSettings::operator!=(const CMadvrSettings &right) const
{
  if (m_ChromaUpscaling != right.m_ChromaUpscaling) return true;
  if (m_ChromaAntiRing != right.m_ChromaAntiRing) return true;
  if (m_ChromaSuperRes != right.m_ChromaSuperRes) return true;

  if (m_ImageUpscaling != right.m_ImageUpscaling) return true;
  if (m_ImageUpAntiRing != right.m_ImageUpAntiRing) return true;
  if (m_ImageUpLinear != right.m_ImageUpLinear) return true;

  if (m_ImageDownscaling != right.m_ImageDownscaling) return true;
  if (m_ImageDownAntiRing != right.m_ImageDownAntiRing) return true;
  if (m_ImageDownLinear != right.m_ImageDownLinear) return true;

  if (m_ImageDoubleLuma != right.m_ImageDoubleLuma) return true;
  if (m_ImageDoubleChroma != right.m_ImageDoubleChroma) return true;
  if (m_ImageQuadrupleLuma != right.m_ImageQuadrupleLuma) return true;
  if (m_ImageQuadrupleChroma != right.m_ImageQuadrupleChroma) return true;

  if (m_ImageDoubleLumaFactor != right.m_ImageDoubleLumaFactor) return true;
  if (m_ImageDoubleChromaFactor != right.m_ImageDoubleChromaFactor) return true;
  if (m_ImageQuadrupleLumaFactor != right.m_ImageQuadrupleLumaFactor) return true;
  if (m_ImageQuadrupleChromaFactor != right.m_ImageQuadrupleChromaFactor) return true;

  if (m_deintactive != right.m_deintactive) return true;
  if (m_deintforce != right.m_deintforce) return true;
  if (m_deintlookpixels != right.m_deintlookpixels) return true;

  if (m_smoothMotion != right.m_smoothMotion) return true;

  if (m_dithering != right.m_dithering) return true;
  if (m_ditheringColoredNoise != right.m_ditheringColoredNoise) return true;
  if (m_ditheringEveryFrame != right.m_ditheringEveryFrame) return true;

  if (m_deband != right.m_deband) return true;
  if (m_debandLevel != right.m_debandLevel) return true;
  if (m_debandFadeLevel != right.m_debandFadeLevel) return true;

  return false;
}
