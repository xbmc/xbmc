/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSettings.h"

#include "threads/CriticalSection.h"

#include <mutex>

CVideoSettings::CVideoSettings()
{
  m_InterlaceMethod = VS_INTERLACEMETHOD_AUTO;
  m_ScalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_ViewMode = ViewModeNormal;
  m_CustomZoomAmount = 1.0f;
  m_CustomPixelRatio = 1.0f;
  m_CustomVerticalShift = 0.0f;
  m_CustomNonLinStretch = false;
  m_AudioStream = -1;
  m_SubtitleStream = -1;
  m_SubtitleDelay = 0.0f;
  m_subtitleVerticalPosition = 0;
  m_subtitleVerticalPositionSave = false;
  m_SubtitleOn = true;
  m_Brightness = 50.0f;
  m_Contrast = 50.0f;
  m_Gamma = 20.0f;
  m_Sharpness = 0.0f;
  m_NoiseReduction = 0;
  m_PostProcess = false;
  m_VolumeAmplification = 0;
  m_AudioDelay = 0.0f;
  m_ResumeTime = 0;
  m_StereoMode = 0;
  m_StereoInvert = false;
  m_VideoStream = -1;
  m_ToneMapMethod = VS_TONEMAPMETHOD_OFF;
  m_ToneMapParam = 1.0f;
  m_Orientation = 0;
  m_CenterMixLevel = 0;
}

bool CVideoSettings::operator!=(const CVideoSettings &right) const
{
  if (m_InterlaceMethod != right.m_InterlaceMethod) return true;
  if (m_ScalingMethod != right.m_ScalingMethod) return true;
  if (m_ViewMode != right.m_ViewMode) return true;
  if (m_CustomZoomAmount != right.m_CustomZoomAmount) return true;
  if (m_CustomPixelRatio != right.m_CustomPixelRatio) return true;
  if (m_CustomVerticalShift != right.m_CustomVerticalShift) return true;
  if (m_CustomNonLinStretch != right.m_CustomNonLinStretch) return true;
  if (m_AudioStream != right.m_AudioStream) return true;
  if (m_SubtitleStream != right.m_SubtitleStream) return true;
  if (m_SubtitleDelay != right.m_SubtitleDelay) return true;
  if (m_subtitleVerticalPosition != right.m_subtitleVerticalPosition)
    return true;
  if (m_subtitleVerticalPositionSave != right.m_subtitleVerticalPositionSave)
    return true;
  if (m_SubtitleOn != right.m_SubtitleOn) return true;
  if (m_Brightness != right.m_Brightness) return true;
  if (m_Contrast != right.m_Contrast) return true;
  if (m_Gamma != right.m_Gamma) return true;
  if (m_Sharpness != right.m_Sharpness) return true;
  if (m_NoiseReduction != right.m_NoiseReduction) return true;
  if (m_PostProcess != right.m_PostProcess) return true;
  if (m_VolumeAmplification != right.m_VolumeAmplification) return true;
  if (m_AudioDelay != right.m_AudioDelay) return true;
  if (m_ResumeTime != right.m_ResumeTime) return true;
  if (m_StereoMode != right.m_StereoMode) return true;
  if (m_StereoInvert != right.m_StereoInvert) return true;
  if (m_VideoStream != right.m_VideoStream) return true;
  if (m_ToneMapMethod != right.m_ToneMapMethod) return true;
  if (m_ToneMapParam != right.m_ToneMapParam) return true;
  if (m_Orientation != right.m_Orientation) return true;
  if (m_CenterMixLevel != right.m_CenterMixLevel) return true;
  return false;
}

//------------------------------------------------------------------------------
// CVideoSettingsLocked
//------------------------------------------------------------------------------
CVideoSettingsLocked::CVideoSettingsLocked(CVideoSettings &vs, CCriticalSection &critSection) :
  m_videoSettings(vs), m_critSection(critSection)
{
}

void CVideoSettingsLocked::SetSubtitleStream(int stream)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_SubtitleStream = stream;
}

void CVideoSettingsLocked::SetSubtitleVisible(bool visible)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_SubtitleOn = visible;
}

void CVideoSettingsLocked::SetAudioStream(int stream)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_AudioStream = stream;
}

void CVideoSettingsLocked::SetVideoStream(int stream)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_VideoStream = stream;
}

void CVideoSettingsLocked::SetAudioDelay(float delay)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_AudioDelay = delay;
}

void CVideoSettingsLocked::SetSubtitleDelay(float delay)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_SubtitleDelay = delay;
}

void CVideoSettingsLocked::SetSubtitleVerticalPosition(int value, bool save)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_subtitleVerticalPosition = value;
  m_videoSettings.m_subtitleVerticalPositionSave = save;
}

void CVideoSettingsLocked::SetViewMode(int mode, float zoom, float par, float shift, bool stretch)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_ViewMode = mode;
  m_videoSettings.m_CustomZoomAmount = zoom;
  m_videoSettings.m_CustomPixelRatio = par;
  m_videoSettings.m_CustomVerticalShift = shift;
  m_videoSettings.m_CustomNonLinStretch = stretch;
}

void CVideoSettingsLocked::SetVolumeAmplification(float amp)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_videoSettings.m_VolumeAmplification = amp;
}
