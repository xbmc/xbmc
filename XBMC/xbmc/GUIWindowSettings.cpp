
#include "GUIWindowSettings.h"

CGUIWindowSettings::CGUIWindowSettings(void)
:CGUIWindow(0)
{
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
 return CGUIWindow::OnMessage(message);
}
