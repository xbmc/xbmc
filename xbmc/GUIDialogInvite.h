#pragma once
#include "GUIDialog.h"
#include "GUIList.h"

class CGUIDialogInvite :
	public CGUIDialog
{
public:
	CGUIDialogInvite(void);
	virtual ~CGUIDialogInvite(void);
	virtual bool	OnMessage(CGUIMessage& message);

	bool			IsConfirmed() const;
	bool			GetSelectedVector(CStdString& aVector);
	void			GetPersonalMessage(CStdString& aMessage);
	void			SetGames(CGUIList* aGamesList);

protected:
	virtual void OnInitWindow();
protected:
	bool m_bMessage;
	bool m_bConfirmed;
	int m_iSelectedIndex;
	CGUIList* m_pGames;
	CStdString m_strMessage;
};
