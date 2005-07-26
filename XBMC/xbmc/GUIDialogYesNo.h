#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogYesNo(void);
  virtual ~CGUIDialogYesNo(void);
  virtual bool OnMessage(CGUIMessage& message);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2);
};
