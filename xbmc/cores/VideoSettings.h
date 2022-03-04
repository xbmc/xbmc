/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE=0,
  VS_INTERLACEMETHOD_AUTO=1,
  VS_INTERLACEMETHOD_RENDER_BLEND=2,
  VS_INTERLACEMETHOD_RENDER_WEAVE=4,
  VS_INTERLACEMETHOD_RENDER_BOB=6,
  VS_INTERLACEMETHOD_DEINTERLACE=7,
  VS_INTERLACEMETHOD_VDPAU_BOB=8,
  VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE=11,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL=12,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF=13,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL=14,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF=15,
  VS_INTERLACEMETHOD_DEINTERLACE_HALF=16,
  VS_INTERLACEMETHOD_VAAPI_BOB = 22,
  VS_INTERLACEMETHOD_VAAPI_MADI = 23,
  VS_INTERLACEMETHOD_VAAPI_MACI = 24,
  VS_INTERLACEMETHOD_DXVA_AUTO = 32,
  VS_INTERLACEMETHOD_MAX // do not use and keep as last enum value.
};

enum ESCALINGMETHOD
{
  VS_SCALINGMETHOD_NEAREST=0,
  VS_SCALINGMETHOD_LINEAR,
  VS_SCALINGMETHOD_CUBIC_B_SPLINE,
  VS_SCALINGMETHOD_CUBIC_MITCHELL,
  VS_SCALINGMETHOD_CUBIC_CATMULL,
  VS_SCALINGMETHOD_CUBIC_0_075,
  VS_SCALINGMETHOD_CUBIC_0_1,
  VS_SCALINGMETHOD_LANCZOS2,
  VS_SCALINGMETHOD_LANCZOS3_FAST,
  VS_SCALINGMETHOD_LANCZOS3,
  VS_SCALINGMETHOD_SINC8,
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

enum ETONEMAPMETHOD
{
  VS_TONEMAPMETHOD_OFF = 0,
  VS_TONEMAPMETHOD_REINHARD = 1,
  VS_TONEMAPMETHOD_ACES = 2,
  VS_TONEMAPMETHOD_HABLE = 3,
  VS_TONEMAPMETHOD_MAX
};

enum ViewMode
{
  ViewModeNormal = 0,
  ViewModeZoom,
  ViewModeStretch4x3,
  ViewModeWideZoom,
  ViewModeStretch16x9,
  ViewModeOriginal,
  ViewModeCustom,
  ViewModeStretch16x9Nonlin,
  ViewModeZoom120Width,
  ViewModeZoom110Width
};

class CVideoSettings
{
public:
  CVideoSettings();
  ~CVideoSettings() = default;

  bool operator!=(const CVideoSettings &right) const;

  EINTERLACEMETHOD m_InterlaceMethod;
  ESCALINGMETHOD m_ScalingMethod;
  int m_ViewMode; // current view mode
  float m_CustomZoomAmount; // custom setting zoom amount
  float m_CustomPixelRatio; // custom setting pixel ratio
  float m_CustomVerticalShift; // custom setting vertical shift
  bool  m_CustomNonLinStretch;
  int m_AudioStream;
  float m_VolumeAmplification;
  int m_SubtitleStream;
  float m_SubtitleDelay;
  bool m_SubtitleOn;
  float m_Brightness;
  float m_Contrast;
  float m_Gamma;
  float m_NoiseReduction;
  bool m_PostProcess;
  float m_Sharpness;
  float m_AudioDelay;
  int m_ResumeTime;
  int m_StereoMode;
  bool m_StereoInvert;
  int m_VideoStream;
  ETONEMAPMETHOD m_ToneMapMethod;
  float m_ToneMapParam;
  int m_Orientation;
  int m_CenterMixLevel; // relative to metadata or default
};

class CCriticalSection;
class CVideoSettingsLocked
{
public:
  CVideoSettingsLocked(CVideoSettings &vs, CCriticalSection &critSection);
  virtual ~CVideoSettingsLocked() = default;

  CVideoSettingsLocked(CVideoSettingsLocked const &) = delete;
  void operator=(CVideoSettingsLocked const &x) = delete;

  void SetSubtitleStream(int stream);
  void SetSubtitleVisible(bool visible);
  void SetAudioStream(int stream);
  void SetVideoStream(int stream);
  void SetAudioDelay(float delay);
  void SetSubtitleDelay(float delay);
  void SetViewMode(int mode, float zoom, float par, float shift, bool stretch);
  void SetVolumeAmplification(float amp);

protected:
  CVideoSettings &m_videoSettings;
  CCriticalSection &m_critSection;
};
