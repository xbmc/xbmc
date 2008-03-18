#pragma once
#include "GUIDialog.h"

class CGUIDialogSubMenu :
      public CGUIDialog
{
public:
  CGUIDialogSubMenu(void);
  virtual ~CGUIDialogSubMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
};
