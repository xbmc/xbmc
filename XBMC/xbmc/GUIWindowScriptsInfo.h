#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"

#include "stdstring.h"

class CGUIWindowScriptsInfo :
  public CGUIDialog
{
public:
  CGUIWindowScriptsInfo(void);
  virtual ~CGUIWindowScriptsInfo(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	void						AddText(const CStdString& strLabel);
protected:
	CStdStringW			strInfo;
};
