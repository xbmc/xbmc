
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

CGUIDialogKaiToast::CGUIDialogKaiToast(void)
:CGUIDialog(0)
{
	m_bNeedsScaling = true;	// make sure we scale this window, as it appears on different resolutions
}

CGUIDialogKaiToast::~CGUIDialogKaiToast(void)
{
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

		case GUI_MSG_CLICKED:
		{
			if (message.GetSenderId()==POPUP_NOTIFICATION_BUTTON)	// who else is it going to be??
			{
				return true;
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

void CGUIDialogKaiToast::SetNotification(CStdString& aCaption, CStdString& aDescription)
{
	CGUIMessage msg1(GUI_MSG_LABEL_SET,GetID(),POPUP_CAPTION_TEXT);
	msg1.SetLabel(aCaption);
	OnMessage(msg1);

	CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(),POPUP_NOTIFICATION_BUTTON);
	msg2.SetLabel(aDescription);
	OnMessage(msg2);

	ResetTimer();
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