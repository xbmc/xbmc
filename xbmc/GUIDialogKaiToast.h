#pragma once
#include "guiDialog.h"

class CGUIImage;

class CGUIDialogKaiToast: public CGUIDialog
{
public:
	CGUIDialogKaiToast(void);
	virtual ~CGUIDialogKaiToast(void);
	
	struct Notification
	{
		CStdString	caption;
		CStdString	description;
		CGUIImage*	image;
	};

	void QueueNotification(CStdString& aCaption, CStdString& aDescription);
	void QueueNotification(CGUIImage* aImage, CStdString& aCaption, CStdString& aDescription);
	bool DoWork();

	virtual bool    OnMessage(CGUIMessage& message);
	virtual void    OnAction(const CAction &action);
	virtual void    Render();
	void			ResetTimer();

protected:

	DWORD		m_dwTimer;
	CGUIImage*	m_pIcon;

	INT m_iIconPosX;
	INT m_iIconPosY;
	DWORD m_dwIconWidth;
	DWORD m_dwIconHeight;

	typedef queue<Notification> TOASTQUEUE;
	TOASTQUEUE m_notifications;
	CRITICAL_SECTION m_critical;
};
