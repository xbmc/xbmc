#pragma once
#include "guidialog.h"
#include "GUIList.h"

class CGUIDialogInvite :
	public CGUIDialog
{
public:
	CGUIDialogInvite(void);
	virtual ~CGUIDialogInvite(void);
	virtual bool	OnMessage(CGUIMessage& message);

	bool			IsConfirmed() const;
	void			SetGames(CGUIList* aGamesList);

protected:
	virtual void OnInitWindow();
protected:
	bool m_bConfirmed;
	CGUIList* m_pGames;
};
