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
	virtual void				OnMouse();
  virtual void				Render();
	void								SetID3Tag(ID3_Tag& tag);
	void								SetCurrentFile(CFileItem& item);
	IDirect3DTexture8* 	m_pTexture;
	int									m_iTextureWidth;
	int									m_iTextureHeight;
  virtual void				FreeResources();
protected:
	void								UpdateInfo(const CMusicInfoTag &tag);
	long								m_lStartOffset;
  int                 m_iFrames;
  bool                m_bShowInfo;
  bool								m_bShowInfoAlways;
  int									m_iFrameIncrement;
  DWORD								m_dwTimeout;
  int                 m_iTopPosition;
  void                UpdatePosition(int iStep, int iMaxSteps);
  void                GetTopControlPosition();
  void                ShowControl(int iControl);
  void                HideControl(int iControl);
};
