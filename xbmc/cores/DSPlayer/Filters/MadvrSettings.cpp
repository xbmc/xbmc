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

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

CMadvrSettings::CMadvrSettings()
{
  m_Resolution = -1;

  m_ChromaUpscaling = ChromaUpDef;
  m_ChromaAntiRing = false;
  m_ChromaSuperRes = false;
  
  m_ImageUpscaling = LumaUpDef;
  m_ImageUpAntiRing = false;
  m_ImageUpLinear = false;

  m_ImageDownscaling = LumaDownDef;
  m_ImageDownAntiRing = false;
  m_ImageDownLinear = false;

  m_ImageDoubleLuma = -1;
  m_ImageDoubleChroma = -1;
  m_ImageQuadrupleLuma = -1;
  m_ImageQuadrupleChroma = -1;

  m_ImageDoubleLumaFactor = MadvrDoubleFactorDef;
  m_ImageDoubleChromaFactor = MadvrDoubleFactorDef;
  m_ImageQuadrupleLumaFactor = MadvrQuadrupleFactorDef;
  m_ImageQuadrupleChromaFactor = MadvrQuadrupleFactorDef;

  m_deintactive = MadvrDeintActiveDef;
  m_deintforce = MadvrDeintForceDef;
  m_deintlookpixels = true;

  m_smoothMotion = -1;

  m_dithering = MadvrDitheringDef;
  m_ditheringColoredNoise = true;
  m_ditheringEveryFrame = true;

  m_deband = false;
  m_debandLevel = MadvrDebandLevelDef;
  m_debandFadeLevel = MadvrDebandFadeLevelDef;

  m_fineSharp = false;
  m_fineSharpStrength = 2.0f;
  m_lumaSharpen = false;
  m_lumaSharpenStrength = 0.65f;
  m_adaptiveSharpen = false;
  m_adaptiveSharpenStrength = 0.5f;

  m_UpRefFineSharp = false;
  m_UpRefFineSharpStrength = 2.0f;
  m_UpRefLumaSharpen = false;
  m_UpRefLumaSharpenStrength = 0.65f;
  m_UpRefAdaptiveSharpen = false;
  m_UpRefAdaptiveSharpenStrength = 0.5f;
  m_superRes = false;
  m_superResStrength = 1.0f;

  m_refineOnce = false;
  m_superResFirst = false;
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

  if (m_fineSharp != right.m_fineSharp) return true;
  if (m_fineSharpStrength != right.m_fineSharpStrength) return true;
  if (m_lumaSharpen != right.m_lumaSharpen) return true;
  if (m_lumaSharpenStrength != right.m_lumaSharpenStrength) return true;
  if (m_adaptiveSharpen != right.m_adaptiveSharpen) return true;
  if (m_adaptiveSharpenStrength != right.m_adaptiveSharpenStrength) return true;

  if (m_UpRefFineSharp != right.m_UpRefFineSharp) return true;
  if (m_UpRefFineSharpStrength != right.m_UpRefFineSharpStrength) return true;
  if (m_UpRefLumaSharpen != right.m_UpRefLumaSharpen) return true;
  if (m_UpRefLumaSharpenStrength != right.m_UpRefLumaSharpenStrength) return true;
  if (m_UpRefAdaptiveSharpen != right.m_UpRefAdaptiveSharpen) return true;
  if (m_UpRefAdaptiveSharpenStrength != right.m_UpRefAdaptiveSharpenStrength) return true;
  if (m_superRes != right.m_superRes) return true;
  if (m_superResStrength != right.m_superResStrength) return true;

  if (m_refineOnce != right.m_refineOnce) return true;
  if (m_superResFirst != right.m_superResFirst) return true;

  return false;
}



int CMadvrSettings::GetScalingId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrScaling); i++)
  {
    if (sValue == MadvrScaling[i].name)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetDeintForceId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrDeintForce); i++)
  {
    if (sValue == MadvrDeintForce[i].name)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetDoubleId(int iValue)
{
  for (unsigned int i = 0; i < countof(MadvrDoubleQuality); i++)
  {
    if (iValue == MadvrDoubleQuality[i].id && "NNEDI3" == MadvrDoubleQuality[i].algo)
      return i;
  }
  return -1;
}

int CMadvrSettings::GeDoubleAlgo(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrDoubleQuality); i++)
  {
    if (sValue == MadvrDoubleQuality[i].algo)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetDoubleFactorId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrDoubleFactor); i++)
  {
    if (sValue == MadvrDoubleFactor[i].name)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetQuadrupleFactorId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrQuadrupleFactor); i++)
  {
    if (sValue == MadvrQuadrupleFactor[i].name)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetSmoothMotionId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrSmoothMotion); i++)
  {
    if (sValue == MadvrSmoothMotion[i].name)
      return i;
  }
  return -1;
}

int CMadvrSettings::GetDitheringId(std::string sValue)
{
  for (unsigned int i = 0; i < countof(MadvrDithering); i++)
  {
    if (sValue == MadvrDithering[i].name)
      return i;
  }
  return -1;
}