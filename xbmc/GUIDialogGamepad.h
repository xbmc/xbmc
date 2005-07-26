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
  CStdStringW m_strUserInput;
  CStdStringW m_strPassword;
  int m_iRetries;
  bool m_bUserInputCleanup;
  bool m_bHideInputChars;
  static bool ShowAndGetInput(CStdString& aTextString, const CStdStringW &dlgHeading, bool bHideUserInput);
  static bool ShowAndGetNewPassword(CStdString& strNewPassword);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdStringW& dlgHeading, int iRetries);
  static bool ShowAndVerifyInput(CStdString& strPassword, const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2, bool bGetUserInput, bool bHideInputChars);
protected:
  virtual bool OnAction(const CAction &action);
  bool m_bCanceled;
  wchar_t m_cHideInputChar;
};
