#include "stdafx.h"
#include "GUIDialogKaiToast.h"
#include "GUISliderControl.h"
#include "application.h"

// May need to change this so that it is "modeless" rather than Modal,
// though it works reasonably well as is...

#define TOAST_DISPLAY_TIME			5000L

#define POPUP_ICON					400
#define POPUP_CAPTION_TEXT			401
#define POPUP_NOTIFICATION_BUTTON	402

CGUIDialogKaiToast::CGUIDialogKaiToast(void) : CGUIDialog(0)
{
	m_bNeedsScaling	= true;	// make sure we scale this window, as it appears on different resolutions
	m_pIcon			= NULL;
	m_iIconPosX		= 0;
	m_iIconPosY		= 0;
	m_dwIconWidth	= 0;
	m_dwIconHeight	= 0;

	InitializeCriticalSection(&m_critical);
}

CGUIDialogKaiToast::~CGUIDialogKaiToast(void)
{
	DeleteCriticalSection(&m_critical);
}

void CGUIDialogKaiToast::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
	{
		Close();
		return;
	}
	CGUIDialog::OnAction(action);
}

bool CGUIDialogKaiToast::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
			// resources are allocated in g_application

			CGUIImage* pIcon	= (CGUIImage*) GetControl(POPUP_ICON);

			if (pIcon)
			{
				m_iIconPosX			= pIcon->GetXPosition();
				m_iIconPosY			= pIcon->GetYPosition();
				m_dwIconWidth		= pIcon->GetWidth();
				m_dwIconHeight		= pIcon->GetHeight();
			}

			ResetTimer();
			return true;
		}
		break;

 		case GUI_MSG_WINDOW_DEINIT:
		{
			//don't deinit, g_application handles it
			return true;
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}


void CGUIDialogKaiToast::QueueNotification(CStdString& aCaption, CStdString& aDescription)
{
	EnterCriticalSection(&m_critical);
	
	Notification toast;
	toast.image = NULL;
	toast.caption = aCaption;
	toast.description = aDescription;
	m_notifications.push(toast);

	LeaveCriticalSection(&m_critical);
}

void CGUIDialogKaiToast::QueueNotification(CGUIImage* aImage, CStdString& aCaption, CStdString& aDescription)
{
	EnterCriticalSection(&m_critical);
	
	Notification toast;
	toast.image = aImage;
	toast.caption = aCaption;
	toast.description = aDescription;
	m_notifications.push(toast);

	LeaveCriticalSection(&m_critical);
}


bool CGUIDialogKaiToast::DoWork()
{
	EnterCriticalSection(&m_critical);
	
	bool bPending = m_notifications.size()>0;
	if (bPending)
	{
		Notification toast = m_notifications.front();
		m_notifications.pop();

		g_graphicsContext.Lock();

		CGUIMessage msg1(GUI_MSG_LABEL_SET,GetID(),POPUP_CAPTION_TEXT);
		msg1.SetLabel(toast.caption);
		OnMessage(msg1);

		CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(),POPUP_NOTIFICATION_BUTTON);
		msg2.SetLabel(toast.description);
		OnMessage(msg2);

		CGUIImage* pOldIcon = m_pIcon;

		if (pOldIcon)
		{
			m_pIcon= NULL;
			pOldIcon->FreeResources();
		}

		if (toast.image)
		{
			toast.image->AllocResources();
			m_pIcon = toast.image;
		}

		g_graphicsContext.Unlock();

		ResetTimer();
	}

	LeaveCriticalSection(&m_critical);

	return bPending;
}


void CGUIDialogKaiToast::ResetTimer()
{
	m_dwTimer = timeGetTime();
}

void CGUIDialogKaiToast::Render()
{
	if (m_bRunning)
	{
		if (m_pIcon)
		{
			SET_CONTROL_HIDDEN(POPUP_ICON);
			CGUIDialog::Render();
			m_pIcon->Render(m_iIconPosX+m_iPosX, m_iIconPosY+m_iPosY, m_dwIconWidth, m_dwIconHeight);
		}
		else
		{
			SET_CONTROL_VISIBLE(POPUP_ICON);
			CGUIDialog::Render();
		}

		// now check if we should exit
		if (timeGetTime() - m_dwTimer > TOAST_DISPLAY_TIME)
		{
			Close();
		}
	}
}