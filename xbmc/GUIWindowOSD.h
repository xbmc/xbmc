#pragma once

#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "graphiccontext.h"
#include "key.h"

#include <vector>
#include "stdstring.h"
using namespace std;

class CGUIWindowOSD : public CGUIWindow
{
public:
	CGUIWindowOSD(void);
	virtual ~CGUIWindowOSD(void);
	
	virtual bool	OnMessage(CGUIMessage& message);
	virtual void	OnAction(const CAction &action);
	virtual void	Render();

private:
	virtual void	Get_TimeInfo();
	virtual void	SetVideoProgress();
	virtual void	ToggleButton(DWORD iButtonID, bool bSelected);
	virtual void	ToggleSubMenu(DWORD iButtonID, DWORD iBackID);
	virtual void	SetSliderValue(float fMin, float fMax, float fValue, DWORD iControlID);
	virtual void	SetCheckmarkValue(BOOL bValue, DWORD iControlID);
	virtual void	Handle_ControlSetting(DWORD iControlID);
	virtual void	PopulateBookmarks();
	virtual void	PopulateAudioStreams();
	virtual void	PopulateSubTitles();
	bool			m_bSubMenuOn;
	int				m_iActiveMenu;
	DWORD			m_iActiveMenuButtonID;
	int       m_iCurrentBookmark;
};
