
#include "stdafx.h"
#include "localizestrings.h"
#include "GUIWindowScreensaver.h"
#include "application.h"
#include "settings.h"
#include "guiFontManager.h"
#include "util.h"
#include "sectionloader.h"
#include "screensavers/ScreenSaverFactory.h"
#include "utils/CriticalSection.h"

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
	// We're just a screen saver, nothing to do here ...	
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
			}
			m_pScreenSaver=NULL;
			m_bInitialized = false;
			
			// remove z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, FALSE);
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
				m_pScreenSaver->Create();
			}

			// setup a z-buffer
			RESOLUTION res = g_graphicsContext.GetVideoResolution();
			g_graphicsContext.SetVideoResolution(res, TRUE);
			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
}
