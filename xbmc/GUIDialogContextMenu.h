#pragma once
#include "GUIDialog.h"

class CGUIDialogContextMenu :
	public CGUIDialog
{
public:
	CGUIDialogContextMenu(void);
	virtual ~CGUIDialogContextMenu(void);
  virtual void    OnAction(const CAction &action);
	virtual bool		OnMessage(CGUIMessage &message);
	virtual void		DoModal(DWORD dwParentId);
	void						ClearButtons();
	void						AddButton(int iLabel);
	void						AddButton(const wstring &strButton);
	void						EnableButton(int iButton, bool bEnable);
	int							GetButton();
	DWORD						GetWidth();
	DWORD						GetHeight();

	static bool			BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iLockMode, bool bMaxRetryExceeded, int iPosX, int iPosY);

protected:
	virtual void		OnInitWindow();
	static bool			CheckMasterCode(int iLockMode);
private:
	int m_iNumButtons;
	int m_iClickedButton;
};
