#pragma once
#include "GUIWindow.h"

class CGUIWindowSystemInfo :
	public CGUIWindow
{
public:
	CGUIWindowSystemInfo(void);
	virtual ~CGUIWindowSystemInfo(void);
	virtual bool    OnMessage(CGUIMessage& message);
	virtual void    OnAction(const CAction &action);
	virtual void	Render();	
protected:
	void			GetValues();
	DWORD           m_dwFPSTime;
	DWORD           m_dwFrames;
	float           m_fFPS;
	wchar_t			m_wszMPlayerVersion[50];
};
