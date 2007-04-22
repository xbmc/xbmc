#pragma once

#include "GUIDialog.h"

#ifdef WITH_LINKS_BROWSER
#include "LinksBoksManager.h"

class CGUIDialogWebBrowserOSD :
      public CGUIDialog
{
public:
  CGUIDialogWebBrowserOSD(void);
  virtual ~CGUIDialogWebBrowserOSD(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
protected:
  int m_iLastControl;

};

#endif