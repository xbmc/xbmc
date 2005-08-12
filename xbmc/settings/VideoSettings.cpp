// VideoSettings.cpp: implementation of the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "VideoSettings.h"
#include "GraphicContext.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVideoSettings::CVideoSettings()
{
  m_NoCache = false;
  m_NonInterleaved = false;
  m_InterlaceMethod = VS_INTERLACEMETHOD_AUTO;
  m_FilmGrain = 0;
  m_ViewMode = VIEW_MODE_NORMAL;
  m_CustomZoomAmount = 1.0f;
  m_CustomPixelRatio = 1.0f;
  m_AudioStream = -1;
  m_SubtitleStream = -1;
  m_SubtitleDelay = 0.0f;
  m_SubtitleOn = true;
  m_Brightness = 50;
  m_Contrast = 50;
  m_Gamma = 20;
  m_AdjustFrameRate = false;
  m_AudioDelay = 0.0f;
  m_ResumeTime = 0;
  m_Crop = false;
  m_CropTop = 0;
  m_CropBottom = 0;
  m_CropLeft = 0;
  m_CropRight = 0;
}

bool CVideoSettings::operator!=(const CVideoSettings &right) const
{
  if (m_NoCache != right.m_NoCache) return true;
  if (m_NonInterleaved != right.m_NonInterleaved) return true;
  if (m_InterlaceMethod != right.m_InterlaceMethod) return true;
  if (m_FilmGrain != right.m_FilmGrain) return true;
  if (m_ViewMode != right.m_ViewMode) return true;
  if (m_CustomZoomAmount != right.m_CustomZoomAmount) return true;
  if (m_CustomPixelRatio != right.m_CustomPixelRatio) return true;
  if (m_AudioStream != right.m_AudioStream) return true;
  if (m_SubtitleStream != right.m_SubtitleStream) return true;
  if (m_SubtitleDelay != right.m_SubtitleDelay) return true;
  if (m_SubtitleOn != right.m_SubtitleOn) return true;
  if (m_Brightness != right.m_Brightness) return true;
  if (m_Contrast != right.m_Contrast) return true;
  if (m_Gamma != right.m_Gamma) return true;
  if (m_AdjustFrameRate != right.m_AdjustFrameRate) return true;
  if (m_AudioDelay != right.m_AudioDelay) return true;
  if (m_ResumeTime != right.m_ResumeTime) return true;
  if (m_Crop != right.m_Crop) return true;
  if (m_CropTop != right.m_CropTop) return true;
  if (m_CropBottom != right.m_CropBottom) return true;
  if (m_CropLeft != right.m_CropLeft) return true;
  if (m_CropRight != right.m_CropRight) return true;
  return false;
}

EINTERLACEMETHOD CVideoSettings::GetInterlaceMethod()
{
  if( m_InterlaceMethod == VS_INTERLACEMETHOD_AUTO )
  {
    int mResolution = g_graphicsContext.GetVideoResolution();
    if( mResolution == HDTV_480p_16x9 || mResolution == HDTV_480p_4x3 || mResolution == HDTV_720p )
      return VS_INTERLACEMETHOD_DEINTERLACE_AUTO;
    else
      return VS_INTERLACEMETHOD_SYNC_AUTO;
  }
  else
    return m_InterlaceMethod;
}
