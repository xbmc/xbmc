#pragma once
#include "GUIDialog.h"

class CGUIDialogProgress :
	public CGUIDialog
{
public:
	CGUIDialogProgress(void);
	virtual ~CGUIDialogProgress(void);

	void						StartModal(DWORD dwParentId);
  virtual bool    OnMessage(CGUIMessage& message);
	void						Progress();
	void						ProgressKeys();
	void					  SetLine(int iLine, const wstring& strLine);
	void					  SetLine(int iLine, const string& strLine);
	void						SetLine(int iLine, int iString);
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
	virtual void		Close();
	bool						IsCanceled() const { return m_bCanceled; }
	void						SetPercentage(int iPercentage);
	void						ShowProgressBar(bool bOnOff);
protected:
	bool						m_bCanceled;
};
