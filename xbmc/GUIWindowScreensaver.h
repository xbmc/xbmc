#pragma once

#include "GUIWindow.h"
#include "screensavers/ScreenSaver.h"
#include "Utils/CriticalSection.h"

#define SCREENSAVER_FADE   1
#define SCREENSAVER_BLACK  2
#define SCREENSAVER_XBS	   3

class CGUIWindowScreensaver : public CGUIWindow
{
public:
	CGUIWindowScreensaver(void);
	virtual ~CGUIWindowScreensaver(void);
	
	virtual bool	OnMessage(CGUIMessage& message);
	virtual void	OnAction(const CAction &action);
	virtual void	OnMouse();
	virtual void	Render();

private:
	CScreenSaver* m_pScreenSaver;
	bool m_bInitialized;
	CCriticalSection	m_critSection;
};
