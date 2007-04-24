#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogOK :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogOK(void);
  virtual ~CGUIDialogOK(void);
  virtual bool OnMessage(CGUIMessage& message);
  static void ShowAndGetInput(int heading, int line0, int line1, int line2);
};
