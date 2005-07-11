#pragma once
#include "GUIDialog.h"

class CGUIDialogOK :
      public CGUIDialog
{
public:
  CGUIDialogOK(void);
  virtual ~CGUIDialogOK(void);
  virtual bool OnMessage(CGUIMessage& message);
  static void ShowAndGetInput(int heading, int line0, int line1, int line2);
  bool IsConfirmed() const;
  void SetLine(int iLine, const wstring& strLine);
  void SetLine(int iLine, const string& strLine);
  void SetLine(int iLine, int iString);
  void SetHeading(const wstring& strLine);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);
protected:
  bool m_bConfirmed;

};
