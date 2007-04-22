#pragma once

#include "GUIDialog.h"

#ifdef WITH_LINKS_BROWSER
#include "LinksBoksManager.h"

class CGUIDialogWebBrowserHistory :
      public CGUIDialog
{
public:
  CGUIDialogWebBrowserHistory(void);
  virtual ~CGUIDialogWebBrowserHistory(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

protected:
  void PopulateHistory();
  CFileItemList m_vecHistory;
};

#endif