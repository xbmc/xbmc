#include "stdafx.h"
#include "GUIWindowScriptsInfo.h"

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
	if (action.wID == ACTION_SHOW_INFO)
	{
		// erase debug screen
		strInfo="";
		CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_TEXTAREA);
		msg.SetLabel(strInfo);
		OnMessage(msg);
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowScriptsInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
			return true;
    }
		break;

		case GUI_MSG_USER:
    {
			strInfo += message.GetLabel();
			CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_TEXTAREA);
			msg.SetLabel(strInfo);
			msg.SetParam1(5); // 5 pages max
			OnMessage(msg);
		}
    break;
  }
  return CGUIWindow::OnMessage(message);
}
