#pragma once
#include "guidialog.h"

class CGUIDialogContextMenu :
	public CGUIDialog
{
public:
	CGUIDialogContextMenu(void);
	virtual ~CGUIDialogContextMenu(void);
  virtual void    OnAction(const CAction &action);
	virtual bool		OnMessage(CGUIMessage &message);
	virtual void		DoModal(DWORD dwParentId);
	void						AddButton(int iLabel);
	void						AddButton(const wstring &strButton);
	int							GetButton();
	DWORD						GetWidth();
	DWORD						GetHeight();

protected:
	virtual void		OnInitWindow();

private:
	int m_iNumButtons;
	int m_iClickedButton;
};
