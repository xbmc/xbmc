#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once
#include "gui3d.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

#include "stdstring.h"
using namespace std;

class CGraphicContext
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);
  LPDIRECT3DDEVICE8			Get3DDevice();
  void									Set(LPDIRECT3DDEVICE8 p3dDevice, int iWidth, int iHeight, int iOffsetX, int iOffsetY, bool bWideScreen=false);
  int										GetWidth() const;
  int										GetHeight() const;
  void									SendMessage(CGUIMessage& message);
  void									setMessageSender(IMsgSenderCallback* pCallback);
  DWORD									GetNewID();
  const CStdString&     GetMediaDir() const;
  void									SetMediaDir(const CStdString& strMediaDir);
	bool									IsWidescreen() const;
	void									Correct(FLOAT& fCoordinateX, FLOAT& fCoordinateY, FLOAT& fCoordinateX2, FLOAT& fCoordinateY2) const;
	void									SetViewPort(float fx, float fy , float fwidth, float fheight);
	void									RestoreViewPort();
	const RECT&						GetViewWindow() const;
	void									SetViewWindow(const RECT&	rc) ;
	void									SetFullScreenVideo(bool bOnOff); 
	bool									IsFullScreenVideo() const; 
	void									SetOffset(int iXoffset, int iYoffset);
	void									Lock();
	void									Unlock();
	void									EnablePreviewWindow(bool bEnable);
protected:
	CRITICAL_SECTION			  m_critSection;
  IMsgSenderCallback*     m_pCallback;
  LPDIRECT3DDEVICE8       m_pd3dDevice;
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
};

extern CGraphicContext g_graphicsContext;
#endif