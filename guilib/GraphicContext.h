/*!
	\file GraphicContext.h
	\brief 
	*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#include "IMsgSenderCallback.h"
#include "common/mouse.h"

/*!
	\ingroup graphics
	\brief 
	*/
enum RESOLUTION {
	INVALID = -1,
	HDTV_1080i = 0,
	HDTV_720p = 1,
	HDTV_480p_4x3 = 2,
	HDTV_480p_16x9 = 3,
	NTSC_4x3 = 4,
	NTSC_16x9 = 5,
	PAL_4x3 = 6,
	PAL_16x9 = 7,
	PAL60_4x3 = 8,
	PAL60_16x9 = 9,
  AUTORES = 10
};

/*!
	\ingroup graphics
	\brief 
	*/
struct OVERSCAN {
	int left;
	int top;
	int right;
	int bottom;
};

/*!
	\ingroup graphics
	\brief 
	*/
struct RESOLUTION_INFO	{
	OVERSCAN Overscan;
	int iWidth;
	int iHeight;
	int iSubtitles;
	int iOSDYOffset;	// Y offset for OSD (applied to all Y pos in skin)
	DWORD dwFlags;
	float fPixelRatio;
	char strMode[11];
};

/*!
	\ingroup graphics
	\brief 
	*/
class CGraphicContext
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);
	LPDIRECT3DDEVICE8			Get3DDevice() {  return m_pd3dDevice; }
  void									SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice);
//  void									GetD3DParameters(D3DPRESENT_PARAMETERS &params);
  void									SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams, RESOLUTION_INFO *pResInfo);
	int										GetWidth() const { return m_iScreenWidth; }
  int										GetHeight() const { return m_iScreenHeight; }
  void									SendMessage(CGUIMessage& message);
  void									setMessageSender(IMsgSenderCallback* pCallback);
  DWORD									GetNewID();
	const CStdString&     GetMediaDir() const { return m_strMediaDir; }
	void									SetMediaDir(const CStdString& strMediaDir) { m_strMediaDir=strMediaDir; }
	bool									IsWidescreen() const { return m_bWidescreen; }
	void									Correct(float& fCoordinateX, float& fCoordinateY) const;	
	void									Correct(int& iCoordinateX, int& iCoordinateY) const;	
	void									Scale(float& fCoordinateX, float& fCoordinateY, float& fWidth, float& fHeight) const;
	void									SetViewPort(float fx, float fy , float fwidth, float fheight);
	void									RestoreViewPort();
	const RECT&						GetViewWindow() const;
	void									SetViewWindow(const RECT&	rc) ;
  void									SetFullScreenViewWindow(RESOLUTION &res);
	void									SetFullScreenVideo(bool bOnOff); 
	bool									IsFullScreenVideo() const; 
	bool									IsCalibrating() const; 
	void									SetCalibrating(bool bOnOff);
  bool                  IsOverlayAllowed() const;
  void                  SetOverlay(bool bOnOff);
	void									SetGUIResolution(RESOLUTION &res);
	void									GetAllowedResolutions(vector<RESOLUTION> &res, bool bAllowPAL60 = false);
	bool									IsValidResolution(RESOLUTION res);
	void									SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE);
	RESOLUTION						GetVideoResolution() const;
	void									ResetScreenParameters(RESOLUTION res);
	void									SetOffset(int iXoffset, int iYoffset);
	void									Lock() { EnterCriticalSection(&m_critSection); }
	void									Unlock() { LeaveCriticalSection(&m_critSection); }
	void									EnablePreviewWindow(bool bEnable);
	void									ScalePosToScreenResolution(DWORD& x, DWORD&  y, RESOLUTION res);
	void									ScaleRectToScreenResolution(DWORD& left, DWORD&  top, DWORD& right, DWORD& bottom, RESOLUTION res);
	void									ScaleXCoord(DWORD &x, RESOLUTION res);
	void									ScaleXCoord(int &x, RESOLUTION res);
	void									ScaleXCoord(long &x, RESOLUTION res);
	void									ScaleYCoord(DWORD &x, RESOLUTION res);
	void									ScaleYCoord(int &x, RESOLUTION res);
	void									ScaleYCoord(long &x, RESOLUTION res);
	float									GetPixelRatio(RESOLUTION iRes) const;
	void									CaptureStateBlock();
	void									ApplyStateBlock();
  void                  Clear();

protected:
	CRITICAL_SECTION			  m_critSection;
  IMsgSenderCallback*     m_pCallback;
  LPDIRECT3DDEVICE8       m_pd3dDevice;
  D3DPRESENT_PARAMETERS*  m_pd3dParams;
  int                     m_iScreenHeight;
  int                     m_iScreenWidth;
  DWORD                   m_dwID;
	bool										m_bWidescreen;
  CStdString              m_strMediaDir;
	D3DVIEWPORT8						m_oldviewport;
	RECT										m_videoRect;
	bool										m_bFullScreenVideo;
	int											m_iScreenOffsetX;
	int											m_iScreenOffsetY;
	bool										m_bShowPreviewWindow;
	bool										m_bCalibrating;
  bool                    m_bShowOverlay;
	RESOLUTION									m_Resolution;
	RESOLUTION_INFO								*m_pResInfo;
	DWORD					m_stateBlock;
};

/*!
	\ingroup graphics
	\brief 
	*/
extern CGraphicContext g_graphicsContext;
#endif
