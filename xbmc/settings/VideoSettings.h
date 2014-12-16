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
// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#pragma once

enum EDEINTERLACEMODE
{
  VS_DEINTERLACEMODE_OFF=0,
  VS_DEINTERLACEMODE_AUTO=1,
  VS_DEINTERLACEMODE_FORCE=2
};

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE=0, // Legacy
  VS_INTERLACEMETHOD_AUTO=1,
  VS_INTERLACEMETHOD_RENDER_BLEND=2,

  VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED=3,
  VS_INTERLACEMETHOD_RENDER_WEAVE=4,
  
  VS_INTERLACEMETHOD_RENDER_BOB_INVERTED=5,
  VS_INTERLACEMETHOD_RENDER_BOB=6,

  VS_INTERLACEMETHOD_DEINTERLACE=7,

  VS_INTERLACEMETHOD_VDPAU_BOB=8,
  VS_INTERLACEMETHOD_INVERSE_TELECINE=9,

  VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE=11,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL=12,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF=13,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL=14,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF=15,
  VS_INTERLACEMETHOD_DEINTERLACE_HALF=16,
  VS_INTERLACEMETHOD_DXVA_BOB = 17,
  VS_INTERLACEMETHOD_DXVA_BEST = 18,
  // VS_INTERLACEMETHOD_DXVA_ANY = 19, Legacy

  VS_INTERLACEMETHOD_SW_BLEND = 20,
  VS_INTERLACEMETHOD_AUTO_ION = 21,

  VS_INTERLACEMETHOD_VAAPI_BOB = 22,
  VS_INTERLACEMETHOD_VAAPI_MADI = 23,
  VS_INTERLACEMETHOD_VAAPI_MACI = 24,

  VS_INTERLACEMETHOD_MMAL_ADVANCED = 25,
  VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF = 26,
  VS_INTERLACEMETHOD_MMAL_BOB = 27,
  VS_INTERLACEMETHOD_MMAL_BOB_HALF = 28,

  VS_INTERLACEMETHOD_MAX // do not use and keep as last enum value.
};

enum ESCALINGMETHOD
{
  VS_SCALINGMETHOD_NEAREST=0,
  VS_SCALINGMETHOD_LINEAR,

  VS_SCALINGMETHOD_CUBIC,
  VS_SCALINGMETHOD_LANCZOS2,
  VS_SCALINGMETHOD_LANCZOS3_FAST,
  VS_SCALINGMETHOD_LANCZOS3,
  VS_SCALINGMETHOD_SINC8,
  VS_SCALINGMETHOD_NEDI,
  
  VS_SCALINGMETHOD_BICUBIC_SOFTWARE,
  VS_SCALINGMETHOD_LANCZOS_SOFTWARE,
  VS_SCALINGMETHOD_SINC_SOFTWARE,
  VS_SCALINGMETHOD_VDPAU_HARDWARE,
  VS_SCALINGMETHOD_DXVA_HARDWARE,

  VS_SCALINGMETHOD_AUTO,

  VS_SCALINGMETHOD_SPLINE36_FAST,
  VS_SCALINGMETHOD_SPLINE36,

  VS_SCALINGMETHOD_MAX // do not use and keep as last enum value.
};

enum EDECODEMETHOD
{
  VS_DECODEMETHOD_SOFTWARE=0,
  VS_DECODEMETHOD_HARDWARE=1
};

typedef enum {
  ViewModeNormal      = 0,
  ViewModeZoom,
  ViewModeStretch4x3,
  ViewModeWideZoom,
  ViewModeStretch16x9,
  ViewModeOriginal,
  ViewModeCustom
} ViewMode;

class CVideoSettings
{
public:
  CVideoSettings();
  ~CVideoSettings() {};

  bool operator!=(const CVideoSettings &right) const;

  EDEINTERLACEMODE m_DeinterlaceMode;
  EINTERLACEMETHOD m_InterlaceMethod;
  ESCALINGMETHOD   m_ScalingMethod;
  int m_ViewMode;   // current view mode
  float m_CustomZoomAmount; // custom setting zoom amount
  float m_CustomPixelRatio; // custom setting pixel ratio
  float m_CustomVerticalShift; // custom setting vertical shift
  bool  m_CustomNonLinStretch;
  int m_AudioStream;
  float m_VolumeAmplification;
  bool m_OutputToAllSpeakers;
  int m_SubtitleStream;
  float m_SubtitleDelay;
  bool m_SubtitleOn;
  bool m_SubtitleCached;
  float m_Brightness;
  float m_Contrast;
  float m_Gamma;
  float m_NoiseReduction;
  bool m_PostProcess;
  float m_Sharpness;
  float m_AudioDelay;
  int m_ResumeTime;
  bool m_Crop;
  int m_CropTop;
  int m_CropBottom;
  int m_CropLeft;
  int m_CropRight;
  int m_StereoMode;
  bool m_StereoInvert;

private:
};

#endif // !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
