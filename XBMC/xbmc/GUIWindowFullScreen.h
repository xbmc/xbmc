#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowFullScreen :
  public CGUIWindow
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	virtual void		Render();
	void				RenderFullScreen();

private:
	bool						m_bShowInfo;
	bool						m_bShowStatus;
	DWORD						m_dwLastTime;
	DWORD						m_dwFPSTime;
	float						m_fFrameCounter;
	FLOAT						m_fFPS;
};
