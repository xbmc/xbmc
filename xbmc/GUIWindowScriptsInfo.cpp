
#include "stdafx.h"
#include "GUIWindowScriptsInfo.h"
#include "graphiccontext.h"
#include "localizestrings.h"
#include "util.h"
#include "application.h"

#define CONTROL_TEXTAREA 5

CGUIWindowScriptsInfo::CGUIWindowScriptsInfo(void)
:CGUIDialog(0)
{
}

CGUIWindowScriptsInfo::~CGUIWindowScriptsInfo(void)
{
}

void CGUIWindowScriptsInfo::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
    {
		Close();
		return;
    }
	CGUIWindow::OnAction(action);
}

bool CGUIWindowScriptsInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			g_application.EnableOverlay();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
			g_application.DisableOverlay();
			return true;
    }
		break;

		case GUI_MSG_USER:
    {
			strInfo += message.GetLabel();
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_TEXTAREA);
			msg.SetLabel(strInfo);
			OnMessage(msg);
		}
    break;

  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowScriptsInfo::AddText(const CStdString& strLabel)
{

}

