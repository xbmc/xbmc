
#include "stdafx.h"
#include "GUIDialogKaiToast.h"
#include "GUISliderControl.h"
#include "guiWindowManager.h"
#include "settings.h"
#include "application.h"
#include "localizeStrings.h"

// May need to change this so that it is "modeless" rather than Modal,
// though it works reasonably well as is...

#define TOAST_DISPLAY_TIME			5000L

#define POPUP_CAPTION_TEXT			401
#define POPUP_NOTIFICATION_BUTTON	402



CGUIDialogKaiToast::CGUIDialogKaiToast(void) : CGUIDialog(0)
{
	m_bNeedsScaling = true;	// make sure we scale this window, as it appears on different resolutions

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

		CGUIMessage msg1(GUI_MSG_LABEL_SET,GetID(),POPUP_CAPTION_TEXT);
		msg1.SetLabel(toast.caption);
		OnMessage(msg1);

		CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(),POPUP_NOTIFICATION_BUTTON);
		msg2.SetLabel(toast.description);
		OnMessage(msg2);

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
	// render the controls
	CGUIDialog::Render();

	// now check if we should exit
	if (timeGetTime() - m_dwTimer > TOAST_DISPLAY_TIME)
	{
		Close();
		return;
	}
}