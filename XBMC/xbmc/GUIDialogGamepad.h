#pragma once
#include "GUIDialogBoxBase.h"

class CGUIDialogGamepad :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogGamepad(void);
  virtual ~CGUIDialogGamepad(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool IsCanceled() const;
  CStdString m_strUserInput;
  CStdString m_strPassword;
  int m_iRetries;
  bool m_bUserInputCleanup;
  bool m_bHideInputChars;
  static bool ShowAndGetInput(CStdString& aTextString, const CStdString& dlgHeading, bool bHideUserInput);
  static bool ShowAndVerifyNewPassword(CStdString& strNewPassword);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdString& dlgHeading, int iRetries);
  static bool ShowAndVerifyInput(CStdString& strPassword, const CStdString& dlgHeading, const CStdString& dlgLine0, const CStdString& dlgLine1, const CStdString& dlgLine2, bool bGetUserInput, bool bHideInputChars);
protected:
  virtual bool OnAction(const CAction &action);
  bool m_bCanceled;
  char m_cHideInputChar;
};
