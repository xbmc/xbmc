#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowSettings :
  public CGUIWindow
{
public:
  CGUIWindowSettings(void);
  virtual ~CGUIWindowSettings(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);

protected:
	int							m_iLastControl;

};
