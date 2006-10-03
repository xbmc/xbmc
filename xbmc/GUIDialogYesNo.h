#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogYesNo(void);
  virtual ~CGUIDialogYesNo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel=-1, int iYesLabel=-1);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, bool& bCanceled);
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel, bool& bCanceled);
  static bool ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2);
  static bool ShowAndGetInput(const CStdString& heading, const CStdString& line0, const CStdString& line1, const CStdString& line2, bool &bCanceled);
protected:
  bool m_bCanceled;
};
