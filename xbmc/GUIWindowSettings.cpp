
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
	if (action.wID == ACTION_PARENT_MENU)
    {
		m_gWindowManager.ActivateWindow(WINDOW_HOME); // back 2 home
		return;
    }
	CGUIWindow::OnAction(action);
}

bool CGUIWindowSettings::OnMessage(CGUIMessage& message)
{
 return CGUIWindow::OnMessage(message);
}
