#pragma once
#include "GUIWindow.h"

#include "FileItem.h"

class CGUIWindowVideoOverlay: 	public CGUIWindow
{
public:
	CGUIWindowVideoOverlay(void);
	virtual ~CGUIWindowVideoOverlay(void);
  virtual bool				OnMessage(CGUIMessage& message);
  virtual void				OnAction(const CAction &action);
	virtual void				Render();
	void								SetCurrentFile(const CFileItem& item);
protected:
  void                HideControl(int iControl);
  void                ShowControl(int iControl);
};
