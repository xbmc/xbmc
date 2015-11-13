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
#include "DSRendererCallback.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

CMadvrSettings::CMadvrSettings()
{
  m_Resolution = -1;
  m_TvShowName = "NOTVSHOW_NULL";

  m_ChromaUpscaling = MADVR_DEFAULT_CHROMAUP;
  m_ChromaAntiRing = false;
  m_ChromaSuperRes = false;
  m_ChromaSuperResPasses = MADVR_DEFAULT_CHROMAUP_SUPERRESPASSES;
  m_ChromaSuperResStrength= MADVR_DEFAULT_CHROMAUP_SUPERRESSTRENGTH;
  m_ChromaSuperResSoftness = MADVR_DEFAULT_CHROMAUP_SUPERRESSOFTNESS;
  
  m_ImageUpscaling = MADVR_DEFAULT_LUMAUP;
  m_ImageUpAntiRing = false;
  m_ImageUpLinear = false;

  m_ImageDownscaling = MADVR_DEFAULT_LUMADOWN;
  m_ImageDownAntiRing = false;
  m_ImageDownLinear = false;

  m_ImageDoubleLuma = -1;
  m_ImageDoubleChroma = -1;
  m_ImageQuadrupleLuma = -1;
  m_ImageQuadrupleChroma = -1;

  m_ImageDoubleLumaFactor = MADVR_DEFAULT_DOUBLEFACTOR;
  m_ImageDoubleChromaFactor = MADVR_DEFAULT_DOUBLEFACTOR;
  m_ImageQuadrupleLumaFactor = MADVR_DEFAULT_QUADRUPLEFACTOR;
  m_ImageQuadrupleChromaFactor = MADVR_DEFAULT_QUADRUPLEFACTOR;

  m_deintactive = MADVR_DEFAULT_DEINTACTIVE;
  m_deintforce = MADVR_DEFAULT_DEINTFORCE;
  m_deintlookpixels = true;

  m_smoothMotion = -1;

  m_dithering = MADVR_DEFAULT_DITHERING;
  m_ditheringColoredNoise = true;
  m_ditheringEveryFrame = true;

  m_deband = false;
  m_debandLevel = MADVR_DEFAULT_DEBAND_LEVEL;
  m_debandFadeLevel = MADVR_DEFAULT_DEBAND_FADELEVEL;

  m_sharpenEdges = false;
  m_sharpenEdgesStrength = MADVR_DEFAULT_SHARPENEDGES;
  m_crispenEdges = false;
  m_crispenEdgesStrength = MADVR_DEFAULT_CRISPENEDGES;
  m_thinEdges = false;
  m_thinEdgesStrength = MADVR_DEFAULT_THINEDGES;
  m_enhanceDetail = false;
  m_enhanceDetailStrength = MADVR_DEFAULT_ENHANCEDETAIL;
  m_lumaSharpen = false;
  m_lumaSharpenStrength = MADVR_DEFAULT_LUMASHARPEN;
  m_adaptiveSharpen = false;
  m_adaptiveSharpenStrength = MADVR_DEFAULT_ADAPTIVESHARPEN;

  m_noSmallScaling = -1;
  m_moveSubs = 0;
  m_detectBars = false;
  m_arChange = -1;
  m_quickArChange = -1;
  m_shiftImage = -1;
  m_dontCropSubs = -1;
  m_cleanBorders = -1;
  m_reduceBigBars = -1;
  m_cropSmallBars = false;
  m_cropBars = false;


  m_UpRefSharpenEdges = false;
  m_UpRefSharpenEdgesStrength = MADVR_DEFAULT_UPSHARPENEDGES;
  m_UpRefCrispenEdges = false;
  m_UpRefCrispenEdgesStrength = MADVR_DEFAULT_UPCRISPENEDGES;
  m_UpRefThinEdges = false;
  m_UpRefThinEdgesStrength = MADVR_DEFAULT_UPTHINEDGES;
  m_UpRefEnhanceDetail = false;
  m_UpRefEnhanceDetailStrength = MADVR_DEFAULT_UPENHANCEDETAIL;
  m_UpRefLumaSharpen = false;
  m_UpRefLumaSharpenStrength = MADVR_DEFAULT_UPLUMASHARPEN;
  m_UpRefAdaptiveSharpen = false;
  m_UpRefAdaptiveSharpenStrength = MADVR_DEFAULT_UPADAPTIVESHARPEN;
  m_superRes = false;
  m_superResStrength = MADVR_DEFAULT_SUPERRES;
  m_superResLinear = false;

  m_refineOnce = false;
}

bool CMadvrSettings::operator!=(const CMadvrSettings &right) const
{
  if (m_ChromaUpscaling != right.m_ChromaUpscaling) return true;
  if (m_ChromaAntiRing != right.m_ChromaAntiRing) return true;
  if (m_ChromaSuperRes != right.m_ChromaSuperRes) return true;
  if (m_ChromaSuperResPasses != right.m_ChromaSuperResPasses) return true;
  if (m_ChromaSuperResStrength != right.m_ChromaSuperResStrength) return true;
  if (m_ChromaSuperResSoftness != right.m_ChromaSuperResSoftness) return true;

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

  if (m_sharpenEdges != right.m_sharpenEdges) return true;
  if (m_sharpenEdgesStrength != right.m_sharpenEdgesStrength) return true;
  if (m_crispenEdges != right.m_crispenEdges) return true;
  if (m_crispenEdgesStrength != right.m_crispenEdgesStrength) return true;
  if (m_thinEdges != right.m_thinEdges) return true;
  if (m_thinEdgesStrength != right.m_thinEdgesStrength) return true;
  if (m_enhanceDetail != right.m_enhanceDetail) return true;
  if (m_enhanceDetailStrength != right.m_enhanceDetailStrength) return true;
  if (m_lumaSharpen != right.m_lumaSharpen) return true;
  if (m_lumaSharpenStrength != right.m_lumaSharpenStrength) return true;
  if (m_adaptiveSharpen != right.m_adaptiveSharpen) return true;
  if (m_adaptiveSharpenStrength != right.m_adaptiveSharpenStrength) return true;

  if (m_noSmallScaling != right.m_noSmallScaling) return true;
  if (m_moveSubs != right.m_moveSubs) return true;
  if (m_detectBars != right.m_detectBars) return true;
  if (m_arChange != right.m_arChange) return true;
  if (m_quickArChange != right.m_quickArChange) return true;
  if (m_shiftImage != right.m_shiftImage) return true;
  if (m_dontCropSubs != right.m_dontCropSubs) return true;
  if (m_cleanBorders != right.m_cleanBorders) return true;
  if (m_reduceBigBars != right.m_reduceBigBars) return true;
  if (m_cropSmallBars != right.m_cropSmallBars) return true;
  if (m_cropBars != right.m_cropBars) return true;

  if (m_UpRefSharpenEdges != right.m_UpRefSharpenEdges) return true;
  if (m_UpRefSharpenEdgesStrength != right.m_UpRefSharpenEdgesStrength) return true;
  if (m_UpRefCrispenEdges != right.m_UpRefCrispenEdges) return true;
  if (m_UpRefCrispenEdgesStrength != right.m_UpRefCrispenEdgesStrength) return true;
  if (m_UpRefThinEdges != right.m_UpRefThinEdges) return true;
  if (m_UpRefThinEdgesStrength != right.m_UpRefThinEdgesStrength) return true;
  if (m_UpRefEnhanceDetail != right.m_UpRefEnhanceDetail) return true;
  if (m_UpRefEnhanceDetailStrength != right.m_UpRefEnhanceDetailStrength) return true;
  if (m_UpRefLumaSharpen != right.m_UpRefLumaSharpen) return true;
  if (m_UpRefLumaSharpenStrength != right.m_UpRefLumaSharpenStrength) return true;
  if (m_UpRefAdaptiveSharpen != right.m_UpRefAdaptiveSharpen) return true;
  if (m_UpRefAdaptiveSharpenStrength != right.m_UpRefAdaptiveSharpenStrength) return true;
  if (m_superRes != right.m_superRes) return true;
  if (m_superResStrength != right.m_superResStrength) return true;

  if (m_refineOnce != right.m_refineOnce) return true;


  return false;
}
