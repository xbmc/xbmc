#pragma once
#include "GUIDialog.h"

class CGUIDialogPlayerControls :
      public CGUIDialog
{
public:
  CGUIDialogPlayerControls(void);
  virtual ~CGUIDialogPlayerControls(void);

  virtual bool OnAction(const CAction &action);
  virtual void Render();
};
