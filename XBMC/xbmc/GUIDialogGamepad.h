#pragma once
#include "GUIDialog.h"

class CGUIDialogGamepad :
	public CGUIDialog
{
public:
	CGUIDialogGamepad(void);
	virtual ~CGUIDialogGamepad(void);
  virtual bool    OnMessage(CGUIMessage& message);
	bool						IsConfirmed() const;
	bool						IsCanceled() const;
	void					  SetLine(int iLine, const wstring& strLine);
	void					  SetLine(int iLine, const string& strLine);
	void						SetLine(int iLine, int iString);
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
  CStdStringW     m_strUserInput;
  CStdStringW     m_strPassword;
	int							m_iRetries;
    bool          m_bUserInputCleanup;
	  bool					m_bHideInputChars;
 	static bool					ShowAndGetInput(CStdStringW& aTextString, const CStdStringW &dlgHeading, bool bHideUserInput);
	static bool					ShowAndGetNewPassword(CStdStringW& strNewPassword);
  static int          ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2);
  static int          ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& dlgHeading, int iRetries);
	static bool					ShowAndVerifyInput(CStdStringW& strPassword, const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2, bool bGetUserInput, bool bHideInputChars);
protected:
	virtual void OnAction(const CAction &action);
	bool m_bConfirmed;
    bool m_bCanceled;
    wchar_t m_cHideInputChar;
};
