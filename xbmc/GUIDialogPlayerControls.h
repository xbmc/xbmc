#pragma once
#include "GUIDialog.h"

class CGUIDialogPlayerControls :
      public CGUIDialog
{
public:
  CGUIDialogPlayerControls(void);
  virtual ~CGUIDialogPlayerControls(void);

  virtual void OnAction(const CAction &action);
  virtual void Render();
};
