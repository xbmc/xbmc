
#include "stdafx.h"
#include "GUIWindowSettings.h"
#include "Credits.h"

#define CONTROL_CREDITS (10)

CGUIWindowSettings::CGUIWindowSettings(void)
:CGUIWindow(0)
{
	m_iLastControl=-1;
}

CGUIWindowSettings::~CGUIWindowSettings(void)
{
}

void CGUIWindowSettings::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindow::OnAction(action);
}

bool CGUIWindowSettings::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
		{
			m_iLastControl=GetFocusedControl();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

	    if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);

			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if (iControl == CONTROL_CREDITS)
			{
				RunCredits();
			}
		}
		break;
	}

	return CGUIWindow::OnMessage(message);
}
