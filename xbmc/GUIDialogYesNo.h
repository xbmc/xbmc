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
	void					  SetLine(int iLine, const wstring& strLine);
	void						SetHeading(const wstring& strLine);
protected:
	bool m_bConfirmed;

};
