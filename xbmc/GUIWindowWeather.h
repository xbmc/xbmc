#pragma once
#include "GUIWindow.h"

class CGUIWindowWeather : 	public CGUIWindow
{
public:
	CGUIWindowWeather(void);
	virtual ~CGUIWindowWeather(void);
	virtual bool			OnMessage(CGUIMessage& message);
	virtual void			OnAction(const CAction &action);
	virtual void			Render();

protected:
	void					UpdateButtons();
	void					Refresh();

	unsigned int			m_iCurWeather;
};
