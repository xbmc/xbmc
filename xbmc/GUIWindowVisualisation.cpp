
#include "GUIWindowVisualisation.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "visualizations/VisualisationFactory.h"
#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
:CGUIWindow(0)
{ 
	m_pVisualisation=NULL;
}

CGUIWindowVisualisation::~CGUIWindowVisualisation(void)
{
}


void CGUIWindowVisualisation::OnKey(const CKey& key)
{
	if ( key.IsButton() )
	{
		switch (key.GetButtonCode() )
		{
			case KEY_BUTTON_X:
			case KEY_REMOTE_DISPLAY:
				// back 2 UI
				m_gWindowManager.ActivateWindow(0); // back 2 home
			break;
		}
	}
  CGUIWindow::OnKey(key);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{
			if (m_pVisualisation)
				delete m_pVisualisation;
			m_pVisualisation=NULL;
		}
		break;
		case GUI_MSG_WINDOW_INIT:
		{
			CVisualisationFactory factory;
			CStdString strVisz;
			strVisz.Format("Q:\\visualisations\\%s", g_stSettings.szDefaultVisualisation);
			m_pVisualisation=factory.LoadVisualisation(strVisz.c_str());
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowVisualisation::Render()
{
	if (m_pVisualisation)
	{
		m_pVisualisation->Render();
	}
	CGUIWindow::Render();
}
