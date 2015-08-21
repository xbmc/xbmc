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

  bool m_fineSharp;
  float m_fineSharpStrength;
  bool m_lumaSharpen;
  float m_lumaSharpenStrength;
  bool m_adaptiveSharpen;
  float m_adaptiveSharpenStrength;

  bool m_UpRefFineSharp;
  float m_UpRefFineSharpStrength;
  bool m_UpRefLumaSharpen;
  float m_UpRefLumaSharpenStrength;
  bool m_UpRefAdaptiveSharpen;
  float m_UpRefAdaptiveSharpenStrength;
  bool m_superRes;
  float m_superResStrength;

  bool m_refineOnce;
  bool m_superResFirst;

private:
};

//#endif // !defined(AFX_MADVRSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
