
#include "GUIWindowFullScreen.h"
#include "settings.h"
#include "application.h"

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

	if ( key.IsButton() )
	{
		switch (key.GetButtonCode() )
		{
			case KEY_BUTTON_DPAD_LEFT:
			case KEY_REMOTE_LEFT:
				g_application.m_pPlayer->Seek(false,false);
			break;

			case KEY_BUTTON_DPAD_RIGHT:
			case KEY_REMOTE_RIGHT:
				g_application.m_pPlayer->Seek(true,false);
			break;

			
			case KEY_BUTTON_DPAD_UP:
			case KEY_REMOTE_UP:
				g_application.m_pPlayer->Seek(true,true);
			break;

			case KEY_BUTTON_DPAD_DOWN:
			case KEY_REMOTE_DOWN:
				g_application.m_pPlayer->Seek(false,true);
			break;

			case KEY_BUTTON_BLACK:
			case KEY_REMOTE_INFO:
				g_application.m_pPlayer->ToggleOSD();
			break;
				
			case KEY_BUTTON_WHITE:
			case KEY_REMOTE_TITLE:
				g_application.m_pPlayer->ToggleSubtitles();
			break;

			case KEY_BUTTON_A:
				g_application.m_pPlayer->closefile();
			break;

			case KEY_BUTTON_BACK:
				g_application.m_pPlayer->Pause();
			break;


		}
	}
  CGUIWindow::OnKey(key);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{

	return CGUIWindow::OnMessage(message);
}
