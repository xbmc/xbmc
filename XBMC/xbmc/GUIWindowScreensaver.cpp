#include "stdafx.h"
#include "GUIWindowScreensaver.h"
#include "application.h"
#include "util.h"
#include "screensavers/ScreenSaverFactory.h"

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
:CGUIWindow(0)
{
	//
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
	//
}

void CGUIWindowScreensaver::Render()
{
	CSingleLock lock(m_critSection);

	if (m_pScreenSaver)
	{
		if (m_bInitialized)
		{
			try
			{
        //some screensavers seem to be depending on xbmc clearing the screen
//	      g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
				m_pScreenSaver->Render();
			}
			catch(...)
			{
				OutputDebugString("Screensaver Render - ohoh!\n");
			}
			return;
		}
		else
		{
			try
			{
				m_pScreenSaver->Start();
				m_bInitialized = true;
			}
			catch(...)
			{
				OutputDebugString("Screensaver Start - ohoh!\n");
			}
			return;
		}
	}
	CGUIWindow::Render();
}

void CGUIWindowScreensaver::OnAction(const CAction &action)
{
	// We're just a screen saver, nothing to do here except maybe take screenshots
  // of the guys' hardwork :)
  if (action.wID == ACTION_TAKE_SCREENSHOT)
    CGUIWindow::OnAction(action);
}

// called when the mouse is moved/clicked etc. etc.
void CGUIWindowScreensaver::OnMouse()
{
	m_gWindowManager.PreviousWindow();
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{
			CSingleLock lock(m_critSection);
			
			if (m_pScreenSaver)
			{
				OutputDebugString("ScreenSaver::Stop()\n");
				m_pScreenSaver->Stop();
				
				OutputDebugString("delete ScreenSaver()\n");
				delete m_pScreenSaver;

        g_graphicsContext.ApplyStateBlock();
			}
			m_pScreenSaver=NULL;
			m_bInitialized = false;
			
			// remove z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, FALSE);

			// enable the overlay
			g_graphicsContext.SetOverlay(true);
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			CSingleLock lock(m_critSection);

			if (m_pScreenSaver)
			{
				m_pScreenSaver->Stop();
				delete m_pScreenSaver;
				g_graphicsContext.ApplyStateBlock();
			}
			m_pScreenSaver = NULL;
			m_bInitialized = false;

			// Setup new screensaver instance
			CScreenSaverFactory factory;
			CStdString strScr;
			OutputDebugString("Load Screensaver\n");
			strScr.Format("Q:\\screensavers\\%s", g_guiSettings.GetString("ScreenSaver.Mode").c_str());
			m_pScreenSaver=factory.LoadScreenSaver(strScr.c_str());
			if (m_pScreenSaver) 
			{
				OutputDebugString("ScreenSaver::Create()\n");
				g_graphicsContext.CaptureStateBlock();
				m_pScreenSaver->Create();
			}

			// setup a z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, TRUE);

			// disable the overlay
			g_graphicsContext.SetOverlay(false);
			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
}
