#pragma once
#include "StdString.h"
#include "guidialog.h"
#include <queue>

class CGUIDialogKaiToast: public CGUIDialog
{
public:
	CGUIDialogKaiToast(void);
	virtual ~CGUIDialogKaiToast(void);
	
	struct Notification
	{
		CStdString	caption;
		CStdString	description;
	};

	void QueueNotification(CStdString& aCaption, CStdString& aDescription);
	bool DoWork();

	virtual bool    OnMessage(CGUIMessage& message);
	virtual void    OnAction(const CAction &action);
	virtual void    Render();
	void			ResetTimer();

protected:
	DWORD	m_dwTimer;

	typedef std::queue<Notification> TOASTQUEUE;
	TOASTQUEUE m_notifications;
	CRITICAL_SECTION m_critical;
};
