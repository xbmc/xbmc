#pragma once
#include "guiwindow.h"

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
	virtual void				Render();
	void								SetID3Tag(ID3_Tag& tag);
	void								SetCurrentFile(const CStdString& strFile);
	IDirect3DTexture8* 	m_pTexture;
	int									m_iTextureWidth;
	int									m_iTextureHeight;
protected:
  int                 m_iFrames;
  int                 m_iPosOrgIcon;
  int                 m_iPosOrgPlay;
  int                 m_iPosOrgPause;
  int                 m_iPosOrgInfo;
  int                 m_iPosOrgBigPlayTime;
  int                 m_iPosOrgPlayTime;
  int                 m_iPosOrgRectangle;
  void                SetPosition(int iControl, int iStep, int iMaxSteps,int iOrgPos);
  int                 GetControlYPosition(int iControl);
};
