
#include "GUIWindowFullScreen.h"
#include "settings.h"

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
}


void CGUIWindowFullScreen::OnKey(const CKey& key)
{
	if ( key.GetButtonCode() == KEY_BUTTON_START  || key.GetButtonCode() == KEY_REMOTE_DISPLAY)
	{
		// zoom->stretch
		if (g_stSettings.m_bZoom)
		{
			g_stSettings.m_bZoom=false;
			g_stSettings.m_bStretch=true;
			return;
		}
		// stretch->normal
		if (g_stSettings.m_bStretch)
		{
			g_stSettings.m_bZoom=false;
			g_stSettings.m_bStretch=false;
			return;
		}
		// normal->zoom
		g_stSettings.m_bZoom=true;
		g_stSettings.m_bStretch=false;
		return;

	}

  CGUIWindow::OnKey(key);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{

	return CGUIWindow::OnMessage(message);
}
