// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CVideoSettings
{
	public:
		CVideoSettings();
		~CVideoSettings() {};

		bool operator!=(const CVideoSettings &right) const;

		bool			m_NoCache;
		bool      m_NonInterleaved;
		bool			m_Deinterlace;
		int				m_FilmGrain;
		int				m_ViewMode;			// current view mode
		float			m_CustomZoomAmount;	// custom setting zoom amount
		float			m_CustomPixelRatio;	// custom setting pixel ratio
		int       m_AudioStream;
		int       m_SubtitleStream;
		float			m_SubtitleDelay;
		bool			m_SubtitleOn;
		int				m_Brightness;
		int				m_Contrast;
		int				m_Gamma;
		bool			m_AdjustFrameRate;
		float			m_AudioDelay;
    int       m_ResumeTime;
private:
};

#endif // !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
