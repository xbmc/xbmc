#pragma once
#include "GUIDialog.h"

class CGUIDialogOK :
	public CGUIDialog
{
public:
	CGUIDialogOK(void);
	virtual ~CGUIDialogOK(void);
  virtual bool    OnMessage(CGUIMessage& message);
  static void     ShowAndGetInput(const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2);
	bool						IsConfirmed() const;
	void					  SetLine(int iLine, const wstring& strLine);
	void					  SetLine(int iLine, const string& strLine);
	void						SetLine(int iLine, int iString);
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
  virtual void    OnAction(const CAction &action);
protected:
	bool m_bConfirmed;

};
