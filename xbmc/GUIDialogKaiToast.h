#pragma once
#include "StdString.h"
#include "guidialog.h"

class CGUIDialogKaiToast: public CGUIDialog
{
public:
	CGUIDialogKaiToast(void);
	virtual ~CGUIDialogKaiToast(void);
	
	void SetNotification(CStdString& aCaption, CStdString& aDescription);

	virtual bool    OnMessage(CGUIMessage& message);
	virtual void    OnAction(const CAction &action);
	virtual void    Render();
	void			ResetTimer();
protected:
	DWORD	m_dwTimer;
};
