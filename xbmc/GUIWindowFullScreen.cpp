
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
			g_settings.Save();
			return;
		}
		// stretch->normal
		if (g_stSettings.m_bStretch)
		{
			g_stSettings.m_bZoom=false;
			g_stSettings.m_bStretch=false;
			g_settings.Save();
			return;
		}
		// normal->zoom
		g_stSettings.m_bZoom=true;
		g_stSettings.m_bStretch=false;
		g_settings.Save();
		return;

	}

  CGUIWindow::OnKey(key);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{

	return CGUIWindow::OnMessage(message);
}
