#pragma once

#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "graphiccontext.h"
#include "key.h"

#include <vector>
#include "stdstring.h"
using namespace std;

class CGUIWindowSlideShow : public CGUIWindow
{
public:
	CGUIWindowSlideShow(void);
	virtual ~CGUIWindowSlideShow(void);
	
	void		Reset();
	void		Add(const CStdString& strPicture);
  bool    IsPlaying() const;
  void    ShowNext();
  void    ShowPrevious();
  void    Select(const CStdString& strPicture);
  void    StartSlideShow();
  bool    InSlideShow() const;
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void		Render();

private:
  struct VERTEX 
	{ 
    D3DXVECTOR4 p;
		D3DCOLOR col; 
		FLOAT tu, tv; 
	};
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;
	
  bool               RenderMethod1();
  bool               RenderMethod2();
  bool               RenderMethod3();
  bool               RenderMethod4();
  bool               RenderMethod5();
  bool               RenderMethod6();
  bool               RenderMethod7();
  bool               RenderMethod8();
	bool							 RenderMethod9();
  IDirect3DTexture8* GetNextSlide(DWORD& dwWidth, DWORD& dwHeight, CStdString& strSlide);
	IDirect3DTexture8* GetPreviousSlide(DWORD& dwWidth, DWORD& dwHeight, CStdString& strSlide);
  void               RenderPause();
  void               DoRotate();
  void				 GetOutputRect(const int iSourceWidth, const int iSourceHeight, int& x, int& y, int& width,int& height);
  void				 Zoom(int iZoom);

	DWORD										m_lSlideTime;

	IDirect3DTexture8* 			m_pTextureBackGround;
	IDirect3DSurface8*			m_pSurfaceBackGround;
	DWORD							 			m_dwWidthBackGround;
	DWORD							 			m_dwHeightBackGround;

	IDirect3DTexture8* 			m_pTextureCurrent;
	IDirect3DSurface8*			m_pSurfaceCurrent;
	DWORD							 			m_dwWidthCurrent;
	DWORD							 			m_dwHeightCurrent;

  DWORD              			m_dwFrameCounter;
  int								 			m_iCurrentSlide;
  int                			m_iTransistionMethod;
  bool               			m_bSlideShow ;
	bool							 			m_bShowInfo;
	CStdString						 			m_strBackgroundSlide;
	CStdString						 			m_strCurrentSlide;
  bool                    m_bPause;
  int                     m_iZoomFactor;
  int                     m_iPosX,m_iPosY;
  int                     m_iRotate;

	vector<CStdString>		 m_vecSlides;
	typedef vector<CStdString>::iterator ivecSlides;

};
