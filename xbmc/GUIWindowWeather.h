#pragma once
#include "guiImage.h"
#include "guiLabelControl.h"
#include "guiwindow.h"
#include "stdstring.h"
#include "utils\HTTP.h"
#include "tinyxml/tinyxml.h"
#include "guiDialogProgress.h"
#include "utils/thread.h"

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
