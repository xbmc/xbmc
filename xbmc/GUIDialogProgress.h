#pragma once
#include "guidialog.h"

class CGUIDialogProgress :
	public CGUIDialog
{
public:
	CGUIDialogProgress(void);
	virtual ~CGUIDialogProgress(void);

	void						StartModal(DWORD dwParentId);
	void						Progress();
	void					  SetLine(int iLine, const wstring& strLine);
	void						SetHeading(const wstring& strLine);
};
