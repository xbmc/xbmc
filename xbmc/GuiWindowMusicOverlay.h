#pragma once
#include "GUIWindow.h"

class CGUIWindowMusicOverlay: 	public CGUIWindow
{
public:
	CGUIWindowMusicOverlay(void);
	virtual ~CGUIWindowMusicOverlay(void);
  virtual bool				OnMessage(CGUIMessage& message);
  virtual void				OnAction(const CAction &action);
	virtual void				OnMouse();
  virtual void				Render();
  virtual void				FreeResources();
	void								Update();
	void								UpdateInfo(const CMusicInfoTag &tag);
	
protected:
  virtual void OnWindowLoaded();
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
