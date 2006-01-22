// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE=0,
  VS_INTERLACEMETHOD_AUTO=1,
  VS_INTERLACEMETHOD_DEINTERLACE=2,
  VS_INTERLACEMETHOD_SYNC_ODD=3,
  VS_INTERLACEMETHOD_SYNC_EVEN=4,

  //These are generated from teh above based on current video mode
  //They should not be user selectable
  VS_INTERLACEMETHOD_DEINTERLACE_AUTO=5, 
  VS_INTERLACEMETHOD_SYNC_AUTO=6,
};


class CVideoSettings
{
public:
  CVideoSettings();
  ~CVideoSettings() {};

  bool operator!=(const CVideoSettings &right) const;

  bool m_NoCache;
  bool m_NonInterleaved;
  bool m_bForceIndex;
  EINTERLACEMETHOD m_InterlaceMethod;
  int m_FilmGrain;
  int m_ViewMode;   // current view mode
  float m_CustomZoomAmount; // custom setting zoom amount
  float m_CustomPixelRatio; // custom setting pixel ratio
  int m_AudioStream;
  float m_VolumeAmplification;
  int m_SubtitleStream;
  float m_SubtitleDelay;
  bool m_SubtitleOn;
  int m_Brightness;
  int m_Contrast;
  int m_Gamma;
  float m_AudioDelay;
  int m_ResumeTime;
  bool m_Crop;
  int m_CropTop;
  int m_CropBottom;
  int m_CropLeft;
  int m_CropRight;

  EINTERLACEMETHOD GetInterlaceMethod();

private:
};

#endif // !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
