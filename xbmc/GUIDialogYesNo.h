#pragma once
#include "guidialog.h"

class CGUIDialogYesNo :
	public CGUIDialog
{
public:
	CGUIDialogYesNo(void);
	virtual ~CGUIDialogYesNo(void);
  virtual bool    OnMessage(CGUIMessage& message);
	bool						IsConfirmed() const;
  static bool     ShowAndGetInput(const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2);
	void					  SetLine(int iLine, const wstring& strLine);
	void					  SetLine(int iLine, const string& strLine);
	void						SetLine(int iLine, int iString);
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
protected:
	bool m_bConfirmed;

};
