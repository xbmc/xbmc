#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
class CGUIWindowSettingsProfile :
  public CGUIWindow
{
public:
  CGUIWindowSettingsProfile(void);
  virtual ~CGUIWindowSettingsProfile(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);

protected:
	int   m_iLastControl;

  bool  GetKeyboard(CStdString& strInput);
  int   GetSelectedItem();
  void  LoadList();
  void  SetLastLoaded();
};
