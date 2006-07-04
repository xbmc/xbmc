#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogYesNo(void);
  virtual ~CGUIDialogYesNo(void);
  virtual bool OnMessage(CGUIMessage& message);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel=-1, int iYesLabel=-1);
  static bool ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2);
};
