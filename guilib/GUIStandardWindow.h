#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"

class CGUIStandardWindow :
	public CGUIWindow
{
public:
	CGUIStandardWindow(void);
	virtual ~CGUIStandardWindow(void);

	virtual bool	OnMessage(CGUIMessage& message);
	virtual void	OnAction(const CAction &action);

protected:
	int						m_iLastControl;
};