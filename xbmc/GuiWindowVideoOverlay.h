#pragma once
#include "guiwindow.h"

#include "stdstring.h"
#include "FileItem.h"
#include <vector>
using namespace std;

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
