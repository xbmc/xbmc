// VideoSettings.cpp: implementation of the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "VideoSettings.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVideoSettings::CVideoSettings()
{
	m_NoCache=false;
	m_NonInterleaved = false;
	m_Deinterlace = false;
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
}

bool CVideoSettings::operator!=(const CVideoSettings &right) const
{
	if (m_NoCache != right.m_NoCache) return true;
	if (m_NonInterleaved != right.m_NonInterleaved) return true;
	if (m_Deinterlace != right.m_Deinterlace) return true;
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
	return false;
}
