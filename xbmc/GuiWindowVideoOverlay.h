#pragma once
#include "GUIWindow.h"

#include "FileItem.h"

class CGUIWindowVideoOverlay: 	public CGUIWindow
{
public:
	CGUIWindowVideoOverlay(void);
	virtual ~CGUIWindowVideoOverlay(void);
	virtual void				Render();
	void								Update();
};
