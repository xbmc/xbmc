#pragma once
#include "guiwindow.h"
#include "FileItem.h"
#include "stdstring.h"
#include <vector>
using namespace std;

class ID3_Tag;
class CGUIWindowMusicOverlay: 	public CGUIWindow
{
public:
	CGUIWindowMusicOverlay(void);
	virtual ~CGUIWindowMusicOverlay(void);
  virtual bool				OnMessage(CGUIMessage& message);
  virtual void				OnAction(const CAction &action);
	virtual void			OnMouse();
  virtual void				Render();
	void								SetID3Tag(ID3_Tag& tag);
	void								SetCurrentFile(CFileItem& item);
	IDirect3DTexture8* 	m_pTexture;
	int									m_iTextureWidth;
	int									m_iTextureHeight;
  virtual void				FreeResources();

protected:
	long			  m_lStartOffset;
  int                 m_iFrames;
  bool                m_bShowInfo;
  bool				m_bShowInfoAlways;
  int				  m_iFrameIncrement;
  DWORD				  m_dwTimeout;
  int                 m_iPosOrgIcon;
  int                 m_iPosOrgPlay;
  int                 m_iPosOrgPause;
  int                 m_iPosOrgInfo;
  int                 m_iPosOrgBigPlayTime;
  int                 m_iPosOrgPlayTime;
  int                 m_iPosOrgRectangle;
  void                SetPosition(int iControl, int iStep, int iMaxSteps,int iOrgPos);
  int                 GetControlYPosition(int iControl);
  void                ShowControl(int iControl);
  void                HideControl(int iControl);
};
