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
	void					  SetLine(int iLine, const string& strLine);
	void						SetLine(int iLine, int iString);
	void						SetHeading(const wstring& strLine);
	void						SetHeading(int iString);
	virtual void		Close();
};
