#pragma once
#include "GUIDialog.h"

class CGUIDialogButtonMenu :
      public CGUIDialog
{
public:
  CGUIDialogButtonMenu(void);
  virtual ~CGUIDialogButtonMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
};
